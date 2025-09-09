#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/wait.h>
#include <ifaddrs.h>//is_wifi_connected专用
#include <netinet/in.h>      // 定义 sockaddr_in 结构体
#include <arpa/inet.h>       // 网络地址转换函数

#define GPIO_SYSFS_PATH "/sys/class/gpio"
#define GPIO_DEBUG_PATH "/sys/kernel/debug/gpio"
#define POLL_INTERVAL_MS 50  // 检测间隔50ms

// IPC相关定义
// 应该使用与display程序完全相同的定义
#define SHM_NAME "/display_shm"       // 共享内存名称 - 与display程序一致
#define SEM_NAME "/display_sem"       // 信号量名称 - 与display程序一致
#define BUFFER_SIZE 128               // 缓冲区大小 - 与display程序一致

// GPIO状态结构体
typedef struct {
    int number;
    int current_state;
    int prev_state;
    int is_exported;
} GPIO_STATE;

// 共享内存和信号量指针
char *shared_memory = NULL;
sem_t *semaphore = NULL;

// 全局变量用于跟踪ai_client_socket进程
static pid_t ai_client_pid = -1;

// 函数声明
static int start_ai_client();
static int stop_ai_client();
static int is_ai_client_running();

// 检查wlan0是否有IP地址
int is_wifi_connected() {
    struct ifaddrs *ifaddr, *ifa;
    int connected = 0;
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return 0;
    }
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET &&
            strcmp(ifa->ifa_name, "wlan0") == 0) {
            struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
            if (sa->sin_addr.s_addr != 0) {
                connected = 1;
                break;
            }
        }
    }
    freeifaddrs(ifaddr);
    return connected;
}

// 导出GPIO
static int export_gpio(int gpio_number) {
    char path[64];
    int fd, len;
    char buf[16];
    
    snprintf(path, sizeof(path), "%s/export", GPIO_SYSFS_PATH);
    fd = open(path, O_WRONLY);
    if (fd < 0) {
        perror("无法打开export文件");
        return -1;
    }
    
    len = snprintf(buf, sizeof(buf), "%d", gpio_number);
    if (write(fd, buf, len) < 0) {
        if (errno == EBUSY) {
            printf("GPIO-%d 已经导出\n", gpio_number);
            close(fd);
            return 0;
        }
        perror("导出GPIO失败");
        close(fd);
        return -1;
    }
    
    close(fd);
    
    // 等待GPIO目录创建
    usleep(100000); // 100ms延迟
    return 0;
}

// 设置GPIO方向
static int set_gpio_direction(int gpio_number, const char *direction) {
    char path[64];
    int fd;
    
    snprintf(path, sizeof(path), "%s/gpio%d/direction", GPIO_SYSFS_PATH, gpio_number);
    fd = open(path, O_WRONLY);
    if (fd < 0) {
        // 使用fprintf到stderr，避免输出到其他地方
        fprintf(stderr, "无法打开direction文件: %s\n", path);
        return -1;
    }
    
    if (write(fd, direction, strlen(direction)) < 0) {
        fprintf(stderr, "设置GPIO方向失败: %s\n", path);
        close(fd);
        return -1;
    }
    
    close(fd);
    return 0;
}

// 读取GPIO状态
static int read_gpio_state(int gpio_number) {
    char path[64];
    char value;
    int fd;
    
    // 先尝试从sysfs读取
    snprintf(path, sizeof(path), "%s/gpio%d/value", GPIO_SYSFS_PATH, gpio_number);
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        if (read(fd, &value, 1) == 1) {
            close(fd);
            return (value == '1') ? 1 : 0;
        }
        close(fd);
    }
    
    // 如果sysfs读取失败，尝试从debug接口读取
    FILE *fp = fopen(GPIO_DEBUG_PATH, "r");
    if (fp) {
        char line[256];
        char gpio_name[32];
        snprintf(gpio_name, sizeof(gpio_name), "gpio-%d", gpio_number);
        
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, gpio_name)) {
                char *pos = strstr(line, " in ");
                if (!pos) pos = strstr(line, " out ");
                if (pos) {
                    pos += 4;
                    while (*pos == ' ') pos++;
                    if (strncmp(pos, "hi", 2) == 0) {
                        fclose(fp);
                        return 1;
                    } else if (strncmp(pos, "lo", 2) == 0) {
                        fclose(fp);
                        return 0;
                    }
                }
            }
        }
        fclose(fp);
    }
    
    return -1;
}

// 初始化IPC通信
static int init_ipc() {
    int retries = 5;
    int shm_fd;
    
    while (retries-- > 0) {
        // 先清理可能残留的资源
        //shm_unlink(SHM_NAME);
        //sem_unlink(SEM_NAME);
        
        // 创建共享内存
        shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
        if (shm_fd == -1) {
            perror("shm_open failed");
            usleep(500000); // 等待500ms后重试
            continue;
        }
        
        if (ftruncate(shm_fd, BUFFER_SIZE) == -1) {
            perror("ftruncate failed");
            close(shm_fd);
            usleep(500000);
            continue;
        }
        
        // 修改这一行，添加显式类型转换
        shared_memory = (char*)mmap(0, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (shared_memory == MAP_FAILED) {
            perror("mmap failed");
            close(shm_fd);
            usleep(500000);
            continue;
        }
        
        close(shm_fd);
        
        // 创建信号量
        semaphore = sem_open(SEM_NAME, O_CREAT, 0666, 0);
        if (semaphore == SEM_FAILED) {
            perror("sem_open failed");
            munmap(shared_memory, BUFFER_SIZE);
            usleep(500000);
            continue;
        }
        
        // 初始化共享内存
        memset(shared_memory, 0, BUFFER_SIZE);
        printf("IPC initialized successfully on attempt %d\n", 5 - retries);
        return 0;
    }
    
    printf("Failed to initialize IPC after 5 attempts\n");
    return -1;
}

// 清理IPC资源
static void cleanup_ipc() {
    // 停止AI客户端进程
    if (ai_client_pid > 0) {
        stop_ai_client();
    }
    
    if (shared_memory) {
        munmap(shared_memory, BUFFER_SIZE);
        shm_unlink(SHM_NAME);
    }
    if (semaphore) {
        sem_close(semaphore);
        sem_unlink(SEM_NAME);
    }
}

// 发送消息给display
static void send_to_display(const char *message) {
    if (!shared_memory || !semaphore) {
        printf("IPC not initialized, cannot send message\n");
        return;
    }
    
    // 复制消息到共享内存
    strncpy(shared_memory, message, BUFFER_SIZE - 1);
    shared_memory[BUFFER_SIZE - 1] = '\0'; // 确保字符串终止
    
    // 通知display有新消息
    if (sem_post(semaphore) == -1) {
        perror("sem_post failed");
    } else {
        printf("Sent message to display: %s\n", message);
    }
}

// 启动ai_client_socket进程
static int start_ai_client() {
    // 如果已经有进程在运行，先终止它
    if (ai_client_pid > 0) {
        stop_ai_client();
    }
    
    ai_client_pid = fork();
    if (ai_client_pid == 0) {
        // 子进程：设置环境变量
        
        // 设置LD_LIBRARY_PATH包含oem库路径
        const char* current_ld_path = getenv("LD_LIBRARY_PATH");
        char new_ld_path[1024];
        if (current_ld_path) {
            snprintf(new_ld_path, sizeof(new_ld_path), 
                    "/oem/usr/lib:/usr/lib:/lib:%s", 
                    current_ld_path);
        } else {
            snprintf(new_ld_path, sizeof(new_ld_path), 
                    "/oem/usr/lib:/usr/lib:/lib");
        }
        setenv("LD_LIBRARY_PATH", new_ld_path, 1);
        
        // 设置PATH包含oem工具路径
        const char* current_path = getenv("PATH");
        char new_path[1024];
        if (current_path) {
            snprintf(new_path, sizeof(new_path), 
                    "/oem/usr/bin:/usr/bin:/bin:%s", 
                    current_path);
        } else {
            snprintf(new_path, sizeof(new_path), 
                    "/oem/usr/bin:/usr/bin:/bin");
        }
        setenv("PATH", new_path, 1);
        
        // 执行ai_client_socket
        execl("/usr/bin/ai_client_socket", "ai_client_socket",
              "--enable-gpio", "--enable-upload", 
              "--server", "your_server_ip", "--port", "your_port",
              "--format", "stream", "--enable-streaming",
              "--playback-volume", "30","--enable-display", NULL);
        
        // 如果execl失败，输出错误并退出
        perror("Failed to start ai_client_socket");
        exit(1);
    } else if (ai_client_pid > 0) {
        // 父进程：记录子进程PID
        //printf("AI client started with PID: %d\n", ai_client_pid);
        return 0;
    } else {
        // fork失败
        perror("Failed to fork ai_client process");
        ai_client_pid = -1;
        return -1;
    }
}

// 停止ai_client_socket进程
static int stop_ai_client() {
    if (ai_client_pid <= 0) {
        printf("No AI client process to stop\n");
        return 0;
    }
    
    printf("Stopping AI client process (PID: %d)\n", ai_client_pid);
    
    // 发送SIGTERM信号优雅终止
    if (kill(ai_client_pid, SIGTERM) == 0) {
        // 等待进程结束，最多等待3秒
        int status;
        int wait_count = 0;
        while (wait_count < 30) { // 30 * 100ms = 3秒
            if (waitpid(ai_client_pid, &status, WNOHANG) == ai_client_pid) {
                printf("AI client process terminated gracefully\n");
                ai_client_pid = -1;
                return 0;
            }
            usleep(100000); // 等待100ms
            wait_count++;
        }
        
        // 如果优雅终止失败，强制杀死进程
        printf("Force killing AI client process\n");
        if (kill(ai_client_pid, SIGKILL) == 0) {
            waitpid(ai_client_pid, &status, 0);
            printf("AI client process killed\n");
            ai_client_pid = -1;
            return 0;
        }
    }
    
    perror("Failed to stop AI client process");
    return -1;
}

// 检查ai_client_socket进程是否还在运行
static int is_ai_client_running() {
    if (ai_client_pid <= 0) {
        return 0;
    }
    
    // 使用kill(pid, 0)检查进程是否存在
    if (kill(ai_client_pid, 0) == 0) {
        return 1; // 进程存在
    } else {
        if (errno == ESRCH) {
            // 进程不存在
            ai_client_pid = -1;
            return 0;
        }
        return 1; // 其他错误，假设进程存在
    }
}

// 检查GPIO是否已经初始化
static int is_gpio_ready(int gpio_number) {
    char path[64];
    snprintf(path, sizeof(path), "%s/gpio%d/direction", GPIO_SYSFS_PATH, gpio_number);
    return access(path, F_OK) == 0;
}

// GPIO监控线程
static void* gpio_monitor_thread(void *arg) {
    GPIO_STATE *gpios = (GPIO_STATE *)arg;
    int MenuValue = 0;
    int gpio75_monitoring = 1; // 控制GPIO75的监听状态

    // 初始化GPIO
    for (int i = 0; i < 2; i++) {
        int retry_count = 0;
        while (retry_count < 3) {
            if (export_gpio(gpios[i].number) == 0) {
                if (set_gpio_direction(gpios[i].number, "in") == 0) {
                    gpios[i].is_exported = 1;
                    gpios[i].current_state = gpios[i].prev_state = read_gpio_state(gpios[i].number);
                    printf("成功初始化GPIO-%d, 初始状态: %s\n", 
                          gpios[i].number, 
                          gpios[i].current_state == 1 ? "高电平" : "低电平");
                    break;  // 成功则跳出重试循环
                }
            }
            retry_count++;
            if (retry_count < 3) {
                printf("GPIO-%d 初始化失败，重试 %d/3...\n", gpios[i].number, retry_count + 1);
                usleep(100000); // 等待100ms后重试
            }
        }
        
        if (retry_count >= 3) {
            printf("GPIO-%d 初始化失败，跳过\n", gpios[i].number);
        }
    }
    
    while (1) {
        // 检查ai_client_socket状态，动态调整GPIO75的监听
        if (is_ai_client_running()) {
            if (gpio75_monitoring) {
                printf("AI client正在运行，暂停监听GPIO75\n");
                gpio75_monitoring = 0;
            }
        } else {
            if (!gpio75_monitoring) {
                printf("AI client已停止，恢复监听GPIO75\n");
                gpio75_monitoring = 1;
            }
        }
        
        for (int i = 0; i < 2; i++) {
            // 如果是GPIO75且当前不监听，则跳过
            if (gpios[i].number == 75 && !gpio75_monitoring) {
                continue;
            }
            
            if (gpios[i].is_exported) {
                int state = read_gpio_state(gpios[i].number);
                if (state >= 0) {
                    gpios[i].current_state = state;
                    
                    // 检测状态变化
                    if (gpios[i].prev_state != gpios[i].current_state) {
                        char message[BUFFER_SIZE];
                        
                        // 发送状态变化消息给display
                        // 根据GPIO编号和状态变化方向发送不同消息
                        if (gpios[i].number == 75) {  // IOA控制是否运行对应应用
                            if (gpios[i].prev_state == 1 && gpios[i].current_state == 0) {
                                // HIGH转LOW
                                if(MenuValue == 1){
                                    // 检查WiFi连接状态
                                    if (is_wifi_connected()) {
                                        // 有WiFi连接，正常启动AI客户端
                                        if (start_ai_client() == 0) {
                                            snprintf(message, sizeof(message), "AI Client Started");
                                            // 播放提示音，使用完整路径和环境变量
                                            system("LD_LIBRARY_PATH=/oem/usr/lib:/usr/lib:/lib /oem/usr/bin/rk_mpi_ao_test -i AItalk.wav --sound_card_name=hw:0,0 --device_ch=2 --device_rate=16000 --input_rate=22000 --input_ch=2");
                                        } else {
                                            snprintf(message, sizeof(message), "AI Client Start Failed");
                                        }
                                    } else {
                                        // 没有WiFi连接，显示No wifi
                                        snprintf(message, sizeof(message), "No wifi");
                                        printf("WiFi未连接，无法启动AI客户端\n");
                                    }
                                }
                                else if(MenuValue == 2){snprintf(message, sizeof(message), "Bright++");}
                                else if(MenuValue == 5){
                                    snprintf(message, sizeof(message), "Wait APP connect");
                                    send_to_display(message);
                                    system("/oem/usr/expect.sh");
                                    system("FFlaunch");//图片压缩控制器
                                }

                                
                            } else if (gpios[i].prev_state == 0 && gpios[i].current_state == 1) {
                                // LOW转HIGH
                                //snprintf(message, sizeof(message), "IOADN");
                            }
                        } else if (gpios[i].number == 0) {  // IOB控制菜单值
                            if (gpios[i].prev_state == 1 && gpios[i].current_state == 0) {
                                // HIGH转LOW
                                MenuValue++;
                                if(MenuValue == 1){
                                    snprintf(message, sizeof(message), "Ai");
                                    // 如果AI客户端正在运行，停止它

                                }
                                else if(MenuValue == 2){
                                    snprintf(message, sizeof(message), "Brightness");
                                    if (is_ai_client_running()) {
                                        stop_ai_client();
                                        //printf("AI Client Stopped");
                                    }
                                }
                                else if(MenuValue == 3){snprintf(message, sizeof(message), "Sleep");}
                                else if(MenuValue == 4){snprintf(message, sizeof(message), "Record");}
                                else if(MenuValue == 5){snprintf(message, sizeof(message), "CamerA");}
                                else{MenuValue = 0;}
                            } else if (gpios[i].prev_state == 0 && gpios[i].current_state == 1) {
                                // LOW转HIGH
                                //snprintf(message, sizeof(message), "IOBDN");
                            }
                        }
                        send_to_display(message);
                        
                        gpios[i].prev_state = gpios[i].current_state;
                    }
                } else {
                    printf("警告: 无法读取GPIO-%d的状态\n", gpios[i].number);
                }
            } else {
                printf("警告: GPIO-%d 未成功导出\n", gpios[i].number);
            }
        }
        usleep(POLL_INTERVAL_MS * 1000);
    }
    
    return NULL;
}

int main() {

    pthread_t gpio_thread;
    GPIO_STATE gpios[2] = {
        {75, -1, -1, 0},  // GPIO 75
        {0, -1, -1, 0}    // GPIO 0
    };
    
    printf("GPIO状态检测程序启动\n");
    printf("检测GPIO-%d和GPIO-%d的状态\n", gpios[0].number, gpios[1].number);
    printf("按Ctrl+C退出程序\n\n");
    usleep(1000*1000);//1s延迟为了共享内存
    
    // 初始化IPC通信
    if (init_ipc() != 0) {
        fprintf(stderr, "IPC初始化失败，程序退出\n");
        return 1;
    }
    
    // 发送初始消息测试
    send_to_display("GPIO Monitor Started");
    
    // 创建GPIO监控线程
    if (pthread_create(&gpio_thread, NULL, gpio_monitor_thread, gpios) != 0) {
        perror("创建GPIO监控线程失败");
        cleanup_ipc();
        return 1;
    }
    
    // 主线程等待
    pthread_join(gpio_thread, NULL);
    
    // 清理资源
    cleanup_ipc();
    
    return 0;
}