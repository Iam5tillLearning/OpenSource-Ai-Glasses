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

#include "jbd013_api.h"
#include "hal_driver.h"
#include "../../SDK/ai_glass_sdk/include/ai_display.h"

#define LOG_TAG "DisplayService"

static volatile int keeping_running = 1;
static int shm_fd = -1;
static ai_display_shm_t *shm_ptr = NULL;
static int server_socket = -1;

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
            uint8_t *frame_data = NULL;
            if (msg.slot_index == 0) {
                frame_data = shm_ptr->framebuffer_slot_0;
            } else if (msg.slot_index == 1) {
                frame_data = shm_ptr->framebuffer_slot_1;
            }

            if (frame_data) {
                printf("[%s] Frame commit (Slot %d)\n", LOG_TAG, msg.slot_index);
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

    // 3. 启动 Socket 服务
    // 为简单起见，直接在主线程跑，或者开个线程
    pthread_t tid;
    pthread_create(&tid, NULL, socket_server_thread, NULL);

    while (keeping_running) {
        sleep(1);
    }

    // 清理
    close(server_socket);
    unlink("/tmp/ai_display_service");
    munmap(shm_ptr, AI_DISPLAY_SHM_SIZE);
    close(shm_fd);
    shm_unlink(AI_DISPLAY_SHM_NAME);

    printf("[%s] Stopped\n", LOG_TAG);
    return 0;
}
