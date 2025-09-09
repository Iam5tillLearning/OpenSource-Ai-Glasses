#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>  // 添加bool类型支持
#include <fcntl.h>
#include <unistd.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <string.h>
#include <strings.h>  // 添加strcasecmp支持
#include <math.h>
#include <pthread.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <signal.h>
#include <errno.h>  // 添加缺失的头文件
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>  // 添加文件状态检查
#include <sys/time.h>  // 添加时间测量支持
#include <jbd013_api.h>
#include <hal_driver.h>
#include <font.h>
#include <sys/wait.h>
#include "ui.h"
#define SPI_DEVICE_PATH "/dev/spidev0.0"
#define SHM_NAME "/display_shm"       // 共享内存名称
#define SEM_NAME "/display_sem"       // 信号量名称
#define BUFFER_SIZE 128               // 消息缓冲区大小
// 在main.c的全局变量区域添加
extern lv_obj_t *ui_Label2;  // 声明外部变量，指向"微笑"标签
extern const lv_img_dsc_t camera;  // 声明camera图标
// 声明录像机相关UI对象
extern lv_obj_t *ui_VideoRecorderRect;  // 录像机矩形
extern lv_obj_t *ui_VideoLine1;         // 录像机线条1
extern lv_obj_t *ui_VideoLine2;         // 录像机线条2  
extern lv_obj_t *ui_VideoLine3;         // 录像机线条3
volatile bool hide_smile_flag = false;  // 线程安全标志位
static int image_saved = 0;
int spi_file;
#define DISP_BUF_SIZE   (640 * 480) 
static lv_disp_drv_t disp_drv;        // 显示驱动
static lv_disp_draw_buf_t draw_buf;    // 显示绘制缓冲区
static uint8_t disp_buf[DISP_BUF_SIZE]; 

int display_inited = 0;  // 显示状态标记
int running = 1;         // 程序运行标记
sem_t *semaphore;        // 信号量指针
char *shared_memory;     // 共享内存指针

// 省电功能相关变量
volatile bool display_power_save_mode = false;  // 省电模式标志
volatile time_t last_activity_time = 0;         // 最后活动时间
volatile time_t power_save_start_time = 0;      // 省电模式开始时间
#define POWER_SAVE_TIMEOUT 30                   // 30秒后进入省电模式

// 添加camera图像对象全局变量
static lv_obj_t *camera_img_obj = NULL;  // camera图像对象指针
static bool camera_visible = false;       // camera图标显示状态

// 添加录像机对象全局变量
static bool recorder_visible = false;     // 录像机图标显示状态

// 函数声明
int spi_init();
int setup();
void loop();
uint8_t* load_bmp_image_fast(const char* filename, uint16_t* width, uint16_t* height);
uint8_t* load_raw_image(const char* filename, uint16_t* width, uint16_t* height);
uint8_t* load_image(const char* filename, uint16_t* width, uint16_t* height);
int load_and_display_image(const char* filename);
uint8_t rgb_to_4bit_fast(uint8_t r, uint8_t g, uint8_t b);
uint8_t rgb_to_4bit(uint8_t r, uint8_t g, uint8_t b);
void bit4_to_rgb(uint8_t gray4, uint8_t* r, uint8_t* g, uint8_t* b);
int save_4bit_to_bmp(const char* filename, uint8_t* image_data, uint16_t width, uint16_t height);
int display_rgb_image(uint8_t* rgb_data, uint16_t width, uint16_t height);
int display_image_fast(uint8_t* image_data, uint16_t width, uint16_t height);
void display_checkerboard_instant(uint16_t square_size);
void display_gradient_instant(void);
void display_circles_instant(void);
void demo_image_display_optimized(void);
// 缩放动画相关
struct zoom_animation_t;
uint8_t* scale_image_nearest(uint8_t* src_data, uint16_t src_width, uint16_t src_height, uint16_t dst_width, uint16_t dst_height);
int display_bmp_zoom_animation(const char* filename, struct zoom_animation_t* anim_params);
void demo_zoom_effects(const char* filename);
// 序列图播放相关
struct sequence_animation_t;
int load_image_sequence(const char* directory, char*** filenames, int* count);
int play_image_sequence(char** filenames, int count, struct sequence_animation_t* anim_params);
void demo_image_sequence(const char* directory);
void free_image_sequence(char** filenames, int count);
void cleanup(int signum);
void* display_update_thread(void* arg);

// SPI初始化（保持不变）
int spi_init() {
    if ((spi_file = open(SPI_DEVICE_PATH, O_RDWR)) < 0) {
        perror("Failed to open SPI device");
        return -1;
    }

    uint8_t mode = SPI_MODE_0;
    if (ioctl(spi_file, SPI_IOC_WR_MODE, &mode) < 0) {
        perror("Failed to set SPI mode");
        close(spi_file);
        return -1;
    }

    uint8_t bits = 8;
    if (ioctl(spi_file, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
        perror("Failed to set SPI bits per word");
        close(spi_file);
        return -1;
    }

    uint32_t speed_hz = 19200000;
    if (ioctl(spi_file, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz) < 0) {
        perror("Failed to set SPI speed");
        close(spi_file);
        return -1;
    }

    int msb_first = 0;
    if (ioctl(spi_file, SPI_IOC_WR_LSB_FIRST, &msb_first) < 0) {
        perror("Failed to set SPI bit order");
        close(spi_file);
        return -1;
    }

    return 0;
}

// 显示更新线程：监听共享内存变化
void* display_update_thread(void* arg) {
    char last_message[BUFFER_SIZE] = {0};
    uint8_t Brightness_display = 30;
    
    while (running) {
        // 等待信号量（有新消息）
        if (sem_wait(semaphore) == -1) {
            if (errno == EINTR) continue;  // 处理中断信号
            perror("sem_wait failed");
            break;
        }
        
        // 检查是否有新消息
        if (strcmp(shared_memory, last_message) != 0) {
            strncpy(last_message, shared_memory, BUFFER_SIZE - 1);
            
            // 处理"init"指令
            if (strcmp(shared_memory, "GPIOA") == 0 && !display_inited) {
                display_inited = 1;
                printf("Display updated to: Inited\n");
            } 
            // 处理"CamerA"指令 - 居中显示camera图标
            else if (strcmp(shared_memory, "CamerA") == 0) {
                // 如果有新内容显示，重新开启显示并更新活动时间
                if (display_power_save_mode) {
                    printf("🔄 检测到CamerA指令，重新开启显示\n");
                    send_cmd(SPI_DISPLAY_ENABLE);
                    send_cmd(SPI_SYNC);
                    usleep(1 * 1000);
                    display_power_save_mode = false;
                }
                
                // 更新最后活动时间
                last_activity_time = time(NULL);
                
                hide_smile_flag = true;  // 设置标志位，隐藏微笑标签
                
                // 检查camera图像对象是否已存在
                if (camera_img_obj == NULL) {
                                    // 首次创建camera图像对象
                camera_img_obj = lv_img_create(lv_scr_act());
                lv_img_set_src(camera_img_obj, &camera);
                lv_obj_set_align(camera_img_obj, LV_ALIGN_CENTER);
                lv_obj_set_y(camera_img_obj, -80);  // 向上移动80像素
                    //printf("Display updated to: CamerA - 创建并显示camera图标\n");
                } else {
                    // 对象已存在，确保可见
                    lv_obj_clear_flag(camera_img_obj, LV_OBJ_FLAG_HIDDEN);
                    //printf("Display updated to: CamerA - 显示已存在的camera图标\n");
                }
                camera_visible = true;
            }
            // 处理"Record"指令 - 显示录像机图标
            else if (strcmp(shared_memory, "Record") == 0) {
                // 如果有新内容显示，重新开启显示并更新活动时间
                if (display_power_save_mode) {
                    printf("🔄 检测到Record指令，重新开启显示\n");
                    send_cmd(SPI_DISPLAY_ENABLE);
                    send_cmd(SPI_SYNC);
                    usleep(1 * 1000);
                    display_power_save_mode = false;
                }
                
                // 更新最后活动时间
                last_activity_time = time(NULL);
                
                hide_smile_flag = true;  // 设置标志位，隐藏微笑标签
                
                // 显示录像机图标（矩形和三条线）
                if (ui_VideoRecorderRect != NULL) {
                    lv_obj_clear_flag(ui_VideoRecorderRect, LV_OBJ_FLAG_HIDDEN);
                }
                if (ui_VideoLine1 != NULL) {
                    lv_obj_clear_flag(ui_VideoLine1, LV_OBJ_FLAG_HIDDEN);
                }
                if (ui_VideoLine2 != NULL) {
                    lv_obj_clear_flag(ui_VideoLine2, LV_OBJ_FLAG_HIDDEN);
                }
                if (ui_VideoLine3 != NULL) {
                    lv_obj_clear_flag(ui_VideoLine3, LV_OBJ_FLAG_HIDDEN);
                }
                recorder_visible = true;
                
                printf("Display updated to: Record - 显示录像机图标\n");
            }
            // 处理其他显示内容
            else if (strcmp(shared_memory, "init") != 0) {
                // 如果有新内容显示，重新开启显示并更新活动时间
                if (display_power_save_mode) {
                    printf("🔄 检测到新内容，重新开启显示\n");
                    send_cmd(SPI_DISPLAY_ENABLE);
                    send_cmd(SPI_SYNC);
                    usleep(1 * 1000);
                    display_power_save_mode = false;
                }
                
                // 更新最后活动时间
                last_activity_time = time(NULL);
                
                hide_smile_flag = true;  // 设置标志位，不直接操作UI
                printf("Display updated to: %s\n", shared_memory);
                
                // 检测是否包含"Ai"字符串，控制对话气泡显示
                if (strstr(shared_memory, "Ai") != NULL) {
                    // 显示对话气泡
                    if (ui_SpeechBubble != NULL) {
                        lv_obj_clear_flag(ui_SpeechBubble, LV_OBJ_FLAG_HIDDEN);
                        //printf("�� 检测到Ai，显示对话气泡\n");
                    }
                    if (ui_SpeechTail1 != NULL) {
                        lv_obj_clear_flag(ui_SpeechTail1, LV_OBJ_FLAG_HIDDEN);
                    }
                    if (ui_SpeechTail2 != NULL) {
                        lv_obj_clear_flag(ui_SpeechTail2, LV_OBJ_FLAG_HIDDEN);
                    }
                } else {
                    // 隐藏对话气泡
                    if (ui_SpeechBubble != NULL) {
                        lv_obj_add_flag(ui_SpeechBubble, LV_OBJ_FLAG_HIDDEN);
                        //printf("🎯 未检测到Ai，隐藏对话气泡\n");
                    }
                    if (ui_SpeechTail1 != NULL) {
                        lv_obj_add_flag(ui_SpeechTail1, LV_OBJ_FLAG_HIDDEN);
                    }
                    if (ui_SpeechTail2 != NULL) {
                        lv_obj_add_flag(ui_SpeechTail2, LV_OBJ_FLAG_HIDDEN);
                    }
                }
                
                // 检测是否为"Brightness"指令，控制亮度调节元素显示
                if (strcmp(shared_memory, "Brightness") == 0) {
                    strncpy(last_message, "亮度设置", 12);
                    strncpy(shared_memory, "亮度设置", 12);
                    
                    // 显示亮度调节相关的5个元素（圆形和4条线）
                    if (ui_Circle != NULL) {
                        lv_obj_clear_flag(ui_Circle, LV_OBJ_FLAG_HIDDEN);
                    }
                    if (ui_UpperLine != NULL) {
                        lv_obj_clear_flag(ui_UpperLine, LV_OBJ_FLAG_HIDDEN);
                    }
                    if (ui_LowerLine != NULL) {
                        lv_obj_clear_flag(ui_LowerLine, LV_OBJ_FLAG_HIDDEN);
                    }
                    if (ui_LeftLine != NULL) {
                        lv_obj_clear_flag(ui_LeftLine, LV_OBJ_FLAG_HIDDEN);
                    }
                    if (ui_RightLine != NULL) {
                        lv_obj_clear_flag(ui_RightLine, LV_OBJ_FLAG_HIDDEN);
                    }
                } else {
                    // 隐藏亮度调节相关的5个元素（圆形和4条线）
                    if (ui_Circle != NULL) {
                        lv_obj_add_flag(ui_Circle, LV_OBJ_FLAG_HIDDEN);
                    }
                    if (ui_UpperLine != NULL) {
                        lv_obj_add_flag(ui_UpperLine, LV_OBJ_FLAG_HIDDEN);
                    }
                    if (ui_LowerLine != NULL) {
                        lv_obj_add_flag(ui_LowerLine, LV_OBJ_FLAG_HIDDEN);
                    }
                    if (ui_LeftLine != NULL) {
                        lv_obj_add_flag(ui_LeftLine, LV_OBJ_FLAG_HIDDEN);
                    }
                    if (ui_RightLine != NULL) {
                        lv_obj_add_flag(ui_RightLine, LV_OBJ_FLAG_HIDDEN);
                    }
                }
                
                // 隐藏camera图标（如果存在且可见）
                if (camera_img_obj != NULL && camera_visible) {
                    lv_obj_add_flag(camera_img_obj, LV_OBJ_FLAG_HIDDEN);
                    camera_visible = false;
                }
                
                // 隐藏录像机图标（如果存在且可见）
                if (recorder_visible) {
                    if (ui_VideoRecorderRect != NULL) {
                        lv_obj_add_flag(ui_VideoRecorderRect, LV_OBJ_FLAG_HIDDEN);
                    }
                    if (ui_VideoLine1 != NULL) {
                        lv_obj_add_flag(ui_VideoLine1, LV_OBJ_FLAG_HIDDEN);
                    }
                    if (ui_VideoLine2 != NULL) {
                        lv_obj_add_flag(ui_VideoLine2, LV_OBJ_FLAG_HIDDEN);
                    }
                    if (ui_VideoLine3 != NULL) {
                        lv_obj_add_flag(ui_VideoLine3, LV_OBJ_FLAG_HIDDEN);
                    }
                    recorder_visible = false;
                }
            }
            // 在display_update_thread函数中修改处理逻辑
            if (strcmp(shared_memory, "Bright++") == 0) {
                // 亮度调节也视为活动，重新开启显示
                if (display_power_save_mode) {
                    //printf("🔄 亮度调节，重新开启显示\n");
                    send_cmd(SPI_DISPLAY_ENABLE);
                    send_cmd(SPI_SYNC);
                    usleep(1 * 1000);
                    display_power_save_mode = false;
                }
                
                // 更新最后活动时间
                last_activity_time = time(NULL);
                
                Brightness_display = Brightness_display+10;
                if(Brightness_display > 63){Brightness_display = 0;}
                wr_cur_reg(Brightness_display);                          //设置电流寄存器
                send_cmd(SPI_SYNC);                     //同步设置
                hide_smile_flag = true;  // 设置标志位，不直接操作UI
                strncpy(last_message, "clean", 12);
                strncpy(shared_memory, "亮度修改", 12);
            }
        }
    }
    
    return NULL;
}

// 清理资源
void cleanup(int signum) {
    printf("\nCleaning up resources...\n");
    running = 0;
    
    // 释放共享内存和信号量
    if (shared_memory) {
        if (munmap(shared_memory, BUFFER_SIZE) == -1) {
            perror("munmap failed");
        }
    }
    
    if (semaphore) {
        sem_close(semaphore);
        sem_unlink(SEM_NAME);
    }
    
    shm_unlink(SHM_NAME);
    
    if (spi_file > 0) {
        close(spi_file);
    }
    
    exit(EXIT_SUCCESS);
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
int setup() {
    lv_init();          // 初始化LVGL图形库
    spi_init();         // 初始化SPI设备
    panel_init();       // 初始化面板

            // 设置信号处理
            signal(SIGINT, cleanup);
            signal(SIGTERM, cleanup);
        
            // 创建并初始化共享内存
            int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
            if (shm_fd == -1) {
                perror("shm_open failed");
                display_string_at(0,0,"shm-f");
                return -1;
            }
            
            if (ftruncate(shm_fd, BUFFER_SIZE) == -1) {
                perror("ftruncate failed");
                close(shm_fd);
                display_string_at(0,0,"ftrun");
                return -1;
            }
            
            shared_memory = mmap(0, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
            if (shared_memory == MAP_FAILED) {
                perror("mmap failed");
                close(shm_fd);
                display_string_at(0,0,"mmap-f");
                return -1;
            }
            
            close(shm_fd);
            memset(shared_memory, 0, BUFFER_SIZE);
        
            // 创建并初始化信号量
            semaphore = sem_open(SEM_NAME, O_CREAT, 0666, 0);
            if (semaphore == SEM_FAILED) {
                perror("sem_open failed");
                display_string_at(0,0,"sem-f");
                munmap(shared_memory, BUFFER_SIZE);
                shm_unlink(SHM_NAME);
                return -1;
            }
        
            // 创建显示更新线程
            pthread_t display_thread;
            if (pthread_create(&display_thread, NULL, display_update_thread, NULL) != 0) {
                perror("Failed to create display thread");
                display_string_at(0,0,"pth-f");
                return -1;
            }
            pthread_detach(display_thread);  // 线程后台运行
    //显示文字测试
    //display_string("FH");
    //demo_image_sequence("/test/test.bmp");

    char mode = '0';  // 修改这个值可以选择不同的演示模式
    //display_string_at(200, 200, "Hello");
    if (mode == '1') {
        // 优化的图片显示演示
        demo_image_display_optimized();
    } else if (mode == '2') {
        // 文字显示演示
        //display_string("'\"杭州科技' 测试文字显示");
    } else if (mode == '3') {
        // 简单测试图案
        printf("显示简单测试图案\n");
        display_gradient_instant();
    } else if (mode == '4') {
        // 瞬间棋盘格测试
        printf("显示瞬间棋盘格\n");
        display_checkerboard_instant(32);
    } else if (mode == '5') {
        // 序列图播放演示
        printf("🎬 序列图播放演示\n");
        demo_image_sequence("/test/images");
    }
    printf("      ###################### 初始化设备 ######################      \n\n");
    
    return 0;
}




void disp_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    
    uint32_t width = area->x2 - area->x1 + 1;    // 刷新区域的像素宽度（关键：单位是像素，不是字节）
    uint32_t height = area->y2 - area->y1 + 1;  // 刷新区域的像素高度
    uint32_t pixel_count = width * height;       // 总像素数（4位色深）
    uint32_t bytes_written = 0;                  // 缓冲区已写入的字节数
    uint32_t transfer_count = 0;                 // SPI传输次数计数器
    
    #define SPI_MAX_TRANSFER_SIZE 1024
    static uint8_t transfer_buffer[SPI_MAX_TRANSFER_SIZE];
    printf("Flush started (4-bit color): area (%d,%d)-(%d,%d), pixels=%d\n", 
           area->x1, area->y1, area->x2, area->y2, pixel_count);    // 添加调试信息
    
    uint8_t current_byte = 0;          // 临时存储2个4位像素（高4位+低4位）
    bool has_upper_nibble = false;     // 标记是否已存储高4位像素
    uint32_t start_pixel = 0;          // 记录当前传输块的**起始像素索引**（关键修正）
    
    for (uint32_t i = 0; i < pixel_count; i++) {
        // 提取4位像素数据（假设有效数据在低4位，可根据实际格式调整）
        uint8_t pixel_4bit = (*((uint8_t *)&color_p[i])) & 0x0F;
        
        if (!has_upper_nibble) {
            // 第一个像素：存到字节的高4位
            current_byte = (pixel_4bit << 4) & 0xF0;
            has_upper_nibble = true;
        } else {
            // 第二个像素：存到字节的低4位，完成1字节
            current_byte |= pixel_4bit & 0x0F;
            transfer_buffer[bytes_written++] = current_byte;
            has_upper_nibble = false;
        }
        
        // 触发SPI传输的条件：缓冲区满 或 处理完所有像素
        bool is_last_pixel = (i == pixel_count - 1);
        if (bytes_written >= SPI_MAX_TRANSFER_SIZE || is_last_pixel) {
            // 处理最后一个像素（若为奇数，补全1字节）
            if (is_last_pixel && has_upper_nibble) {
                transfer_buffer[bytes_written++] = current_byte;  // 低4位留空
                has_upper_nibble = false;
            }
            
            // 关键修正：计算传输块的**起始像素坐标**（基于start_pixel）
            uint32_t start_row = area->y1 + (start_pixel / width);  // 起始行 = 起始像素 / 宽度
            uint32_t start_col = area->x1 + (start_pixel % width);  // 起始列 = 起始像素 % 宽度
            
            // 用起始坐标传输，而非当前像素i的坐标
            if (spi_wr_buffer(start_col, start_row, transfer_buffer, bytes_written) != 0) {
                printf("SPI transfer failed!");
            }
            transfer_count++;
            
            // 更新下一次传输的起始像素（当前传输块的结束像素+1）
            start_pixel = i + 1;
            
            // 重置缓冲区
            bytes_written = 0;
            current_byte = 0;
        }
    }
    
    // 打印传输次数
    printf("spi_wr_buffer called %d times\n", transfer_count);
    
    // 验证传输字节数（4位色深：总字节数 = 总像素数 ÷ 2 向上取整）
    uint32_t expected_bytes = (pixel_count + 1) / 2;
    uint32_t actual_bytes = (transfer_count - 1) * SPI_MAX_TRANSFER_SIZE + bytes_written;
    if (actual_bytes == expected_bytes) {
        printf("Bytes verified: %d bytes for %d pixels\n", actual_bytes, pixel_count);
    } else {
        printf("Bytes mismatch: expected %d, actual %d\n", expected_bytes, actual_bytes);
    }
    
    send_cmd(SPI_SYNC);
    usleep(1 * 1000);
    lv_disp_flush_ready(drv);
}



/* LVGL 系统初始化 */
void lvgl_init(void) {
    // 初始化LVGL核心
    //lv_init();

    // 初始化显示缓冲区
    lv_disp_draw_buf_init(&draw_buf, disp_buf, NULL, DISP_BUF_SIZE);
  
    // 初始化显示驱动
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 640;  // 水平分辨率
    disp_drv.ver_res = 480;  // 垂直分辨率
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
}

/* 系统定时器回调 (用于LVGL心跳) */
void sys_tick_handler(void) {
    lv_tick_inc(1); // 增加LVGL的时钟计数
}

/* 主应用初始化 */
// void app_init(void) {
   
//     lv_obj_t *scr = lv_scr_act();
    
//     // 1. 设置背景为黑色（RGB332 0x00）
//     lv_obj_set_style_bg_color(scr, lv_color_make(0, 0, 0), 0);  // 黑色背景
//     lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);  // 完全不透明
    
//     // 2. 创建白色文本（RGB332 0xFF）
//     lv_obj_t *label = lv_label_create(scr);
//     lv_label_set_text(label, "WeiXiaokeJi");
//     lv_obj_set_style_text_color(label, lv_color_make(255, 255, 255), 0);  // 白色文本
    
//     //设置字体48
//     lv_obj_set_style_text_font(label, &lv_font_montserrat_48, 0);

//     //lv_obj_set_width(label, LV_PCT(100));  // 设置 label 宽度为屏幕宽度
//     //lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);  // 允许文本换行
//     //居中
//     lv_obj_center(label);

// }




// 循环函数（按需保留）
void loop() {
    //clr_char();
    usleep(100 * 1000);
}

// 主函数
int main() {
    usleep(10 * 1000);
    setup();
	clr_cache();
    
    // 初始化LVGL
    lvgl_init();
    
//    初始化应用
    //app_init();
    ui_init();
    

    lv_scr_load(ui_Screen1);

    // 初始化时间检测
    last_activity_time = time(NULL);

    while(1) {
        // 处理LVGL任务
        lv_task_handler();

        if (hide_smile_flag) {
            if (ui_Label2 != NULL) {
                lv_label_set_text(ui_Label2, shared_memory);
                lv_obj_invalidate(lv_scr_act());  // 关键修改： invalidate整个屏幕
                lv_refr_now(lv_disp_get_default()); // 强制刷新
            }
            hide_smile_flag = false;
        }
        
        // 省电模式检测
        time_t current_time = time(NULL);
        
        if (!display_power_save_mode) {
            // 检查是否需要进入省电模式
            if (current_time - last_activity_time >= POWER_SAVE_TIMEOUT) {
                send_cmd(SPI_DISPLAY_DISABLE);
                send_cmd(SPI_SYNC);
                usleep(1 * 1000);
                display_power_save_mode = true;
                power_save_start_time = current_time;
            }
        } else {
            // 已在省电模式，显示省电状态信息
            time_t power_save_duration = current_time - power_save_start_time;
            if (power_save_duration % 10 == 0 && power_save_duration > 0) {
            }
        }
        
        usleep(10 * 1000);
    }
    printf("Succsss\n");
    close(spi_file);
    return 0;
}

// ================== 图片加载API ==================

/**
 * BMP文件头结构定义
 */
 typedef struct {
    uint16_t type;           // 文件类型 'BM'
    uint32_t size;           // 文件大小
    uint32_t reserved;       // 保留字段
    uint32_t offset;         // 数据偏移
    uint32_t header_size;    // 头大小
    int32_t width;           // 图片宽度
    int32_t height;          // 图片高度
    uint16_t planes;         // 颜色平面数
    uint16_t bits_per_pixel; // 每像素位数
} __attribute__((packed)) bmp_header_t;

/**
 * 高效的BMP读取 - 直接内存拷贝版本（不做翻转和颜色转换）
 * 性能最优，适合不需要翻转的场景
 */
uint8_t* load_bmp_image_fast(const char* filename, uint16_t* width, uint16_t* height) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("错误：无法打开文件 %s\n", filename);
        return NULL;
    }
    
    bmp_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        printf("错误：读取BMP头失败\n");
        fclose(file);
        return NULL;
    }
    
    if (header.type != 0x4D42 || header.bits_per_pixel != 24) {
        printf("错误：不支持的BMP格式（仅支持24位RGB）\n");
        fclose(file);
        return NULL;
    }
    
    *width = header.width;
    *height = abs(header.height);
    
    printf("快速读取BMP图片：%d×%d\n", *width, *height);
    
    // 分配RGB数据内存
    uint32_t data_size = (*width) * (*height) * 3;
    uint8_t* rgb_data = malloc(data_size);
    if (!rgb_data) {
        printf("错误：内存分配失败\n");
        fclose(file);
        return NULL;
    }
    
    fseek(file, header.offset, SEEK_SET);
    
    // 检查是否需要处理行对齐
    uint32_t row_size = ((*width) * 3 + 3) & ~3;  // 4字节对齐
    uint32_t pixel_row_size = (*width) * 3;       // 实际像素数据大小
    
    if (row_size == pixel_row_size) {
        // 最优情况：无padding，一次性读取整个图像
        printf("无padding，一次性读取 %d 字节\n", data_size);
        if (fread(rgb_data, data_size, 1, file) != 1) {
            printf("错误：读取图像数据失败\n");
            free(rgb_data);
            fclose(file);
            return NULL;
        }
    } else {
        // 有padding：逐行读取，但用memcpy优化
        printf("有padding，逐行读取\n");
        uint8_t* row_buffer = malloc(row_size);
        
        for (int row = 0; row < *height; row++) {
            if (fread(row_buffer, row_size, 1, file) != 1) {
                printf("错误：读取第%d行失败\n", row);
                free(rgb_data);
                free(row_buffer);
                fclose(file);
                return NULL;
            }
            
            // 使用memcpy复制有效数据，跳过padding
            uint32_t dst_offset = row * pixel_row_size;
            memcpy(rgb_data + dst_offset, row_buffer, pixel_row_size);
        }
        
        free(row_buffer);
    }
    
    fclose(file);
    printf("BMP快速读取完成！\n");
    return rgb_data;
}

/**
 * 读取RAW RGB图片文件
 * 文件格式：宽度(2字节) + 高度(2字节) + RGB数据
 */
uint8_t* load_raw_image(const char* filename, uint16_t* width, uint16_t* height) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("错误：无法打开文件 %s\n", filename);
        return NULL;
    }
    
    // 读取宽度和高度
    if (fread(width, 2, 1, file) != 1 || fread(height, 2, 1, file) != 1) {
        printf("错误：读取图片尺寸失败\n");
        fclose(file);
        return NULL;
    }
    
    printf("读取RAW图片：%d×%d\n", *width, *height);
    
    // 分配内存并读取RGB数据
    uint32_t data_size = (*width) * (*height) * 3;
    uint8_t* rgb_data = malloc(data_size);
    if (!rgb_data) {
        printf("错误：内存分配失败\n");
        fclose(file);
        return NULL;
    }
    
    if (fread(rgb_data, data_size, 1, file) != 1) {
        printf("错误：读取RGB数据失败\n");
        free(rgb_data);
        fclose(file);
        return NULL;
    }
    
    fclose(file);
    printf("RAW图片读取成功！\n");
    return rgb_data;
}

/**
 * 通用图片加载函数
 * @param filename 图片文件路径
 * @param width 输出图片宽度
 * @param height 输出图片高度
 * @return RGB数据指针，需要调用者释放内存
 */
uint8_t* load_image(const char* filename, uint16_t* width, uint16_t* height) {
    if (!filename || !width || !height) {
        printf("错误：参数无效\n");
        return NULL;
    }
    
    // 检查文件是否存在
    struct stat st;
    if (stat(filename, &st) != 0) {
        printf("错误：文件不存在 %s\n", filename);
        return NULL;
    }
    
    printf("加载图片文件：%s (大小: %ld 字节)\n", filename, st.st_size);
    
    // 根据文件扩展名判断格式
    const char* ext = strrchr(filename, '.');
    if (ext) {
        if (strcasecmp(ext, ".bmp") == 0) {
            return load_bmp_image_fast(filename, width, height);
        } else if (strcasecmp(ext, ".raw") == 0) {
            return load_raw_image(filename, width, height);
        }
    }
    
    // 尝试作为BMP文件读取
    printf("未知格式，尝试作为BMP文件读取...\n");
    return load_bmp_image_fast(filename, width, height);
}

/**
 * 加载并显示图片文件
 * @param filename 图片文件路径（默认：/test/test.bmp）
 */
int load_and_display_image(const char* filename) {
    const char* default_path = "/test/test.bmp";
    if (!filename) {
        filename = default_path;
    }
    
    printf("\n=== 加载并显示图片 ===\n");
    printf("图片路径：%s\n", filename);
    
    uint16_t width, height;
    uint8_t* rgb_data = load_image(filename, &width, &height);
    
    if (!rgb_data) {
        printf("图片加载失败！\n");
        
        return -1;
    }
    
    printf("图片加载成功：%d×%d\n", width, height);
    
    // 显示图片
    int result = display_rgb_image(rgb_data, width, height);
    
    // 清理内存
    free(rgb_data);
    
    if (result == 0) {
        printf("图片显示完成：%s\n", filename);
    } else {
        printf("图片显示失败：%s\n", filename);
    }
    
    return result;
}

// ================== 图片加载API结束 ==================


// ================== 图片显示API ==================
/**
 * RGB颜色转换为4位颜色值 - 优化版本
 * 使用内联函数和位运算优化性能
 */
uint8_t rgb_to_4bit_fast(uint8_t r, uint8_t g, uint8_t b) {
    // 使用位运算近似灰度转换：0.3*r + 0.59*g + 0.11*b
    // 系数转换：30% ≈ 77/256, 59% ≈ 151/256, 11% ≈ 28/256
    uint16_t gray = (r * 77 + g * 151 + b * 28) >> 8;
    return ((gray > 255 ? 255 : gray) >> 4) & 0x0F;
}

// 保留原函数用于兼容性
uint8_t rgb_to_4bit(uint8_t r, uint8_t g, uint8_t b) {
    return rgb_to_4bit_fast(r, g, b);
}

/**
 * 4位灰度值转换回RGB（用于保存调试）
 */
void bit4_to_rgb(uint8_t gray4, uint8_t* r, uint8_t* g, uint8_t* b) {
    // 将4位灰度(0-15)扩展到8位(0-255)
    uint8_t gray8 = (gray4 << 4) | gray4;  // 例如：0x0F -> 0xFF, 0x08 -> 0x88
    *r = *g = *b = gray8;
}

/**
 * 保存4位图像数据为BMP文件
 * @param filename 输出文件名
 * @param image_data 4位图像数据（每字节包含2个像素）
 * @param width 图像宽度
 * @param height 图像高度
 */
int save_4bit_to_bmp(const char* filename, uint8_t* image_data, uint16_t width, uint16_t height) {
    if (!filename || !image_data) {
        printf("错误：保存参数无效\n");
        return -1;
    }
    
    printf("保存4位图像到BMP文件：%s (%d×%d)\n", filename, width, height);
    
    FILE* file = fopen(filename, "wb");
    if (!file) {
        printf("错误：无法创建文件 %s\n", filename);
        return -1;
    }
    
    // BMP文件头
    uint32_t row_size = (width * 3 + 3) & ~3;  // 4字节对齐
    uint32_t image_size = row_size * height;
    uint32_t file_size = 54 + image_size;
    
    // 写入BMP文件头
    uint8_t header[54] = {0};
    
    // 文件头 (14字节)
    header[0] = 'B'; header[1] = 'M';
    *(uint32_t*)(header + 2) = file_size;
    *(uint32_t*)(header + 10) = 54;
    
    // 信息头 (40字节)
    *(uint32_t*)(header + 14) = 40;
    *(int32_t*)(header + 18) = width;
    *(int32_t*)(header + 22) = height;
    *(uint16_t*)(header + 26) = 1;
    *(uint16_t*)(header + 28) = 24;
    *(uint32_t*)(header + 34) = image_size;
    
    fwrite(header, 54, 1, file);
    
    // 转换并写入图像数据
    uint8_t* row_data = malloc(row_size);
    memset(row_data, 0, row_size);
    
    for (int row = height - 1; row >= 0; row--) {  // BMP从下到上
        for (int col = 0; col < width; col++) {
            // 从4位数据中提取像素值
            uint32_t data_index = row * ((width + 1) / 2) + col / 2;
            uint8_t packed_pixels = image_data[data_index];
            uint8_t pixel4;
            
            if (col % 2 == 0) {
                pixel4 = (packed_pixels >> 4) & 0x0F;  // 高4位
            } else {
                pixel4 = packed_pixels & 0x0F;         // 低4位
            }
            
            // 转换为RGB
            uint8_t r, g, b;
            bit4_to_rgb(pixel4, &r, &g, &b);
            
            // 写入BGR格式
            int index = col * 3;
            row_data[index + 0] = b;  // B
            row_data[index + 1] = g;  // G
            row_data[index + 2] = r;  // R
        }
        
        fwrite(row_data, row_size, 1, file);
    }
    
    free(row_data);
    fclose(file);
    
    printf("4位图像保存完成：%s\n", filename);
    return 0;
}

/**
 * 从RGB数据生成图片并显示
 * @param rgb_data RGB数据数组 (r,g,b,r,g,b...)
 * @param width 图片宽度（0表示使用默认640）
 * @param height 图片高度（0表示使用默认480）
 */
int display_rgb_image(uint8_t* rgb_data, uint16_t width, uint16_t height) {
    // 设置默认尺寸
    if (width == 0) width = 640;
    if (height == 0) height = 480;
    
    if (rgb_data == NULL || width > 640 || height > 480) {
        printf("错误：RGB数据无效或尺寸超出范围\n");
        return -1;
    }
    
    printf("转换并显示RGB图片 %d×%d\n", width, height);
    
    // 分配转换后的图片数据
    uint32_t converted_size = ((width + 1) / 2) * height;
    uint8_t* converted_data = malloc(converted_size);
    if (converted_data == NULL) {
        printf("错误：内存分配失败\n");
        return -1;
    }
    
    // RGB转换为4位格式 - 推荐版本2：减少条件判断 + 内联优化
    uint8_t* rgb_ptr = rgb_data;  // RGB数据指针
    uint8_t* output_ptr = converted_data;  // 输出数据指针
    uint32_t pixel_pairs = width / 2;  // 完整的像素对数量
    bool has_odd_pixel = (width % 2) != 0;  // 是否有奇数像素
    
    for (uint16_t row = 0; row < height; row++) {
        // 处理成对的像素 - 减少分支预测失败
        for (uint16_t pair = 0; pair < pixel_pairs; pair++) {
            uint8_t pixel1 = rgb_to_4bit_fast(rgb_ptr[0], rgb_ptr[1], rgb_ptr[2]);
            rgb_ptr += 3;
            uint8_t pixel2 = rgb_to_4bit_fast(rgb_ptr[0], rgb_ptr[1], rgb_ptr[2]);
            rgb_ptr += 3;
            
            *output_ptr++ = (pixel1 << 4) | pixel2;
        }
        
        // 处理最后一个奇数像素（如果存在）
        if (has_odd_pixel) {
            uint8_t pixel1 = rgb_to_4bit_fast(rgb_ptr[0], rgb_ptr[1], rgb_ptr[2]);
            rgb_ptr += 3;
            *output_ptr++ = (pixel1 << 4);  // 第二个像素为0
        }
    }
    
    // ✅ 已实现的版本：减少条件判断 + 内联函数 + 位运算优化
    
    // 保存转换后的图片用于调试
    save_4bit_to_bmp("/test/out.bmp", converted_data, width, height);
    
    // 显示转换后的图片
    int result = display_image_fast(converted_data, width, height);
    
    free(converted_data);
    return result;
}

/**
 * 显示图片（支持任意尺寸，非全屏图片自动居中）最大支持的全屏数据：320×480字节 (640×480像素，2像素/字节)
 * @param image_data 图片数据
 * @param width 图片宽度（像素）
 * @param height 图片高度（像素）
 */
int display_image_fast(uint8_t* image_data, uint16_t width, uint16_t height) {
    if (image_data == NULL) {
        printf("错误：图片数据为空\n");
        return -1;
    }
    
    if (width > 640 || height > 480) {
        printf("错误：图片尺寸超出屏幕范围\n");
        return -1;
    }
    
    printf("显示图片 %d×%d\n", width, height);
    
    // 清屏
    clr_cache();
    
    // 判断是否为全屏图片
    if (width == 640 && height == 480) {
        // 全屏快速显示
    uint32_t total_size = 320 * 480;  // 153600字节
        printf("全屏快速显示 (%d字节)...\n", total_size);
        display_image(0, 0, image_data, total_size);
    } else {
        // 非全屏图片：需要转换为全屏缓冲区格式
        printf("转换非全屏图片为全屏缓冲区格式\n");
        
        // 分配全屏缓冲区并清零（黑色背景）
        uint32_t screen_size = 320 * 480;
        uint8_t* screen_buffer = malloc(screen_size);
        if (!screen_buffer) {
            printf("错误：内存分配失败\n");
            return -1;
        }
        memset(screen_buffer, 0x00, screen_size);  // 黑色背景
        
        // 计算居中位置
        uint16_t start_x = (640 - width) / 2;
        uint16_t start_y = (480 - height) / 2;
        
        printf("居中显示在位置 (%d,%d)\n", start_x, start_y);
        
        // 高效的行复制方式 - 借鉴用户的优化思路
        uint16_t src_row_bytes = (width + 1) / 2;  // 原图每行字节数
        uint16_t dst_row_bytes = 320;              // 全屏缓冲区每行字节数
        uint16_t start_x_bytes = start_x / 2;      // 起始列的字节偏移
        
        for (uint16_t row = 0; row < height; row++) {
            uint16_t screen_row = start_y + row;
            if (screen_row >= 480) break;
            
            // 计算源和目标行的起始指针
            uint8_t* src_row = &image_data[row * src_row_bytes];
            uint8_t* dst_row = &screen_buffer[screen_row * dst_row_bytes + start_x_bytes];
            
            // 一次性复制整行数据
            memcpy(dst_row, src_row, src_row_bytes);
        }
        
        // 使用全屏模式显示
        printf("全屏模式显示转换后的缓冲区\n");
        display_image(0, 0, screen_buffer, screen_size);
        
        free(screen_buffer);
    }
    
    printf("图片显示完成！\n");
    return 0;
}
// ================== 图片显示API结束 ==================

// ================== 🎬 图片缩放动画API ==================

/**
 * 简单最近邻缩放算法
 * @param src_data 源图片RGB数据
 * @param src_width 源图片宽度
 * @param src_height 源图片高度  
 * @param dst_width 目标宽度
 * @param dst_height 目标高度
 * @return 缩放后的RGB数据，需要调用者释放
 */
uint8_t* scale_image_nearest(uint8_t* src_data, uint16_t src_width, uint16_t src_height, 
                           uint16_t dst_width, uint16_t dst_height) {
    if (!src_data || dst_width == 0 || dst_height == 0) {
        return NULL;
    }
    
    // 分配目标图片内存
    uint32_t dst_size = dst_width * dst_height * 3;
    uint8_t* dst_data = malloc(dst_size);
    if (!dst_data) {
        printf("错误：缩放内存分配失败\n");
        return NULL;
    }
    
    // 计算缩放比例 (定点数运算，避免浮点)
    uint32_t x_ratio = (src_width << 16) / dst_width;   // 16.16定点数
    uint32_t y_ratio = (src_height << 16) / dst_height;
    
    // 最近邻缩放
    for (uint16_t dst_y = 0; dst_y < dst_height; dst_y++) {
        for (uint16_t dst_x = 0; dst_x < dst_width; dst_x++) {
            // 计算对应的源像素坐标
            uint16_t src_x = (dst_x * x_ratio) >> 16;
            uint16_t src_y = (dst_y * y_ratio) >> 16;
            
            // 边界检查
            if (src_x >= src_width) src_x = src_width - 1;
            if (src_y >= src_height) src_y = src_height - 1;
            
            // 复制像素数据
            uint32_t src_idx = (src_y * src_width + src_x) * 3;
            uint32_t dst_idx = (dst_y * dst_width + dst_x) * 3;
            
            dst_data[dst_idx + 0] = src_data[src_idx + 0];  // R
            dst_data[dst_idx + 1] = src_data[src_idx + 1];  // G
            dst_data[dst_idx + 2] = src_data[src_idx + 2];  // B
        }
    }
    
    return dst_data;
}

/**
 * 缩放动画参数结构
 */
struct zoom_animation_t {
    float start_scale;      // 起始缩放比例 (0.1 = 10%)
    float end_scale;        // 结束缩放比例 (1.0 = 100%)
    uint16_t total_frames;  // 总帧数
    uint16_t frame_delay;   // 每帧延迟(毫秒)
    bool loop_back;         // 是否来回循环
};

/**
 * 序列图播放动画参数结构体
 */
struct sequence_animation_t {
    float target_fps;       // 目标帧率 (0.0 = 最快速度)
    bool pingpong_mode;     // true = ping-pong播放, false = 循环播放
    uint16_t loop_count;    // 循环次数 (0 = 无限循环)
    bool show_performance;  // 是否显示性能信息
};

/**
 * BMP图片缩放动画显示
 * @param filename BMP文件路径
 * @param anim_params 动画参数
 */
int display_bmp_zoom_animation(const char* filename, struct zoom_animation_t* anim_params) {
    if (!filename || !anim_params) {
        printf("错误：动画参数无效\n");
        return -1;
    }
    
    printf("\n🎬 === BMP缩放动画开始 ===\n");
    printf("文件: %s\n", filename);
    printf("缩放范围: %.1f%% → %.1f%%\n", 
           anim_params->start_scale * 100, anim_params->end_scale * 100);
    printf("总帧数: %d, 延迟: %dms\n", 
           anim_params->total_frames, anim_params->frame_delay);
    
    // 加载原始BMP图片
    uint16_t orig_width, orig_height;
    uint8_t* orig_rgb = load_bmp_image_fast(filename, &orig_width, &orig_height);
    if (!orig_rgb) {
        printf("❌ BMP加载失败\n");
        return -1;
    }
    
    printf("✅ 原图加载成功: %d×%d\n", orig_width, orig_height);
    
    // 帧率统计变量
    struct timeval animation_start, frame_start, frame_end;
    gettimeofday(&animation_start, NULL);
    
    float target_fps = 1000.0f / anim_params->frame_delay;
    printf("🎯 目标帧率: %.1f FPS\n", target_fps);
    
    // 动画循环
    uint16_t total_animation_frames = anim_params->loop_back ? 
                                     (anim_params->total_frames * 2) : 
                                     anim_params->total_frames;
    
    float total_frame_time = 0.0f;  // 累计帧处理时间
    float total_actual_delay = 0.0f;  // 累计实际延迟时间
    
    for (uint16_t frame = 0; frame < total_animation_frames; frame++) {
        gettimeofday(&frame_start, NULL);  // 记录帧开始时间
        
        // 计算当前帧的缩放比例
        float progress;
        if (anim_params->loop_back && frame >= anim_params->total_frames) {
            // 回程：从end_scale回到start_scale
            progress = 1.0f - (float)(frame - anim_params->total_frames) / anim_params->total_frames;
        } else {
            // 去程：从start_scale到end_scale
            progress = (float)(frame % anim_params->total_frames) / anim_params->total_frames;
        }
        
        float current_scale = anim_params->start_scale + 
                             progress * (anim_params->end_scale - anim_params->start_scale);
        
        // 计算当前帧的目标尺寸
        uint16_t scaled_width = (uint16_t)(orig_width * current_scale);
        uint16_t scaled_height = (uint16_t)(orig_height * current_scale);
        
        // 限制最小尺寸
        if (scaled_width < 2) scaled_width = 2;
        if (scaled_height < 2) scaled_height = 2;
        
        // 限制最大尺寸
        if (scaled_width > 640) scaled_width = 640;
        if (scaled_height > 480) scaled_height = 480;
        
        // 缩放图片
        uint8_t* scaled_rgb = scale_image_nearest(orig_rgb, orig_width, orig_height,
                                                scaled_width, scaled_height);
        if (!scaled_rgb) {
            printf("❌ 帧%d缩放失败\n", frame);
            continue;
        }
        
        // 显示缩放后的图片（自动居中）
        display_rgb_image(scaled_rgb, scaled_width, scaled_height);
        
        // 释放当前帧内存
        free(scaled_rgb);
        
        // 计算帧处理时间
        gettimeofday(&frame_end, NULL);
        float frame_process_time = (frame_end.tv_sec - frame_start.tv_sec) * 1000.0f + 
                                  (frame_end.tv_usec - frame_start.tv_usec) / 1000.0f;
        total_frame_time += frame_process_time;
        
        // 计算实际应该延迟的时间
        float remaining_delay = anim_params->frame_delay - frame_process_time;
        if (remaining_delay > 0) {
            usleep((int)(remaining_delay * 1000));
            total_actual_delay += remaining_delay;
        } else {
            total_actual_delay += 0;  // 处理时间超过目标延迟
        }
        
        // 计算并输出实时帧率
        float actual_frame_time = frame_process_time + (remaining_delay > 0 ? remaining_delay : 0);
        float current_fps = 1000.0f / actual_frame_time;
        
        printf("🎞️ 帧 %d/%d: %.1f%% (%d×%d) | 处理:%.1fms | 实际FPS:%.1f\n", 
               frame + 1, total_animation_frames, current_scale * 100, 
               scaled_width, scaled_height, frame_process_time, current_fps);
    }
    
    // 计算总体动画统计
    struct timeval animation_end;
    gettimeofday(&animation_end, NULL);
    
    float total_animation_time = (animation_end.tv_sec - animation_start.tv_sec) * 1000.0f + 
                                (animation_end.tv_usec - animation_start.tv_usec) / 1000.0f;
    
    float avg_frame_process_time = total_frame_time / total_animation_frames;
    float avg_actual_fps = 1000.0f * total_animation_frames / total_animation_time;
    float theoretical_time = anim_params->frame_delay * total_animation_frames;
    float time_efficiency = (theoretical_time / total_animation_time) * 100.0f;
    
    printf("\n📊 === 动画性能统计 ===\n");
    printf("🎯 目标帧率: %.1f FPS (%.1fms/帧)\n", target_fps, (float)anim_params->frame_delay);
    printf("⚡ 实际帧率: %.1f FPS (%.1fms/帧)\n", avg_actual_fps, total_animation_time / total_animation_frames);
    printf("🔧 平均处理时间: %.1fms/帧\n", avg_frame_process_time);
    printf("⏱️ 总动画时间: %.1fms (理论: %.1fms)\n", total_animation_time, theoretical_time);
    printf("📈 时间效率: %.1f%%\n", time_efficiency);
    
    if (avg_actual_fps < target_fps * 0.9f) {
        printf("  警告：实际帧率低于目标帧率90%%，建议：\n");
        printf("   - 增加frame_delay延迟时间\n");
        printf("   - 减少图片尺寸\n");
        printf("   - 减少总帧数\n");
    } else if (avg_actual_fps > target_fps * 1.1f) {
        printf(" 性能良好：可以考虑降低延迟或增加效果复杂度\n");
    } else {
        printf(" 帧率达标：性能表现良好\n");
    }
    
    // 清理资源
    free(orig_rgb);
    
    printf("🎬 缩放动画完成！\n\n");
    return 0;
}

/**
 * 预设动画效果
 */
void demo_zoom_effects(const char* filename) {
    printf("\n🎭 === 缩放动画演示集 ===\n");
    
    struct timeval demo_start, demo_end;
    gettimeofday(&demo_start, NULL);
    
    // 效果1: 从小放大 (经典缩放入场)
    printf("\n📈 效果1: 缩放入场动画\n");
    struct zoom_animation_t zoom_in = {
        .start_scale = 0.1f,     // 从10%开始
        .end_scale = 1.0f,       // 到100%
        .total_frames = 30,      // 30帧
        .frame_delay = 50,       // 50ms/帧
        .loop_back = false       // 单向
    };
    display_bmp_zoom_animation(filename, &zoom_in);
    
    usleep(1000 * 1000);  // 间隔1秒
    
    // 效果2: 呼吸效果 (来回缩放)
    printf("\n💨 效果2: 呼吸缩放效果\n");
    struct zoom_animation_t breathing = {
        .start_scale = 0.8f,     // 从80%开始
        .end_scale = 1.0f,       // 到100%
        .total_frames = 20,      // 20帧
        .frame_delay = 100,      // 100ms/帧
        .loop_back = true        // 来回循环
    };
    display_bmp_zoom_animation(filename, &breathing);
    
    usleep(1000 * 1000);  // 间隔1秒
    
    // 效果3: 快速脉冲
    printf("\n⚡ 效果3: 快速脉冲效果\n");
    struct zoom_animation_t pulse = {
        .start_scale = 0.5f,     // 从50%开始
        .end_scale = 1.2f,       // 到120% (超出原图)
        .total_frames = 15,      // 15帧
        .frame_delay = 30,       // 30ms/帧 (快速)
        .loop_back = true        // 来回
         };
     display_bmp_zoom_animation(filename, &pulse);
     
     // 演示集性能总结
     gettimeofday(&demo_end, NULL);
     float total_demo_time = (demo_end.tv_sec - demo_start.tv_sec) * 1000.0f + 
                            (demo_end.tv_usec - demo_start.tv_usec) / 1000.0f;
     
     printf("\n🏁 === 演示集完成 ===\n");
     printf("⏱️  总演示时间: %.2f秒\n", total_demo_time / 1000.0f);
     printf("🎬 演示了3种不同的缩放动画效果\n");
     printf("📊 性能数据可用于优化参数设置\n\n");
 }

// ================== 🎬 图片缩放动画API结束 ==================


// ================== 图片显示Demo ==================
/**
 * 生成并瞬间显示棋盘格图案
 * @param square_size 每个方格的边长（像素）
 */
void display_checkerboard_instant(uint16_t square_size) {
    printf("生成瞬间棋盘格图案，方格大小: %d×%d\n", square_size, square_size);
    
    // 分配全屏缓冲区
    uint32_t screen_size = 320 * 480;  // 640×480像素 = 320×480字节
    uint8_t* screen_buffer = malloc(screen_size);
    if (screen_buffer == NULL) {
        printf("错误：内存分配失败\n");
        return;
    }
    
    // 生成棋盘格图案
    for (uint16_t row = 0; row < 480; row++) {
        for (uint16_t col = 0; col < 640; col += 2) {
            // 计算当前位置的方格坐标
            uint16_t grid_row = row / square_size;
            uint16_t grid_col = col / square_size;
            
            // 棋盘格模式：(行+列)为偶数时为白色，奇数时为黑色
            uint8_t is_white = (grid_row + grid_col) % 2;
            uint8_t pixel1 = is_white ? 0x0F : 0x00;
            uint8_t pixel2 = is_white ? 0x0F : 0x00;
            
            // 第二个像素（如果存在）
            if (col + 1 < 640) {
                uint16_t grid_col2 = (col + 1) / square_size;
                uint8_t is_white2 = (grid_row + grid_col2) % 2;
                pixel2 = is_white2 ? 0x0F : 0x00;
            }
            
            // 存储到缓冲区
            uint32_t buffer_index = row * 320 + col / 2;
            screen_buffer[buffer_index] = (pixel1 << 4) | pixel2;
        }
    }
    
    printf("棋盘格数据生成完成，开始传输...\n");
    
    // 使用快速全屏显示
    display_image_fast(screen_buffer, 640, 480);
    
    free(screen_buffer);
    printf("棋盘格显示完成！\n");
}

/**
 * 优化的渐变图案 - 一次性生成和显示
 */
void display_gradient_instant() {
    printf("生成瞬间渐变图案\n");
    
    uint32_t screen_size = 320 * 480;
    uint8_t* screen_buffer = malloc(screen_size);
    if (screen_buffer == NULL) {
        printf("错误：内存分配失败\n");
        return;
    }
    
    // 生成双向渐变：水平+垂直
    for (uint16_t row = 0; row < 480; row++) {
        for (uint16_t col = 0; col < 640; col += 2) {
            // 水平渐变分量
            uint8_t h_gradient = (col * 15) / 640;
            // 垂直渐变分量  
            uint8_t v_gradient = (row * 15) / 480;
            // 混合渐变
            uint8_t pixel1 = (h_gradient + v_gradient) / 2;
            
            uint8_t pixel2 = pixel1;
            if (col + 1 < 640) {
                uint8_t h_gradient2 = ((col + 1) * 15) / 640;
                pixel2 = (h_gradient2 + v_gradient) / 2;
            }
            
            uint32_t buffer_index = row * 320 + col / 2;
            screen_buffer[buffer_index] = (pixel1 << 4) | pixel2;
        }
    }
    
    printf("渐变数据生成完成，开始传输...\n");
    display_image_fast(screen_buffer, 640, 480);
    
    free(screen_buffer);
    printf("渐变图案显示完成！\n");
}

/**
 * 生成同心圆图案
 */
void display_circles_instant() {
    printf("生成瞬间同心圆图案\n");
    
    uint32_t screen_size = 320 * 480;
    uint8_t* screen_buffer = malloc(screen_size);
    if (screen_buffer == NULL) {
        printf("错误：内存分配失败\n");
        return;
    }
    
    // 屏幕中心点
    int16_t center_x = 320;
    int16_t center_y = 240;
    
    for (uint16_t row = 0; row < 480; row++) {
        for (uint16_t col = 0; col < 640; col += 2) {
            // 计算到中心的距离
            int16_t dx1 = col - center_x;
            int16_t dy1 = row - center_y;
            uint16_t dist1 = sqrt(dx1*dx1 + dy1*dy1);
            uint8_t pixel1 = (dist1 / 20) & 0x0F;  // 每20像素一个环
            
            uint8_t pixel2 = pixel1;
            if (col + 1 < 640) {
                int16_t dx2 = (col + 1) - center_x;
                uint16_t dist2 = sqrt(dx2*dx2 + dy1*dy1);
                pixel2 = (dist2 / 20) & 0x0F;
            }
            
            uint32_t buffer_index = row * 320 + col / 2;
            screen_buffer[buffer_index] = (pixel1 << 4) | pixel2;
        }
    }
    
    printf("同心圆数据生成完成，开始传输...\n");
    display_image_fast(screen_buffer, 640, 480);
    
    free(screen_buffer);
    printf("同心圆图案显示完成！\n");
}

/**
 * 优化的图片显示演示
 */
void demo_image_display_optimized() {
    printf("=== 图片显示演示 ===\n");
    
    // 演示1：瞬间棋盘格 - 大方格
    printf("\n1. 瞬间显示大棋盘格 (64×64像素)\n");
    display_checkerboard_instant(64);
    usleep(3000 * 1000);  // 等待3秒
    
    // // 演示2：瞬间棋盘格 - 小方格
    // printf("\n2. 瞬间显示小棋盘格 (16×16像素)\n");
    // display_checkerboard_instant(16);
    // usleep(3000 * 1000);
    
    // // 演示3：瞬间棋盘格 - 超小方格
    // printf("\n3. 瞬间显示超小棋盘格 (8×8像素)\n");
    // display_checkerboard_instant(8);
    // usleep(3000 * 1000);
    
    // // 演示4：瞬间渐变
    // printf("\n4. 瞬间显示双向渐变\n");
    // display_gradient_instant();
    // usleep(3000 * 1000);
    
    // // 演示5：同心圆
    // printf("\n5. 瞬间显示同心圆\n");
    // display_circles_instant();
    // usleep(3000 * 1000);
    
    // 演示6：图片文件加载显示
    printf("\n6. 加载并显示图片文件\n");
    
    // 6.1 尝试加载默认图片
    load_and_display_image(NULL);  // 使用默认路径 /test/test.bmp
    usleep(2000 * 1000);
    
    // // 演示7：缩放动画效果
    // printf("\n7. 缩放动画演示\n");
    // demo_zoom_effects("/test/test.bmp");
    
    // 演示8：序列图播放
    printf("\n8. 序列图播放演示\n");
    demo_image_sequence("/test/images");
    
    printf("\n图片显示演示完成！\n");
}

// ===================== 📸 序列图播放功能 =====================

/**
 * 加载指定目录下的固定图片序列 (1.bmp 到 5.bmp)
 * @param directory 图片目录路径
 * @param filenames 输出：文件名数组指针
 * @param count 输出：图片数量
 * @return 0成功，-1失败
 */
int load_image_sequence(const char* directory, char*** filenames, int* count) {
    printf("📁 加载固定序列图: %s (1.bmp - 5.bmp)\n", directory);
    
    const int SEQUENCE_COUNT = 5;
    *filenames = malloc(SEQUENCE_COUNT * sizeof(char*));
    
    // 构建固定的5个文件路径
    for (int i = 0; i < SEQUENCE_COUNT; i++) {
        // 分配足够的内存存储路径
        (*filenames)[i] = malloc(strlen(directory) + 20);  // 足够存储路径 + "/X.bmp\0"
        
        // 构建文件路径：directory/N.bmp (N = 1,2,3,4,5)
        snprintf((*filenames)[i], strlen(directory) + 20, "%s/%d.bmp", directory, i + 1);
        
        printf("  📷 序列图片 %d: %s\n", i + 1, (*filenames)[i]);
    }
    
    *count = SEQUENCE_COUNT;
    printf("✅ 成功加载 %d 张序列图片\n\n", SEQUENCE_COUNT);
    return 0;
}

/**
 * 释放图片序列内存
 */
void free_image_sequence(char** filenames, int count) {
    if (filenames) {
        for (int i = 0; i < count; i++) {
            free(filenames[i]);
        }
        free(filenames);
    }
}

/**
 * 播放图片序列
 * @param filenames 图片文件名数组
 * @param count 图片数量
 * @param anim_params 播放参数
 * @return 0成功，-1失败
 */
int play_image_sequence(char** filenames, int count, struct sequence_animation_t* anim_params) {
    if (count == 0) {
        printf("❌ 错误：图片序列为空\n");
        return -1;
    }
    
    printf(" 开始播放序列图 (共%d张)\n", count);
    printf("    模式: %s\n", anim_params->pingpong_mode ? "Ping-Pong" : "循环");
    printf("    目标帧率: %.1f FPS\n", anim_params->target_fps > 0 ? anim_params->target_fps : 0);
    printf("    循环次数: %s\n", anim_params->loop_count == 0 ? "无限" : "有限");
    printf("    性能监控: %s\n\n", anim_params->show_performance ? "开启" : "关闭");
    
    // 计算帧间延迟
    uint32_t frame_delay_us = 0;
    if (anim_params->target_fps > 0) {
        frame_delay_us = (uint32_t)(1000000.0f / anim_params->target_fps);
    }
    
    // 性能统计变量
    struct timeval sequence_start, frame_start, frame_end;
    struct timeval load_start, load_end, process_start, process_end, display_start, display_end;
    gettimeofday(&sequence_start, NULL);
    
    uint32_t total_frames_shown = 0;
    float total_processing_time = 0.0f;
    float min_frame_time = 999999.0f;
    float max_frame_time = 0.0f;
    
    // 详细时间统计
    float total_load_time = 0.0f;
    float total_process_time = 0.0f;
    float total_display_time = 0.0f;
    float total_delay_time = 0.0f;
    
    uint16_t current_loop = 0;
    bool forward_direction = true;
    
    // 主播放循环
    while (anim_params->loop_count == 0 || current_loop < anim_params->loop_count) {
        
        // 确定播放方向和范围
        int start_idx, end_idx, step;
        if (forward_direction) {
            start_idx = 0;
            end_idx = count;
            step = 1;
        } else {
            start_idx = count - 1;
            end_idx = -1;
            step = -1;
        }
        
        // 播放当前方向的所有帧
        for (int i = start_idx; i != end_idx; i += step) {
            gettimeofday(&frame_start, NULL);
            
            // ========== 步骤1：图片加载 ==========
            gettimeofday(&load_start, NULL);
            uint16_t img_width, img_height;
            uint8_t* rgb_data = load_image(filenames[i], &img_width, &img_height);
            gettimeofday(&load_end, NULL);
            
            if (!rgb_data) {
                printf("⚠️  警告：无法加载图片 %s\n", filenames[i]);
                continue;
            }
            
            // ========== 步骤2：图片处理（RGB转换+缩放等） ==========
            gettimeofday(&process_start, NULL);
            
            // 细分时间测量点
            struct timeval alloc_start, alloc_end, convert_start, convert_end, copy_start, copy_end;
            
            // 2.1 内存分配
            gettimeofday(&alloc_start, NULL);
            uint8_t display_buffer[320 * 480];  // 栈分配，自动管理，替换掉原本的uint8_t* display_buffer = malloc(320 * 480);  // 4位格式缓冲区
            gettimeofday(&alloc_end, NULL);
            bool need_free_buffer = false;
            
            if (!display_buffer) {
                printf("⚠️  警告：内存分配失败 %s\n", filenames[i]);
                free(rgb_data);
                continue;
            }
            need_free_buffer = true;
            
            if (img_width == 640 && img_height == 480) {
                // ===== 全屏图片：直接RGB转换 =====
                
                // 2.2 缓冲区清零（全屏图片跳过）
                gettimeofday(&copy_start, NULL);
                gettimeofday(&copy_end, NULL);  // 立即结束，表示跳过
                
                // 2.3 RGB转换
                gettimeofday(&convert_start, NULL);
                
                // 高效RGB转4位格式（指针优化版本）
                uint8_t* rgb_ptr = rgb_data;
                uint8_t* buf_ptr = display_buffer;
                uint32_t pixel_pairs = (640 * 480) / 2;
                
                for (uint32_t pair = 0; pair < pixel_pairs; pair++) {
                    // 像素1：BGR→RGB
                    uint8_t pixel1 = rgb_to_4bit_fast(rgb_ptr[2], rgb_ptr[1], rgb_ptr[0]);
                    rgb_ptr += 3;
                    
                    // 像素2：BGR→RGB
                    uint8_t pixel2 = rgb_to_4bit_fast(rgb_ptr[2], rgb_ptr[1], rgb_ptr[0]);
                    rgb_ptr += 3;
                    
                    // 合并两个4位像素到一个字节
                    *buf_ptr++ = (pixel1 << 4) | pixel2;
                }
                
                gettimeofday(&convert_end, NULL);
                
            } else {
                // ===== 非全屏图片：居中处理 =====
                
                // 2.2 缓冲区清零
                gettimeofday(&copy_start, NULL);
                memset(display_buffer, 0, 320 * 480);  // 黑色背景
                gettimeofday(&copy_end, NULL);
                
                // 2.3 居中位置计算
                struct timeval calc_start, calc_end;
                gettimeofday(&calc_start, NULL);
                uint16_t start_x = (640 - img_width) / 2;
                uint16_t start_y = (480 - img_height) / 2;
                gettimeofday(&calc_end, NULL);
                
                // 2.4 逐像素转换和复制
                gettimeofday(&convert_start, NULL);
                for (uint16_t y = 0; y < img_height; y++) {
                    for (uint16_t x = 0; x < img_width; x += 2) {
                        uint16_t screen_x = start_x + x;
                        uint16_t screen_y = start_y + y;
                        
                        if (screen_x < 640 && screen_y < 480) {
                            uint8_t* rgb_ptr = rgb_data + (y * img_width + x) * 3;
                            uint8_t pixel1 = rgb_to_4bit_fast(rgb_ptr[2], rgb_ptr[1], rgb_ptr[0]);
                            
                            uint8_t pixel2 = pixel1;
                            if (x + 1 < img_width) {
                                rgb_ptr += 3;
                                pixel2 = rgb_to_4bit_fast(rgb_ptr[2], rgb_ptr[1], rgb_ptr[0]);
                            }
                            
                            uint32_t buffer_index = screen_y * 320 + screen_x / 2;
                            display_buffer[buffer_index] = (pixel1 << 4) | pixel2;
                        }
                    }
                }
                gettimeofday(&convert_end, NULL);
            }
            
            // 2.5 释放原始RGB数据
            struct timeval free_start, free_end;
            gettimeofday(&free_start, NULL);
            free(rgb_data);
            gettimeofday(&free_end, NULL);
            
            gettimeofday(&process_end, NULL);
            
            if (!display_buffer) {
                printf("⚠️  警告：内存分配失败 %s\n", filenames[i]);
                continue;
            }
            
            // ========== 步骤3：显示传输 ==========
            gettimeofday(&display_start, NULL);
            
            // 3.1 清屏操作
            struct timeval clear_start, clear_end;
            gettimeofday(&clear_start, NULL);
            clr_cache();
            gettimeofday(&clear_end, NULL);
            
            // 3.2 SPI数据传输
            struct timeval spi_start, spi_end;
            gettimeofday(&spi_start, NULL);
            uint32_t total_size = 320 * 480;  // 153600字节
            display_image(0, 0, display_buffer, total_size);
            gettimeofday(&spi_end, NULL);
            
            gettimeofday(&display_end, NULL);
            
            if (need_free_buffer) {
                free(display_buffer);
            }
            
            gettimeofday(&frame_end, NULL);
            
            // ========== 步骤4：计算各阶段时间 ==========
            float load_time = (load_end.tv_sec - load_start.tv_sec) * 1000.0f +
                             (load_end.tv_usec - load_start.tv_usec) / 1000.0f;
            float process_time = (process_end.tv_sec - process_start.tv_sec) * 1000.0f +
                                (process_end.tv_usec - process_start.tv_usec) / 1000.0f;
            float display_time = (display_end.tv_sec - display_start.tv_sec) * 1000.0f +
                                 (display_end.tv_usec - display_start.tv_usec) / 1000.0f;
            float total_frame_time = (frame_end.tv_sec - frame_start.tv_sec) * 1000.0f +
                                    (frame_end.tv_usec - frame_start.tv_usec) / 1000.0f;
            
            // 计算细分时间
            float alloc_time = (alloc_end.tv_sec - alloc_start.tv_sec) * 1000.0f +
                              (alloc_end.tv_usec - alloc_start.tv_usec) / 1000.0f;
            float copy_time = (copy_end.tv_sec - copy_start.tv_sec) * 1000.0f +
                             (copy_end.tv_usec - copy_start.tv_usec) / 1000.0f;
            float convert_time = (convert_end.tv_sec - convert_start.tv_sec) * 1000.0f +
                                (convert_end.tv_usec - convert_start.tv_usec) / 1000.0f;
            float free_time = (free_end.tv_sec - free_start.tv_sec) * 1000.0f +
                             (free_end.tv_usec - free_start.tv_usec) / 1000.0f;
            float clear_time = (clear_end.tv_sec - clear_start.tv_sec) * 1000.0f +
                              (clear_end.tv_usec - clear_start.tv_usec) / 1000.0f;
            float spi_time = (spi_end.tv_sec - spi_start.tv_sec) * 1000.0f +
                            (spi_end.tv_usec - spi_start.tv_usec) / 1000.0f;
            
            // 累计统计
            total_load_time += load_time;
            total_process_time += process_time;
            total_display_time += display_time;
            total_processing_time += total_frame_time;
            
            if (total_frame_time < min_frame_time) min_frame_time = total_frame_time;
            if (total_frame_time > max_frame_time) max_frame_time = total_frame_time;
            
            total_frames_shown++;
            
            // 详细性能信息显示
            if (anim_params->show_performance) {
                float current_fps = 1000.0f / total_frame_time;
                const char* filename = strrchr(filenames[i], '/') ? strrchr(filenames[i], '/') + 1 : filenames[i];
                
                printf("📊 帧 %d/%d | %s | 总时间: %.1fms | FPS: %.1f\n", 
                       total_frames_shown, count, filename, total_frame_time, current_fps);
                printf("   ├─ 📁 加载: %.1fms (%.1f%%)\n", load_time, (load_time/total_frame_time)*100);
                printf("   ├─ ⚙️  处理: %.1fms (%.1f%%) 🔍\n", process_time, (process_time/total_frame_time)*100);
                printf("   │  ├─ 🏷️  内存分配: %.2fms\n", alloc_time);
                if (copy_time > 0.01f) {  // 只有非全屏图片才显示
                    printf("   │  ├─ 🧹 缓冲区清零: %.2fms\n", copy_time);
                }
                printf("   │  ├─ 🔄 RGB转换: %.1fms\n", convert_time);
                printf("   │  └─ 🗑️  内存释放: %.2fms\n", free_time);
                printf("   └─ 📺 显示: %.1fms (%.1f%%) 🔍\n", display_time, (display_time/total_frame_time)*100);
                printf("      ├─ 🧹 清屏: %.1fms\n", clear_time);
                printf("      └─ 📡 SPI传输: %.1fms (%.1fKB/s)\n", spi_time, (153.6f/spi_time)*1000);
                
                // 性能瓶颈提示
                if (spi_time > clear_time * 5) {
                    printf("      💡 SPI传输是显示瓶颈，建议提高SPI时钟频率\n");
                } else if (clear_time > spi_time * 2) {
                    printf("      💡 清屏操作较慢，可能是显存访问瓶颈\n");
                }
                
                if (convert_time > alloc_time * 10) {
                    printf("   💡 RGB转换是处理瓶颈，建议使用查找表或硬件加速\n");
                }
            }
            
            // ========== 步骤5：帧率控制延迟 ==========
            struct timeval delay_start, delay_end;
            float delay_time = 0.0f;
            
            if (frame_delay_us > 0) {
                gettimeofday(&delay_start, NULL);
                uint32_t processing_time_us = total_frame_time * 1000;
                if (processing_time_us < frame_delay_us) {
                    usleep(frame_delay_us - processing_time_us);
                }
                gettimeofday(&delay_end, NULL);
                delay_time = (delay_end.tv_sec - delay_start.tv_sec) * 1000.0f +
                            (delay_end.tv_usec - delay_start.tv_usec) / 1000.0f;
                total_delay_time += delay_time;
                
                if (anim_params->show_performance && delay_time > 0.1f) {
                    printf("   └─ ⏱️  延迟: %.1fms\n", delay_time);
                }
            }
        }
        
        // ping-pong模式方向切换
        if (anim_params->pingpong_mode) {
            forward_direction = !forward_direction;
            // 一个完整的ping-pong周期算作一次循环
            if (forward_direction) {
                current_loop++;
            }
        } else {
            // 普通循环模式
            current_loop++;
        }
        
        // 显示循环进度
        if (anim_params->loop_count > 0) {
            printf("🔄 完成循环 %d/%d\n", current_loop, anim_params->loop_count);
        }
    }
    
    // 计算总体统计
    struct timeval sequence_end;
    gettimeofday(&sequence_end, NULL);
    
    float total_time = (sequence_end.tv_sec - sequence_start.tv_sec) * 1000.0f +
                      (sequence_end.tv_usec - sequence_start.tv_usec) / 1000.0f;
    
    float avg_fps = total_frames_shown * 1000.0f / total_time;
    float avg_frame_time = total_processing_time / total_frames_shown;
    
    printf("\n 序列播放统计:\n");
    printf("    总帧数: %d 帧\n", total_frames_shown);
    printf("    总时长: %.2f 秒\n", total_time / 1000.0f);
    printf("    平均FPS: %.1f\n", avg_fps);
    printf("    平均帧时间: %.1f ms\n", avg_frame_time);
    printf("    最快帧: %.1f ms\n", min_frame_time);
    printf("    最慢帧: %.1f ms\n", max_frame_time);
    
    printf("\n详细时间分解 (平均每帧):\n");
    float avg_load_time = total_load_time / total_frames_shown;
    float avg_process_time = total_process_time / total_frames_shown;
    float avg_display_time = total_display_time / total_frames_shown;
    float avg_delay_time = total_delay_time / total_frames_shown;
    
    printf("    图片加载: %.1f ms (%.1f%%)\n", avg_load_time, (avg_load_time/avg_frame_time)*100);
    printf("    数据处理: %.1f ms (%.1f%%)\n", avg_process_time, (avg_process_time/avg_frame_time)*100);
    printf("      ├─ 内存操作: ~%.1f ms\n", (avg_process_time * 0.1f));  // 估算内存分配+释放时间
    printf("      └─ RGB转换: ~%.1f ms\n", (avg_process_time * 0.9f));    // 估算RGB转换时间
    printf("   📺 显示传输: %.1f ms (%.1f%%)\n", avg_display_time, (avg_display_time/avg_frame_time)*100);
    printf("      ├─ 清屏操作: ~%.1f ms\n", (avg_display_time * 0.2f));   // 估算清屏时间
    printf("      └─ SPI传输: ~%.1f ms (%.1f MB/s)\n", 
           (avg_display_time * 0.8f), (153.6f/(avg_display_time * 0.8f)));  // 估算SPI传输时间和速度
    if (avg_delay_time > 0.1f) {
        printf("   ⏱️  帧率延迟: %.1f ms (%.1f%%)\n", avg_delay_time, (avg_delay_time/(avg_frame_time+avg_delay_time))*100);
    }
    
    printf("\n💡 详细性能分析:\n");
    
    // 找出最耗时的环节
    if (avg_load_time > avg_process_time && avg_load_time > avg_display_time) {
        printf("   🔍 主要瓶颈：图片加载 (%.1fms)\n", avg_load_time);
        printf("      💡 建议：使用更快的存储设备或预加载图片到内存\n");
    } else if (avg_process_time > avg_display_time) {
        printf("   🔍 主要瓶颈：数据处理 (%.1fms)\n", avg_process_time);
        float estimated_rgb_time = avg_process_time * 0.9f;
        if (estimated_rgb_time > 15.0f) {
            printf("      RGB转换过慢 (%.1fms)：建议使用查找表或SIMD指令\n", estimated_rgb_time);
        }
        printf("      可考虑：预处理图片为4位格式存储\n");
    } else {
        printf("   🔍 主要瓶颈：显示传输 (%.1fms)\n", avg_display_time);
        float estimated_spi_time = avg_display_time * 0.8f;
        float current_spi_speed = 153.6f / estimated_spi_time;  // MB/s
        printf("      当前SPI速度：%.1f MB/s\n", current_spi_speed);
        if (current_spi_speed < 15.0f) {
            printf("      建议：提高SPI时钟频率 (当前可能低于20MHz)\n");
        }
        if (estimated_spi_time > 60.0f) {
            printf("      建议：使用DMA传输减少CPU占用\n");
        }
    }
    
    // CPU利用率分析
    float cpu_utilization = ((avg_load_time + avg_process_time + avg_display_time) / avg_frame_time) * 100.0f;
    printf("    CPU利用率: %.1f%%\n", cpu_utilization);
    
    // 优化潜力分析
    if (cpu_utilization < 50.0f) {
        printf("    CPU利用率较低，可考虑并行处理或提高目标帧率\n");
    } else if (cpu_utilization > 90.0f) {
        printf("    CPU负载较高，建议优化算法或降低处理复杂度\n");
    }
    
    // 具体优化建议
    printf("\n🛠️  具体优化建议:\n");
    if (avg_process_time > 20.0f) {
        printf("   1 RGB转换优化：使用位运算代替除法，或预计算查找表\n");
    }
    if (avg_display_time > 60.0f) {
        printf("   2 SPI优化：检查时钟频率设置，考虑使用DMA传输\n");
    }
    if (avg_load_time > 10.0f) {
        printf("   3 I/O优化：使用RAM磁盘或预加载图片数据\n");
    }
    printf("   4 格式优化：考虑存储预处理的4位格式图片\n");
    printf("   5 硬件优化：使用专用图形处理器或FPGA加速\n");
    
    if (anim_params->target_fps > 0) {
        float efficiency = (anim_params->target_fps / avg_fps) * 100.0f;
        printf("    帧率达成度: %.1f%%\n", efficiency > 100 ? 100.0f : efficiency);
        
        if (avg_fps < anim_params->target_fps * 0.9f) {
            printf("    建议：降低目标帧率或优化图片大小\n");
        } else if (avg_fps > anim_params->target_fps * 1.1f) {
            printf("    建议：可以提高目标帧率或增加图片复杂度\n");
        }
    }
    
    printf("✅ 序列播放完成！\n\n");
    return 0;
}

/**
 * 序列图播放演示
 * @param directory 图片目录路径
 */
void demo_image_sequence(const char* directory) {
    printf("\n🎬 ============= 序列图播放演示 =============\n\n");
    
    char** filenames = NULL;
    int count = 0;
    
    // 加载图片序列
    if (load_image_sequence(directory, &filenames, &count) != 0) {
        return;
    }
    
    // 演示1：最快速度ping-pong播放
    printf("🚀 演示1：最快速度Ping-Pong播放 (5次完整循环)\n");
    struct sequence_animation_t fast_play = {
        .target_fps = 0.0f,        // 最快速度
        .pingpong_mode = true,     // ping-pong模式
        .loop_count = 5,           // 播放5次完整循环
        .show_performance = true   // 显示详细性能
    };
    play_image_sequence(filenames, count, &fast_play);
    
    usleep(1000 * 1000);  // 等待1秒
    
    // 演示2：固定帧率播放
    printf("⏱️  演示2：5 FPS固定帧率播放 (1次循环)\n");
    struct sequence_animation_t fixed_fps = {
        .target_fps = 5.0f,        // 5 FPS
        .pingpong_mode = false,    // 普通循环
        .loop_count = 1,           // 播放1次
        .show_performance = true   // 显示性能
    };
    play_image_sequence(filenames, count, &fixed_fps);
    
    // 清理内存
    free_image_sequence(filenames, count);
    
    printf("🎉 序列图播放演示完成！\n\n");
}

