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
#include <time.h>
#include <ifaddrs.h>//is_wifi_connected专用
#include <netinet/in.h>      // 定义 sockaddr_in 结构体
#include <arpa/inet.h>       // 网络地址转换函数
#include <net/if.h>          // 定义 IFF_UP 和 IFF_RUNNING 标志

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

// 全局变量用于跟踪rk_mpi_ai_test进程
static pid_t rk_mpi_ai_test_pid = -1;

// 全局变量用于跟踪视频录制进程
static pid_t video_recording_pid = -1;

// 全局变量用于跟踪rkaiq_3A_server进程
static pid_t rkaiq_3a_server_pid = -1;

// 全局变量用于跟踪命令行选项
static int enable_3a = 0;

//识别长按


// 函数声明
static int start_ai_client();
static int stop_ai_client();
static int is_ai_client_running();
static void parse_command_line_args(int argc, char *argv[]);
static int start_rk_mpi_ai_test();
static int stop_rk_mpi_ai_test();
static int start_video_recording();
static int stop_video_recording();
static int start_rkaiq_3a_server();
static int stop_rkaiq_3a_server();

// 解析命令行参数
static void parse_command_line_args(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--Enable_3A") == 0) {
            enable_3a = 1;
            printf("已启用 --Enable_3A 选项\n");
        }
    }
}

// 获取wlan0的IP地址
char* get_wlan0_ip() {
    static char ip_str[INET_ADDRSTRLEN] = "0.0.0.0";
    struct ifaddrs *ifaddr, *ifa;
    
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return ip_str;
    }
    
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET &&
            strcmp(ifa->ifa_name, "wlan0") == 0) {
            
            // 检查网卡是否处于UP状态且正在运行
            if (!(ifa->ifa_flags & IFF_UP) || !(ifa->ifa_flags & IFF_RUNNING)) {
                continue;
            }
            
            struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
            if (sa->sin_addr.s_addr != 0) {
                inet_ntop(AF_INET, &(sa->sin_addr), ip_str, INET_ADDRSTRLEN);
                break;
            }
        } 
    }
    
    freeifaddrs(ifaddr);
    return ip_str;
}

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
            // 检查网卡是否处于UP状态且正在运行
            if (!(ifa->ifa_flags & IFF_UP) || !(ifa->ifa_flags & IFF_RUNNING)) {
                continue;
            }
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

void Start_wifi(){
    int Wifi_status = 0;
    system("insmod /oem/usr/ko/cfg80211.ko");
    system("insmod /oem/usr/ko/rtl8723ds.ko");
    system("wpa_supplicant -B -i wlan0 -c /etc/wpa_supplicant.conf");
    system("ifconfig wlan0 up");
    system("udhcpc -i wlan0");
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
        printf("无法打开direction文件:\n");
        return -1;
    }
    
    if (write(fd, direction, strlen(direction)) < 0) {
        printf("设置GPIO方向失败:\n");
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
    
    // 停止rk_mpi_ai_test进程
    if (rk_mpi_ai_test_pid > 0) {
        stop_rk_mpi_ai_test();
    }
    
    // 停止视频录制进程
    if (video_recording_pid > 0) {
        stop_video_recording();
    }

    // 停止rkaiq_3A_server进程
    if (rkaiq_3a_server_pid > 0) {
        stop_rkaiq_3a_server();
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
              "--server", "47.99.87.250", "--port", "7860",
              "--format", "stream", "--enable-streaming",
              "--playback-volume", "80","--enable-display", NULL);
        
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

// 检查rk_mpi_ai_test进程是否还在运行
static int is_rk_mpi_ai_test_running() {
    if (rk_mpi_ai_test_pid <= 0) {
        return 0;
    }

    if (kill(rk_mpi_ai_test_pid, 0) == 0) {
        return 1; // 进程存在
    } else {
        if (errno == ESRCH) {
            // 进程不存在
            rk_mpi_ai_test_pid = -1;
            return 0;
        }
        return 1; // 其他错误，假设进程存在
    }
}

// 检查GPIO是否已经初始化,实际没用
static int is_gpio_ready(int gpio_number) {
    char path[64];
    snprintf(path, sizeof(path), "%s/gpio%d/direction", GPIO_SYSFS_PATH, gpio_number);
    return access(path, F_OK) == 0;
}

void Submenu_manager(void){

    
}

// GPIO监控线程
static void* gpio_monitor_thread(void *arg) {
    GPIO_STATE *gpios = (GPIO_STATE *)arg;
    int MenuValue = 0;
    int EnhanceMenu = 0;
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

            //gpios[i].current_state == 1
            
            if (gpios[i].is_exported) {
                int state = read_gpio_state(gpios[i].number);
                if (state >= 0) {
                    gpios[i].current_state = state;
                    /*
                    if(EnhanceMenu){
                        time_count++;一开始有基于时间检测的方案 一开始是有基于时间检测去识别长按的 但是发现实际操作起来会导致用户操作混乱 干脆就只用两个按钮 
                    }
                    */
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
                                            send_to_display(message);

                                            system("LD_LIBRARY_PATH=/oem/usr/lib:/usr/lib:/lib /oem/usr/bin/rk_mpi_ao_test -i AItalk.wav --sound_card_name=hw:0,0 --device_ch=2 --device_rate=16000 --input_rate=22000 --input_ch=2");
                                        } else {
                                            snprintf(message, sizeof(message), "AI Client Start Failed");
                                            send_to_display(message);
                                        }
                                    } else {
                                        // 没有WiFi连接，显示No wifi
                                        snprintf(message, sizeof(message), "Wifi WaiTing");
                                        send_to_display(message);
                                    }
                                }
                                else if(MenuValue == 2){snprintf(message, sizeof(message), "Bright++");send_to_display(message);}
                                 else if(MenuValue == 3){
                                     system("v4l2-ctl -d /dev/v4l-subdev2 --set-ctrl=exposure=1300,analogue_gain=500");//设置增益
                                     //if (enable_3a) {
                                        printf("已经启动--Enable_3A\n");
                                        // 启动rkaiq_3A_server子进程
                                        if (start_rkaiq_3a_server() == 0) {
                                            printf("rkaiq_3A_server started successfully\n");
                                        } else {
                                            printf("Failed to start rkaiq_3A_server\n");
                                        }
                                    //}
                                     //usleep(1000 * 50);//让3A稳定化
                                     system("v4l2-ctl  -d  /dev/video7   --set-fmt-video=width=1920,height=1080,pixelformat=NV12   --stream-mmap=3      --stream-to=/userdata/1.raw --stream-count=1    --stream-skip=4");//拍照
                                     //system("pgrep -x FFlaunch >/dev/null || FFlaunch &");
                                     snprintf(message, sizeof(message), "Finish-Photo");
                                     send_to_display(message);//发送信息
                                     char ffmpeg_cmd[512];
                                     snprintf(ffmpeg_cmd, sizeof(ffmpeg_cmd), "ffmpeg -y -f rawvideo -pixel_format nv12 -s 1920x1080 -i /userdata/1.raw -vf scale=1920:1080 -q:v 2 -frames:v 1 -f image2 /userdata/123.jpg");
                                     int ffmpeg_result = system(ffmpeg_cmd);//压缩，保存临时文件，容易被覆盖
                                     snprintf(ffmpeg_cmd, sizeof(ffmpeg_cmd), "mkdir -p /userdata/Rec && cp /userdata/123.jpg /userdata/Rec/P$(date +%%s).jpg");
                                     system(ffmpeg_cmd);//保存
                                     snprintf(message, sizeof(message), "FFmFinished");
                                     send_to_display(message);
                                 }
                                else if(MenuValue == 4){
                                    snprintf(message, sizeof(message), "VideoRecing");
                                    send_to_display(message);
                                    system("v4l2-ctl -d /dev/v4l-subdev2 --set-ctrl=exposure=1300,analogue_gain=500");//设置增益
                                    system("rk_mpi_amix_test --control \"ADC ALC Left Volume\" --value \"31\"");//修改录音音量
                                    if (start_rkaiq_3a_server() == 0) {
                                        printf("rkaiq_3A_server started successfully\n");
                                    } else {
                                        printf("Failed to start rkaiq_3A_server\n");
                                    }
                                    usleep(1000 * 50);//让3A稳定化
                                    if (start_video_recording() == 0) {
                                        snprintf(message, sizeof(message), "Finish-Video");
                                        send_to_display(message);
                                    }
                                }
                                else if(MenuValue == 5){
                                    snprintf(message, sizeof(message), "TelePrompTerNextParagraph");send_to_display(message);
                                }
                                else if(MenuValue == 6){
                                    system("rk_mpi_amix_test --control \"ADC ALC Left Volume\" --value \"31\"");
                                    if(rk_mpi_ai_test_pid > 0) { stop_rk_mpi_ai_test(); }
                                    snprintf(message, sizeof(message), "RecorDeRworking");
                                    // 正在录音
                                    send_to_display(message);
                                    start_rk_mpi_ai_test();
                                }
                                else if(MenuValue == 7){//翻译导航显示图片和语音转文字和休眠和个性化大小字和姿态,先按下选定键进去子菜单，长按IOBDN退出子菜单
                                    //snprintf(message, sizeof(message), "----");
                                    //send_to_display(message);
                                    EnhanceMenu ++ ;
                                    if (EnhanceMenu == 1){
                                        snprintf(message, sizeof(message), "DisplayPhoto");
                                        send_to_display(message);
                                    }
                                    else if (EnhanceMenu == 2){
                                        snprintf(message, sizeof(message), "bd_ASR");
                                        send_to_display(message);
                                    }
                                    else if (EnhanceMenu == 3){
                                        snprintf(message, sizeof(message), "TranslatE");//翻译
                                        send_to_display(message);
                                    }
                                    else if (EnhanceMenu == 4){
                                        snprintf(message, sizeof(message), "NavigaT");//导航
                                        send_to_display(message);
                                    }
                                    else if (EnhanceMenu == 5){
                                        snprintf(message, sizeof(message), "SleeP");//睡眠
                                        send_to_display(message);
                                    }
                                    else if (EnhanceMenu == 6){
                                        snprintf(message, sizeof(message), "FontSize");//字体大小
                                        send_to_display(message);
                                    }
                                    else if (EnhanceMenu == 7){
                                        snprintf(message, sizeof(message), "QuiT");//退出
                                        send_to_display(message);
                                    }
                                    else if (EnhanceMenu == 8){
                                        snprintf(message, sizeof(message), "IMUtest");//姿态
                                        send_to_display(message);
                                    }
                                    else
                                    {EnhanceMenu=0;}
                                }
                                else if(MenuValue == 8){snprintf(message, sizeof(message), "MeMoDisplay");send_to_display(message);}
                            } else if (gpios[i].prev_state == 0 && gpios[i].current_state == 1) {
                                // LOW转HIGH
                                //printf("IOADN\n");  
                            }
                        } else if (gpios[i].number == 0) {  // IOB控制菜单值，按下就切换显示
                            if (gpios[i].prev_state == 1 && gpios[i].current_state == 0) {
                                // HIGH转LOW
                                if(EnhanceMenu == 0){
                                        MenuValue++;
                                        if(MenuValue == 1){
                                            snprintf(message, sizeof(message), "AiTalk");
                                            // 如果AI客户端正在运行，停止它
                                            send_to_display(message);
                                        }
                                        else if(MenuValue == 2){
                                            snprintf(message, sizeof(message), "Brightness");
                                            if (is_ai_client_running()) {
                                                stop_ai_client();//关闭AI客户端
                                                //printf("AI Client Stopped");
                                            }
                                            send_to_display(message);
                                        }
                                        else if(MenuValue == 4){snprintf(message, sizeof(message), "Record");send_to_display(message);}
                                        else if(MenuValue == 3){snprintf(message, sizeof(message), "CamerA");send_to_display(message);}
                                        else if(MenuValue == 5){
                                            snprintf(message, sizeof(message), "TelePrompTer");send_to_display(message);
                                            if(video_recording_pid > 0) {
                                                stop_video_recording();
                                                //snprintf(message, sizeof(message), "Video-Stop");
                                                //send_to_display(message);
                                            }
                                            // 关闭rkaiq_3A_server进程
                                            if (rkaiq_3a_server_pid > 0) {
                                                stop_rkaiq_3a_server();
                                            }
                                        }
                                        else if(MenuValue == 6){snprintf(message, sizeof(message), "RecorDeR");send_to_display(message);}
                                        //else if(MenuValue == 7){snprintf(message, sizeof(message), "PictureDisplay");send_to_display(message);}
                                        else if(MenuValue == 7){
                                            snprintf(message, sizeof(message), "MoRe");
                                            send_to_display(message);
                                            if(rk_mpi_ai_test_pid > 0) { 
                                                stop_rk_mpi_ai_test();  //录音结束
                                                snprintf(message, sizeof(message), "RecorDeR-End");
                                                send_to_display(message);
                                            }
                                        }//按到下一个时候关闭录音
                                        else if(MenuValue == 8){snprintf(message, sizeof(message), "MeMo");send_to_display(message);}
                                        //else if(MenuValue == 9){snprintf(message, sizeof(message), "HeadUPdisplay");send_to_display(message);}
                                        else{MenuValue = 0;}
                                    }
                                    else
                                    {
                                        if(EnhanceMenu == 1){snprintf(message, sizeof(message), "DisplayPhoto-ON");send_to_display(message);}
                                        //else if(EnhanceMenu == 2){snprintf(message, sizeof(message), "bd_ASR-ON");send_to_display(message);}
                                        else if(EnhanceMenu == 3){snprintf(message, sizeof(message), "TranslatE-ON");send_to_display(message);}
                                        else if(EnhanceMenu == 4){snprintf(message, sizeof(message), "NavigaT-ON");send_to_display(message);}
                                        else if(EnhanceMenu == 5){
                                            snprintf(message, sizeof(message), "3秒后进入休眠");
                                            send_to_display(message);
                                            usleep(3000000); // 
                                            snprintf(message, sizeof(message), "IntoSleep");
                                            send_to_display(message);
                                            usleep(300000); // 
                                            system("echo mem > /sys/power/state");
                                        }
                                        else if(EnhanceMenu == 6){snprintf(message, sizeof(message), "FontSize-ON");send_to_display(message);}
                                        else if(EnhanceMenu == 7){snprintf(message, sizeof(message), "QuiTed");send_to_display(message);EnhanceMenu = 0;}
                                        else if(EnhanceMenu == 8){snprintf(message, sizeof(message), "IMUtest-ON");send_to_display(message);}
                                        //else if(EnhanceMenu == 9){snprintf(message, sizeof(message), "DisplayPos");send_to_display(message);}
                                    }
                            } else if (gpios[i].prev_state == 0 && gpios[i].current_state == 1) {
                                 printf("IOBDN\n");
                            }
                        }
                        //send_to_display(message);
                        
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
        
        // 检测/tmp/phone是否存在
        if(access("/tmp/phone", F_OK) == 0) {
            unlink("/tmp/phone");
            if(access("/userdata/phone", F_OK) != 0) {
                send_to_display("有电话或处于音乐模式");
            }
        }
        
    }
    
    return NULL;
}

static int start_rk_mpi_ai_test() {
    if(rk_mpi_ai_test_pid > 0) stop_rk_mpi_ai_test();
    rk_mpi_ai_test_pid = fork();
    if(rk_mpi_ai_test_pid == 0) {
        // 子进程
        execlp("rk_mpi_ai_test", "rk_mpi_ai_test", "--sound_card_name=hw:0,0", "--device_rate=8000", "--device_ch=2", "--out_rate=8000", "--out_ch=2", "--output=/userdata/Rec", NULL);
        perror("Failed to start rk_mpi_ai_test");
        exit(1);
    } else if(rk_mpi_ai_test_pid > 0) {
        return 0;
    } else {
        perror("Failed to fork rk_mpi_ai_test process");
        rk_mpi_ai_test_pid = -1;
        return -1;
    }
}
static int stop_rk_mpi_ai_test() {
    if(rk_mpi_ai_test_pid <= 0) return 0;
    printf("-------------");
    printf("------%d-------",rk_mpi_ai_test_pid);
    kill(rk_mpi_ai_test_pid, SIGINT);

    int status, wait_count=0;
    while(wait_count<30) {
        if (waitpid(rk_mpi_ai_test_pid, &status, WNOHANG) == rk_mpi_ai_test_pid) {
            rk_mpi_ai_test_pid = -1;
            return 0;
        }
        usleep(100000);
        wait_count++;
    }
    kill(rk_mpi_ai_test_pid, SIGKILL);
    waitpid(rk_mpi_ai_test_pid, &status, 0);
    rk_mpi_ai_test_pid = -1;
    printf("+++++++++++");
    return 0;
}

static int start_video_recording() {
    if(video_recording_pid > 0) stop_video_recording();
    video_recording_pid = fork();
    if(video_recording_pid == 0) {
        // 子进程
        char cmd[256];
        snprintf(cmd, sizeof(cmd),
                 "ts=$(date +%%s); "
                 "simple_vi_bind_venc -c 81000 "
                 "-o /userdata/Rec/out_${ts}.h264 "
                 "--sound_card_name=hw:0,0 "
                 "--device_rate=8000 --device_ch=2 "
                 "--out_rate=8000 --out_ch=2 "
                 "--output=/userdata/Rec/out_${ts}.pcm");
        execl("/bin/sh", "sh", "-c", cmd, NULL);
        perror("Failed to start video recording");
        exit(1);
    } else if(video_recording_pid > 0) {
        return 0;
    } else {
        perror("Failed to fork video recording process");
        video_recording_pid = -1;
        return -1;
    }
}

static int stop_video_recording() {
    if(video_recording_pid <= 0) return 0;
    printf("Stopping video recording process (PID: %d)\n", video_recording_pid);
    kill(video_recording_pid, SIGINT);

    int status, wait_count=0;
    while(wait_count<30) {
        if (waitpid(video_recording_pid, &status, WNOHANG) == video_recording_pid) {
            video_recording_pid = -1;
            return 0;
        }
        usleep(100000);
        wait_count++;
    }
    kill(video_recording_pid, SIGKILL);
    waitpid(video_recording_pid, &status, 0);
    video_recording_pid = -1;
    return 0;
}

static int start_rkaiq_3a_server() {
    // 如果已经有进程在运行，先终止它
    if (rkaiq_3a_server_pid > 0) {
        //stop_rkaiq_3a_server();
        return 0;
    }
    
    rkaiq_3a_server_pid = fork();
    if (rkaiq_3a_server_pid == 0) {
        // 子进程：设置环境变量
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
        
        // 执行rkaiq_3A_server
        execl("/oem/usr/bin/rkaiq_3A_server", "rkaiq_3A_server", NULL);
        
        // 如果/oem/usr/bin路径失败，尝试/usr/bin路径
        execl("/usr/bin/rkaiq_3A_server", "rkaiq_3A_server", NULL);
        
        // 如果execl失败，输出错误并退出
        perror("Failed to start rkaiq_3A_server");
        exit(1);
    } else if (rkaiq_3a_server_pid > 0) {
        // 父进程：记录子进程PID
        printf("rkaiq_3A_server started with PID: %d\n", rkaiq_3a_server_pid);
        return 0;
    } else {
        // fork失败
        perror("Failed to fork rkaiq_3A_server process");
        rkaiq_3a_server_pid = -1;
        return -1;
    }
}

static int stop_rkaiq_3a_server() {
    if (rkaiq_3a_server_pid <= 0) {
        printf("No rkaiq_3A_server process to stop\n");
        return 0;
    }
    
    printf("Stopping rkaiq_3A_server process (PID: %d)\n", rkaiq_3a_server_pid);
    
    // 发送SIGTERM信号优雅终止
    if (kill(rkaiq_3a_server_pid, SIGTERM) == 0) {
        // 等待进程结束，最多等待3秒
        int status;
        int wait_count = 0;
        while (wait_count < 30) { // 30 * 100ms = 3秒
            if (waitpid(rkaiq_3a_server_pid, &status, WNOHANG) == rkaiq_3a_server_pid) {
                printf("rkaiq_3A_server process terminated gracefully\n");
                rkaiq_3a_server_pid = -1;
                return 0;
            }
            usleep(100000); // 等待100ms
            wait_count++;
        }
        
        // 如果优雅终止失败，强制杀死进程
        printf("Force killing rkaiq_3A_server process\n");
        if (kill(rkaiq_3a_server_pid, SIGKILL) == 0) {
            waitpid(rkaiq_3a_server_pid, &status, 0);
            printf("rkaiq_3A_server process killed\n");
            rkaiq_3a_server_pid = -1;
            return 0;
        }
    }
    
    perror("Failed to stop rkaiq_3A_server process");
    return -1;
}

int main(int argc, char *argv[]) {
    // 解析命令行参数
    parse_command_line_args(argc, argv);

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
    //send_to_display("GPIO Monitor Started");
    
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