#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <time.h>

#include "jbd013_api.h"
#include "hal_driver.h"
#include "../../SDK/ai_glass_sdk/include/ai_display.h"
#include "../../SDK/ai_glass_sdk/include/ai_gpio.h"

#define LOG_TAG "DisplayService"

// ==================== 省电功能配置 ====================
#define POWER_SAVE_TIMEOUT  30  // 30秒无活动息屏
#define GPIO_KEY_0   0
#define GPIO_KEY_1   1
#define GPIO_KEY_75  75

static volatile int keeping_running = 1;
static int shm_fd = -1;
static ai_display_shm_t *shm_ptr = NULL;
static int server_socket = -1;

// 省电功能变量
static volatile time_t last_activity_time = 0;
static volatile int display_off = 0;
static gpio_event_hub_client_t gpio_hub_client;  // v2.0 GPIO Hub 客户端


// ==================== GPIO 唤醒回调 ====================
void gpio_wakeup_callback(gpio_event_t event, int gpio, void *data) {
    // 任何 GPIO 事件都重置活动时间
    last_activity_time = time(NULL);
    
    // 如果屏幕已关闭，则唤醒
    if (display_off) {
        send_cmd(SPI_DISPLAY_ENABLE);
        send_cmd(SPI_SYNC);
        display_off = 0;
        printf("[%s] Display woken by GPIO %d\n", LOG_TAG, gpio);
    }
}

void sig_handler(int signo) {
    if (signo == SIGINT || signo == SIGTERM) {
        printf("[%s] Received signal %d, stopping...\n", LOG_TAG, signo);
        keeping_running = 0;
    }
}

// 初始化共享内存
int init_shm() {
    shm_fd = shm_open(AI_DISPLAY_SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        return -1;
    }

    if (ftruncate(shm_fd, AI_DISPLAY_SHM_SIZE) == -1) {
        perror("ftruncate failed");
        return -1;
    }

    shm_ptr = (ai_display_shm_t *)mmap(0, AI_DISPLAY_SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap failed");
        return -1;
    }

    // 初始化 SHM 头
    memset(shm_ptr, 0, AI_DISPLAY_SHM_SIZE);
    shm_ptr->magic = 0x44495350; // "DISP"
    shm_ptr->active_client_pid = 0;

    printf("[%s] Shared memory initialized\n", LOG_TAG);
    return 0;
}

// 处理客户端连接 (持久连接，循环读取消息)
void *handle_client_thread(void *arg) {
    int client_sock = *(int*)arg;
    free(arg);
    
    ai_display_msg_t msg;
    
    while (keeping_running) {
        ssize_t n = recv(client_sock, &msg, sizeof(msg), 0);
        if (n <= 0) {
            // 客户端断开连接或出错
            break;
        }

        if (msg.type == AI_DISPLAY_MSG_COMMIT) {
            // 如果屏幕已关闭，先唤醒
            if (display_off) {
                send_cmd(SPI_DISPLAY_ENABLE);
                send_cmd(SPI_SYNC);
                display_off = 0;
                printf("[%s] Display woken by frame commit\n", LOG_TAG);
            }
            
            // 更新活动时间
            last_activity_time = time(NULL);
            
            uint8_t *frame_data = NULL;
            if (msg.slot_index == 0) {
                frame_data = shm_ptr->framebuffer_slot_0;
            } else if (msg.slot_index == 1) {
                frame_data = shm_ptr->framebuffer_slot_1;
            }

            if (frame_data) {
                display_image(0, 0, frame_data, AI_DISPLAY_FRAME_SIZE);
            }
        } else if (msg.type == AI_DISPLAY_MSG_REQUEST_FOCUS) {
            shm_ptr->active_client_pid = msg.pid;
            printf("[%s] Client %d acquired focus\n", LOG_TAG, msg.pid);
        }
    }
    
    close(client_sock);
    printf("[%s] Client disconnected\n", LOG_TAG);
    return NULL;
}

// Socket 服务线程
void *socket_server_thread(void *arg) {
    struct sockaddr_un addr;
    char socket_path[] = "/tmp/ai_display_service";

    if ((server_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);

    unlink(socket_path);

    if (bind(server_socket, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind error");
        exit(1);
    }

    if (listen(server_socket, 5) == -1) {
        perror("listen error");
        exit(1);
    }

    printf("[%s] Listening on %s\n", LOG_TAG, socket_path);

    while (keeping_running) {
        int client_sock = accept(server_socket, NULL, NULL);
        if (client_sock == -1) {
            if (errno == EINTR) continue;
            perror("accept error");
            continue;
        }
        
        printf("[%s] New client connected\n", LOG_TAG);
        
        // 为每个客户端启动一个线程处理
        int *client_fd = malloc(sizeof(int));
        *client_fd = client_sock;
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client_thread, client_fd);
        pthread_detach(tid); // 线程结束后自动回收
    }
    return NULL;
}

int main(int argc, char **argv) {
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    printf("[%s] Starting...\n", LOG_TAG);

    // 1. 初始化 IPC
    if (init_shm() != 0) {
        return -1;
    }

    // 2. 初始化硬件
    if (spi_init() != 0) {
        fprintf(stderr, "[%s] SPI init failed\n", LOG_TAG);
        return -1;
    }
    panel_init(); // JBD013 初始化
    printf("[%s] Hardware initialized\n", LOG_TAG);
    
    // 3. 初始化 GPIO Hub 客户端 (省电唤醒)
    ai_gpio_hub_client_create(&gpio_hub_client);
    if (ai_gpio_hub_client_connect(&gpio_hub_client) == 0) {
        int gpios[] = {GPIO_KEY_0, GPIO_KEY_1, GPIO_KEY_75};
        if (ai_gpio_hub_client_subscribe_gpios(&gpio_hub_client, gpios, 3, 
                                                gpio_wakeup_callback, NULL) == 0) {
            printf("[%s] GPIO Hub connected, monitoring GPIO %d/%d/%d\n", 
                   LOG_TAG, GPIO_KEY_0, GPIO_KEY_1, GPIO_KEY_75);
        } else {
            printf("[%s] Warning: GPIO Hub subscribe failed\n", LOG_TAG);
        }
    } else {
        printf("[%s] Warning: GPIO Hub connect failed\n", LOG_TAG);
    }
    
    // 初始化活动时间
    last_activity_time = time(NULL);

    // 4. 启动 Socket 服务
    pthread_t tid;
    pthread_create(&tid, NULL, socket_server_thread, NULL);

    // 5. 主循环 - 省电检测
    while (keeping_running) {
        // 检查是否需要息屏
        if (!display_off && last_activity_time > 0) {
            if (time(NULL) - last_activity_time >= POWER_SAVE_TIMEOUT) {
                send_cmd(SPI_DISPLAY_DISABLE);
                send_cmd(SPI_SYNC);
                display_off = 1;
                printf("[%s] Display off (power save after %ds)\n", 
                       LOG_TAG, POWER_SAVE_TIMEOUT);
            }
        }
        sleep(1);
    }

    // 清理
    ai_gpio_hub_client_destroy(&gpio_hub_client);
    close(server_socket);
    unlink("/tmp/ai_display_service");
    munmap(shm_ptr, AI_DISPLAY_SHM_SIZE);
    close(shm_fd);
    shm_unlink(AI_DISPLAY_SHM_NAME);

    printf("[%s] Stopped\n", LOG_TAG);
    return 0;
}
