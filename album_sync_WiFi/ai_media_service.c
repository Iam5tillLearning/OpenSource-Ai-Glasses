#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <dirent.h> // 新增：用于文件夹扫描

// 消息类型定义
#define MSG_CONFIG       0x01
#define MSG_IMAGE_START  0x10
#define MSG_IMAGE_DATA   0x11
#define MSG_IMAGE_END    0x12
#define MSG_IMAGE_ACK    0x13
#define MSG_CLIENT_INFO  0x20  // 客户端上传信息
#define MSG_CLIENT_QUIT  0x21  // 客户端请求断开连接
#define MSG_RESTART_IMAGE 0x22  // 客户端请求重新传图
#define MSG_REQUEST_IMAGE 0x23  // 客户端请求发送图片（新增）
#define MSG_SAVE_TO_ALBUM 0x24  // 客户端请求相册同步（新增）
#define MSG_ALBUM_SYNC_START 0x25  // 相册同步开始
#define MSG_ALBUM_SYNC_END   0x26  // 相册同步结束
// 新增系统更新相关消息类型
#define MSG_UPDATE_START 0x30  // 系统更新开始
#define MSG_UPDATE_DATA  0x31  // 系统更新数据
#define MSG_UPDATE_END   0x32  // 系统更新结束
#define MSG_UPDATE_ACK   0x33  // 系统更新确认
#define IMAGE_CHUNK_SIZE 4096
#define SOCKET_TIMEOUT_SEC 30
#define DEVICE_LISTEN_PORT 8080  // 设备端固定监听端口

// GPIO相关定义
#define GPIO_DEBUG_PATH "/sys/kernel/debug/gpio"  // GPIO调试文件路径
#define GPIO_NUMBER 75                            // 要监控的GPIO编号
#define GPIO_POLL_INTERVAL 50                    // GPIO状态检查间隔(ms)

// IPC相关定义 - 与launch.cpp保持一致
#define SHM_NAME "/display_shm"       // 共享内存名称
#define SEM_NAME "/display_sem"       // 信号量名称
#define BUFFER_SIZE 128               // 缓冲区大小

static int gRecorderExit = 0;

// 共享内存和信号量指针
static char *shared_memory = NULL;
static sem_t *semaphore = NULL;

// 函数声明
static int socket_send_message(int sockfd, unsigned char msg_type, const void *data, unsigned int data_len);
static int socket_receive_message(int sockfd, unsigned char *msg_type, void *data, unsigned int *data_len, unsigned int max_len);
static int send_image_to_client(int client_fd, const void *image_data, size_t image_size);
static int communicate_with_phone(int client_fd, const char *image_path);
static int start_device_server(const char *image_path);
static int read_gpio_state(const char *gpio_debug_path, int gpio_number);
static int wait_for_gpio_press(void);
static int init_ipc(void);
static void cleanup_ipc(void);
static void send_to_display(const char *message);
static int handle_system_update(int client_fd, const char *update_filename, size_t update_size);
static int scan_album_folder(const char *folder_path, char ***file_list, int *file_count);  // 新增
static int send_album_sync(int client_fd, const char *folder_path);  // 新增
static int send_single_file_streaming(int client_fd, const char *file_path);  // 新增声明

// 重新加载图片数据
static int reload_image_data(const char *image_path, void **image_data, size_t *image_size) {
    FILE *fp;
    struct stat file_stat;
    
    // 检查文件是否存在
    if (stat(image_path, &file_stat) != 0) {
        printf("❌ [IMAGE] 图像文件不存在: %s\n", image_path);
        return -1;
    }
    
    // 释放旧数据
    if (*image_data) {
        free(*image_data);
        *image_data = NULL;
    }
    
    // 重新分配内存
    *image_size = file_stat.st_size;
    *image_data = malloc(*image_size);
    if (*image_data == NULL) {
        printf("❌ [IMAGE] 分配图像内存失败\n");
        return -1;
    }
    
    // 读取文件
    fp = fopen(image_path, "rb");
    if (fp == NULL || fread(*image_data, 1, *image_size, fp) != *image_size) {
        printf("❌ [IMAGE] 读取图像文件失败\n");
        free(*image_data);
        *image_data = NULL;
        if (fp) fclose(fp);
        return -1;
    }
    fclose(fp);
    
    printf("✅ [IMAGE] 图片重新加载成功: %zu 字节\n", *image_size);
    return 0;
}

// 发送Socket消息
static int socket_send_message(int sockfd, unsigned char msg_type, const void *data, unsigned int data_len) {
    unsigned char header[5];
    ssize_t sent_bytes;
    
    // 构建消息头：消息类型(1字节) + 数据长度(4字节，网络字节序)
    header[0] = msg_type;
    header[1] = (data_len >> 24) & 0xFF;
    header[2] = (data_len >> 16) & 0xFF;
    header[3] = (data_len >> 8) & 0xFF;
    header[4] = data_len & 0xFF;
    
    printf("📤 [SEND] 发送消息: 类型=0x%02X, 数据长度=%u\n", msg_type, data_len);
    
    // 发送消息头
    sent_bytes = send(sockfd, header, 5, 0);
    if (sent_bytes != 5) {
        printf("❌ [ERROR] 发送消息头失败: %s\n", strerror(errno));
        return -1;
    }
    
    // 发送数据（如果有的话）
    if (data_len > 0 && data != NULL) {
        sent_bytes = send(sockfd, data, data_len, 0);
        if (sent_bytes != (ssize_t)data_len) {
            printf("❌ [ERROR] 发送消息数据失败: %s\n", strerror(errno));
            return -1;
        }
    }
    
    printf("✅ [SEND] 消息发送成功\n");
    return 0;
}

// 接收Socket消息
static int socket_receive_message(int sockfd, unsigned char *msg_type, void *data, unsigned int *data_len, unsigned int max_len) {
    unsigned char header[5];
    ssize_t received_bytes;
    unsigned int payload_len;
    
    // 设置接收超时
    struct timeval timeout;
    timeout.tv_sec = SOCKET_TIMEOUT_SEC;
    timeout.tv_usec = 0;
    
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    
    // 检查socket是否有数据可读
    int select_result = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
    if (select_result <= 0) {
        if (select_result == 0) {
            printf("⚠️  [TIMEOUT] Socket接收超时 (%d秒)\n", SOCKET_TIMEOUT_SEC);
        } else {
            printf("❌ [ERROR] Socket select失败: %s\n", strerror(errno));
        }
        return -1;
    }
    
    // 接收消息头（5字节）
    received_bytes = recv(sockfd, header, 5, MSG_WAITALL);
    if (received_bytes != 5) {
        printf("❌ [ERROR] 接收消息头失败: %s\n", strerror(errno));
        return -1;
    }
    
    // 解析消息头
    *msg_type = header[0];
    payload_len = (header[1] << 24) | (header[2] << 16) | (header[3] << 8) | header[4];
    
    printf("📥 [RECV] 接收消息: 类型=0x%02X, 数据长度=%u\n", *msg_type, payload_len);
    
    // 检查数据长度是否合理
    if (payload_len > max_len) {
        printf("❌ [ERROR] 数据长度超出限制: %u > %u\n", payload_len, max_len);
        return -1;
    }
    
    // 接收数据（如果有的话）
    if (payload_len > 0) {
        received_bytes = recv(sockfd, data, payload_len, MSG_WAITALL);
        if (received_bytes != (ssize_t)payload_len) {
            printf("❌ [ERROR] 接收消息数据失败: %s\n", strerror(errno));
            return -1;
        }
    }
    
    *data_len = payload_len;
    printf("✅ [RECV] 消息接收成功\n");
    return 0;
}

// 发送图像的函数
static int send_image_to_client(int client_fd, const void *image_data, size_t image_size) {
    char config_json[512];
    const char *data_ptr = (const char*)image_data;
    size_t remaining = image_size;
    int chunk_count = 0;
    unsigned char ack_msg_type;
    char ack_buffer[256];
    unsigned int ack_len;

    // 2.1 发送配置消息
    snprintf(config_json, sizeof(config_json), "{\"type\":\"image\",\"format\":\"jpeg\",\"size\":%zu}", image_size);
    if (socket_send_message(client_fd, MSG_CONFIG, config_json, strlen(config_json)) != 0) {
        printf("❌ [COMM] 发送配置消息失败\n");
        return -1;
    }

    // 2.2 发送图像开始消息
    if (socket_send_message(client_fd, MSG_IMAGE_START, NULL, 0) != 0) {
        printf("❌ [COMM] 发送图像开始消息失败\n");
        return -1;
    }

    // 2.3 分块发送图像数据
    while (remaining > 0 && !gRecorderExit) {
        size_t chunk_size = (remaining > IMAGE_CHUNK_SIZE) ? IMAGE_CHUNK_SIZE : remaining;
        if (socket_send_message(client_fd, MSG_IMAGE_DATA, data_ptr, chunk_size) != 0) {
            printf("❌ [COMM] 发送图像块 %d 失败\n", chunk_count);
            return -1;
        }
        data_ptr += chunk_size;
        remaining -= chunk_size;
        chunk_count++;

        // 打印进度
        if (chunk_count % 20 == 0 || remaining == 0) {
            float progress = (float)(image_size - remaining) / image_size * 100.0f;
            printf("📊 [COMM] 发送进度: %.1f%% (%zu/%zu 字节)\n", progress, image_size - remaining, image_size);
        }
    }

    // 2.4 发送图像结束消息
    if (socket_send_message(client_fd, MSG_IMAGE_END, NULL, 0) != 0) {
        printf("❌ [COMM] 发送图像结束消息失败\n");
        return -1;
    }

    // 2.5 等待手机端的确认消息
    if (socket_receive_message(client_fd, &ack_msg_type, ack_buffer, &ack_len, sizeof(ack_buffer)) == 0) {
        if (ack_msg_type == MSG_IMAGE_ACK) {
            printf("✅ [COMM] 收到手机确认，图像发送完成\n");
            return 0;
        } else {
            printf("⚠️ [COMM] 收到未知确认类型: 0x%02X\n", ack_msg_type);
            return 0;  // 兼容：即使类型不对，数据发送成功也视为完成
        }
    } else {
        printf("⚠️ [COMM] 未收到手机确认，但数据已发送\n");
        return 0;
    }
}

// 新增：处理系统更新文件
static int handle_system_update(int client_fd, const char *update_filename, size_t update_size) {
    FILE *fp;
    char update_path[256];
    size_t received_bytes = 0;
    unsigned char msg_type;
    char data_buffer[4096];
    unsigned int data_len;
    int chunk_count = 0;
    
    // 构建更新文件路径
    snprintf(update_path, sizeof(update_path), "/tmp/%s", update_filename);
    
    printf("📁 [UPDATE] 开始接收系统更新文件: %s (预期大小: %zu 字节)\n", update_filename, update_size);
    
    // 创建更新文件
    fp = fopen(update_path, "wb");
    if (fp == NULL) {
        printf("❌ [UPDATE] 创建更新文件失败: %s\n", strerror(errno));
        return -1;
    }
    
    // 接收更新数据块
    while (received_bytes < update_size && !gRecorderExit) {
        if (socket_receive_message(client_fd, &msg_type, data_buffer, &data_len, sizeof(data_buffer)) != 0) {
            printf("❌ [UPDATE] 接收更新数据失败\n");
            fclose(fp);
            unlink(update_path); // 删除不完整的文件
            return -1;
        }
        
        if (msg_type == MSG_UPDATE_DATA) {
            // 写入数据到文件
            if (fwrite(data_buffer, 1, data_len, fp) != data_len) {
                printf("❌ [UPDATE] 写入更新数据失败\n");
                fclose(fp);
                unlink(update_path);
                return -1;
            }
            
            received_bytes += data_len;
            chunk_count++;
            
            // 打印进度
            if (chunk_count % 20 == 0 || received_bytes == update_size) {
                float progress = (float)received_bytes / update_size * 100.0f;
                printf("📊 [UPDATE] 接收进度: %.1f%% (%zu/%zu 字节)\n", progress, received_bytes, update_size);
            }
        } else if (msg_type == MSG_UPDATE_END) {
            printf("✅ [UPDATE] 收到更新结束消息\n");
            break;
        } else {
            printf("⚠️ [UPDATE] 收到未知消息类型: 0x%02X\n", msg_type);
        }
    }
    
    fclose(fp);
    
    if (received_bytes == update_size) {
        printf("✅ [UPDATE] 系统更新文件接收完成: %s (%zu 字节)\n", update_path, received_bytes);
        
        // 发送确认消息
        socket_send_message(client_fd, MSG_UPDATE_ACK, NULL, 0);
        
        // 这里可以添加实际的系统更新逻辑
        // 例如：验证文件、解压、安装等
        printf("📝 [UPDATE] 更新文件已保存到: %s\n", update_path);
        printf("📝 [UPDATE] 请手动执行系统更新操作\n");
        
        // 发送消息到display显示更新完成
        char display_msg[256];
        snprintf(display_msg, sizeof(display_msg), "UPDATE: %s received (%zu bytes)", update_filename, received_bytes);
        send_to_display(display_msg);
        
        return 0;
    } else {
        printf("❌ [UPDATE] 更新文件接收不完整: %zu/%zu 字节\n", received_bytes, update_size);
        unlink(update_path);
        return -1;
    }
}

// 修复：扫描相册文件夹
static int scan_album_folder(const char *folder_path, char ***file_list, int *file_count) {
    printf("📁 [ALBUM] 扫描相册文件夹: %s\n", folder_path);

    // 参数检查
    if (!folder_path || !file_list || !file_count) {
        fprintf(stderr, "错误: 参数不能为NULL\n");
        return -1;
    }
    
    // 打开目录
    DIR *dir = opendir(folder_path);
    if (!dir) {
        fprintf(stderr, "错误: 无法打开目录 '%s' (%s)\n", folder_path, strerror(errno));
        return -1;
    }
    
    // 初始化变量
    int count = 0;
    int capacity = 32; // 初始容量
    char **files = malloc(capacity * sizeof(char *));
    if (!files) {
        closedir(dir);
        fprintf(stderr, "错误: 内存分配失败\n");
        return -1;
    }
    
    struct dirent *entry;
    int has_files = 0;
    
    // 遍历目录
    while ((entry = readdir(dir)) != NULL) {
        // 只处理普通文件
        if (entry->d_type != DT_REG) {
            continue;
        }
        
        char *filename = entry->d_name;
        char *dot = strrchr(filename, '.');
        
        // 检查文件扩展名
        if (!dot) continue;
        
        int is_jpg = (strcasecmp(dot, ".jpg") == 0);
        int is_h264 = (strcasecmp(dot, ".h264") == 0);
        
        if (!is_jpg && !is_h264) {
            continue;
        }
        
        has_files = 1;
        
        // 动态扩容
        if (count >= capacity) {
            capacity *= 2;
            char **new_files = realloc(files, capacity * sizeof(char *));
            if (!new_files) {
                // 内存分配失败，清理已分配的资源
                for (int i = 0; i < count; i++) {
                    free(files[i]);
                }
                free(files);
                closedir(dir);
                fprintf(stderr, "错误: 内存重新分配失败\n");
                return -1;
            }
            files = new_files;
        }
        
        // 构建完整文件路径
        int path_length = strlen(folder_path) + strlen(filename) + 2; // +2 用于 '/' 和 '\0'
        files[count] = malloc(path_length);
        if (!files[count]) {
            // 内存分配失败，清理已分配的资源
            for (int i = 0; i < count; i++) {
                free(files[i]);
            }
            free(files);
            closedir(dir);
            fprintf(stderr, "错误: 文件路径内存分配失败\n");
            return -1;
        }
        
        snprintf(files[count], path_length, "%s/%s", folder_path, filename);
        count++;
        
        printf("找到文件: %s\n", filename);
    }
    
    closedir(dir);
    
    // 设置输出参数
    if (has_files) {
        *file_list = files;
        *file_count = count;
    } else {
        // 没有找到文件，释放内存并设置为NULL
        free(files);
        *file_list = NULL;
        *file_count = 0;
    }
    
    printf("扫描完成: 在 '%s' 中找到 %d 个文件\n", folder_path, count);
    return 0;
}

// 修复：发送相册同步
static int send_album_sync(int client_fd, const char *folder_path) {
    char **file_list = NULL;
    int file_count = 0;
    int i;
    
    // 扫描文件夹
    if (scan_album_folder(folder_path, &file_list, &file_count) != 0) {
        printf("❌ [ALBUM] 扫描文件夹失败\n");
        return -1;
    }
    
    if (file_count == 0) {
        printf("📁 [ALBUM] 文件夹中没有找到图片文件\n");
        // 修复：检查file_list是否为NULL
        if (file_list != NULL) {
            free(file_list);
        }
        return 0;
    }
    
    // 发送相册同步开始消息
    char sync_info[64];
    snprintf(sync_info, sizeof(sync_info), "album_sync:%d", file_count);
    if (socket_send_message(client_fd, MSG_ALBUM_SYNC_START, sync_info, strlen(sync_info)) != 0) {
        printf("❌ [ALBUM] 发送同步开始消息失败\n");
        // 修复：释放所有分配的内存
        for (i = 0; i < file_count; i++) {
            if (file_list[i] != NULL) {  // 添加NULL检查
                free(file_list[i]);
            }
        }
        free(file_list);
        return -1;
    }
    
    // 逐个发送文件
    for (i = 0; i < file_count; i++) {
        char *file_path = file_list[i];
        printf("📤 [ALBUM] 发送文件 %d/%d: %s\n", i + 1, file_count, file_path);
        
        // 发送单个文件
        if (send_single_file_streaming(client_fd, file_path) != 0) {
            printf("❌ [ALBUM] 发送文件失败: %s\n", file_path);
            continue;
        }
        
        printf("✅ [ALBUM] 文件发送完成: %s\n", file_path);
    }
    
    // 发送相册同步结束消息
    if (socket_send_message(client_fd, MSG_ALBUM_SYNC_END, NULL, 0) != 0) {
        printf("❌ [ALBUM] 发送同步结束消息失败\n");
    }
    
    printf("🎉 [ALBUM] 相册同步完成，共发送 %d 个文件\n", file_count);
    
    // 修复：释放所有分配的内存
    for (i = 0; i < file_count; i++) {
        if (file_list[i] != NULL) {  // 添加NULL检查
            free(file_list[i]);
        }
    }
    free(file_list);
    return 0;
}

// 优化：流式发送单个文件（不占用大量内存）
static int send_single_file_streaming(int client_fd, const char *file_path) {
    FILE *fp;
    struct stat file_stat;
    size_t file_size;
    char buffer[IMAGE_CHUNK_SIZE];  // 只使用4KB缓冲区
    size_t bytes_read;
    int chunk_count = 0;
    unsigned char ack_msg_type;
    char ack_buffer[256];
    unsigned int ack_len;

    // 检查文件是否存在
    if (stat(file_path, &file_stat) != 0) {
        printf("❌ [FILE] 文件不存在: %s\n", file_path);
        return -1;
    }
    file_size = file_stat.st_size;
    printf("📊 [FILE] 文件大小: %zu 字节\n", file_size);

    // 打开文件（保持打开状态）
    fp = fopen(file_path, "rb");
    if (fp == NULL) {
        printf("❌ [FILE] 打开文件失败: %s\n", strerror(errno));
        return -1;
    }

    // 发送配置消息（包含文件名）
    char config_json[512];
    const char *filename = strrchr(file_path, '/');
    filename = filename ? filename + 1 : file_path; // 获取文件名
    
    snprintf(config_json, sizeof(config_json), 
             "{\"type\":\"file\",\"format\":\"binary\",\"size\":%zu,\"filename\":\"%s\"}", 
             file_size, filename);
    
    if (socket_send_message(client_fd, MSG_CONFIG, config_json, strlen(config_json)) != 0) {
        printf("❌ [FILE] 发送配置消息失败\n");
        fclose(fp);
        return -1;
    }

    // 发送文件开始消息
    if (socket_send_message(client_fd, MSG_IMAGE_START, NULL, 0) != 0) {
        printf("❌ [FILE] 发送文件开始消息失败\n");
        fclose(fp);
        return -1;
    }

    // 流式读取和发送
    size_t total_sent = 0;
    while ((bytes_read = fread(buffer, 1, IMAGE_CHUNK_SIZE, fp)) > 0 && !gRecorderExit) {
        if (socket_send_message(client_fd, MSG_IMAGE_DATA, buffer, bytes_read) != 0) {
            printf("❌ [FILE] 发送文件块 %d 失败\n", chunk_count);
            fclose(fp);
            return -1;
        }
        
        total_sent += bytes_read;
        chunk_count++;

        // 打印进度
        if (chunk_count % 20 == 0 || bytes_read < IMAGE_CHUNK_SIZE) {
            float progress = (float)total_sent / file_size * 100.0f;
            printf("📊 [FILE] 发送进度: %.1f%% (%zu/%zu 字节)\n", progress, total_sent, file_size);
        }
    }

    fclose(fp);

    // 发送文件结束消息
    if (socket_send_message(client_fd, MSG_IMAGE_END, NULL, 0) != 0) {
        printf("❌ [FILE] 发送文件结束消息失败\n");
        return -1;
    }

    // 等待手机端的确认消息
    if (socket_receive_message(client_fd, &ack_msg_type, ack_buffer, &ack_len, sizeof(ack_buffer)) == 0) {
        if (ack_msg_type == MSG_IMAGE_ACK) {
            printf("✅ [FILE] 收到手机确认，文件发送完成\n");
            return 0;
        } else {
            printf("⚠️ [FILE] 收到未知确认类型: 0x%02X\n", ack_msg_type);
            return 0;
        }
    } else {
        printf("⚠️ [FILE] 未收到手机确认，但数据已发送\n");
        return 0;
    }
}

// 修改communicate_with_phone函数
static int communicate_with_phone(int client_fd, const char *image_path) {
    void *image_data = NULL;
    size_t image_size = 0;
    int result = -1;

    printf("✅ [COMM] 客户端已连接，等待指令...\n");
    
    unsigned char msg_type;
    char info_buffer[1024];
    unsigned int info_len;
    
    while (!gRecorderExit) {
        // 设置较短的超时时间，以便能够及时响应退出信号
        struct timeval timeout;
        timeout.tv_sec = 5;  // 5秒超时
        timeout.tv_usec = 0;
        
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(client_fd, &readfds);
        
        int select_result = select(client_fd + 1, &readfds, NULL, NULL, &timeout);
        if (select_result <= 0) {
            if (select_result == 0) {
                // 超时，继续循环等待
                continue;
            } else {
                printf("❌ [COMM] 监听客户端消息时select失败: %s\n", strerror(errno));
                break;
            }
        }
        
        // 接收客户端消息
        if (socket_receive_message(client_fd, &msg_type, info_buffer, &info_len, sizeof(info_buffer)) == 0) {
            switch (msg_type) {
                case MSG_REQUEST_IMAGE:
                    // 客户端请求发送图片
                    printf("📷 [CLIENT] 客户端请求发送图片\n");
                    if (reload_image_data(image_path, &image_data, &image_size) == 0) {
                        result = send_image_to_client(client_fd, image_data, image_size);
                        if (result != 0) {
                            printf("❌ [COMM] 发送图片失败\n");
                            goto cleanup;
                        }
                        printf("✅ [COMM] 图片发送完成，继续监听客户端消息...\n");
                    } else {
                        printf("❌ [COMM] 图片加载失败\n");
                    }
                    break;
                    
                case MSG_SAVE_TO_ALBUM:
                    // 客户端请求相册同步
                    printf("📁 [CLIENT] 客户端请求相册同步\n");
                    result = send_album_sync(client_fd, "/userdata/Rec");
                    if (result == 0) {
                        printf("✅ [COMM] 相册同步完成，继续监听客户端消息...\n");
                    } else {
                        printf("❌ [COMM] 相册同步失败\n");
                    }
                    break;
                    
                case MSG_CLIENT_INFO:
                    // 客户端上传需要显示的信息，打印到bash并转发到共享内存
                    info_buffer[info_len] = '\0';  // 确保字符串结束
                    printf("📝 [CLIENT] 收到客户端信息: %s\n", info_buffer);
                    
                    // 转发到共享内存给display程序
                    send_to_display(info_buffer);
                    break;
                    
                case MSG_RESTART_IMAGE:
                    // 客户端请求重新传图
                    printf("🔄 [CLIENT] 客户端请求重新传图\n");
                    if (reload_image_data(image_path, &image_data, &image_size) == 0) {
                        result = send_image_to_client(client_fd, image_data, image_size);
                        if (result != 0) {
                            printf("❌ [COMM] 重新传图失败\n");
                            goto cleanup;
                        }
                        printf("✅ [COMM] 重新传图完成，继续监听客户端消息...\n");
                    }
                    break;
                    
                case MSG_UPDATE_START:
                    // 客户端开始发送系统更新文件
                    info_buffer[info_len] = '\0';
                    printf("📁 [UPDATE] 客户端开始发送系统更新: %s\n", info_buffer);
                    
                    // 解析文件名和大小（假设格式为 "filename:size"）
                    char *filename = info_buffer;
                    char *size_str = strchr(info_buffer, ':');
                    size_t update_size = 0;
                    
                    if (size_str) {
                        *size_str = '\0';
                        size_str++;
                        update_size = strtoul(size_str, NULL, 10);
                    }
                    
                    if (update_size > 0) {
                        int update_result = handle_system_update(client_fd, filename, update_size);
                        if (update_result == 0) {
                            printf("✅ [UPDATE] 系统更新文件接收成功\n");
                        } else {
                            printf("❌ [UPDATE] 系统更新文件接收失败\n");
                        }
                    } else {
                        printf("❌ [UPDATE] 无效的更新文件大小\n");
                    }
                    break;
                    
                case MSG_CLIENT_QUIT:
                    // 客户端请求断开连接
                    printf("👋 [CLIENT] 客户端请求断开连接\n");
                    goto cleanup;
                    
                default:
                    printf("⚠️ [CLIENT] 收到未知消息类型: 0x%02X\n", msg_type);
                    break;
            }
        } else {
            printf("❌ [COMM] 接收客户端消息失败\n");
            break;
        }
    }

cleanup:
    free(image_data);
    return result;
}

// 设备端作为服务器，持续监听连接并发送图像
static int start_device_server(const char *image_path) {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int opt = 1;

    // 1. 创建TCP socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("❌ [SERVER] 创建socket失败: %s\n", strerror(errno));
        return -1;
    }

    // 2. 设置socket选项：允许端口复用
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        printf("❌ [SERVER] 设置socket选项失败: %s\n", strerror(errno));
        close(server_fd);
        return -1;
    }

    // 3. 绑定socket到固定端口
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(DEVICE_LISTEN_PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("❌ [SERVER] 绑定端口 %d 失败: %s\n", DEVICE_LISTEN_PORT, strerror(errno));
        close(server_fd);
        return -1;
    }

    // 4. 开始监听连接
    if (listen(server_fd, 5) < 0) {
        printf("❌ [SERVER] 监听连接失败: %s\n", strerror(errno));
        close(server_fd);
        return -1;
    }
    printf("✅ [SERVER] 设备端已启动，监听端口 %d，等待手机连接...\n", DEVICE_LISTEN_PORT);

    // 5. 持续等待客户端连接
    while (!gRecorderExit) {
        // 接收客户端连接
        if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
            printf("❌ [SERVER] 接收连接失败: %s\n", strerror(errno));
            continue;
        }
        
        // 打印连接的手机IP
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("✅ [SERVER] 手机已连接: IP=%s, 端口=%d\n", client_ip, ntohs(client_addr.sin_port));

        // 与手机端通信：发送图像
        int comm_result = communicate_with_phone(client_fd, image_path);
        if (comm_result == 0) {
            printf("🎉 [SERVER] 与手机通信完成，等待下一次连接...\n");
        } else {
            printf("⚠️  [SERVER] 与手机通信异常，等待下一次连接...\n");
        }

        // 关闭当前客户端连接
        close(client_fd);
        printf("🔌 [SERVER] 已断开与 %s 的连接\n", client_ip);
    }

    // 关闭服务器socket
    close(server_fd);
    printf("🔌 [SERVER] 设备端服务器已关闭\n");
    return 0;
}

// 读取GPIO状态
static int read_gpio_state(const char *gpio_debug_path, int gpio_number) {
    FILE *fp;
    char line[256];
    char gpio_name[32];
    char state[16];
    
    fp = fopen(gpio_debug_path, "r");
    if (!fp) {
        printf("❌ [GPIO] 无法打开GPIO调试文件: %s\n", gpio_debug_path);
        return -1;
    }
    
    // 查找对应的GPIO行
    snprintf(gpio_name, sizeof(gpio_name), "gpio-%d", gpio_number);
    
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, gpio_name)) {
            // 解析GPIO状态 (查找 " in " 或 " out " 后面的状态)
            char *pos = strstr(line, " in ");
            if (!pos) {
                pos = strstr(line, " out ");
            }
            
            if (pos) {
                // 移动到状态位置
                pos += 4; // 跳过 " in " 或 " out "
                while (*pos == ' ') pos++; // 跳过空格
                
                sscanf(pos, "%s", state);
                fclose(fp);
                
                if (strcmp(state, "hi") == 0) {
                    return 1; // 高电平
                } else if (strcmp(state, "lo") == 0) {
                    return 0; // 低电平
                }
            }
            break;
        }
    }
    
    fclose(fp);
    return -1; // 未找到或解析失败
}

// 等待GPIO按下 (lo -> hi)
static int wait_for_gpio_press(void) {
    int current_state, prev_state = -1;
    
    printf(" [GPIO] 等待GPIO-%d按下 (lo -> hi)...\n", GPIO_NUMBER);
    fflush(stdout);
    
    while (!gRecorderExit) {
        current_state = read_gpio_state(GPIO_DEBUG_PATH, GPIO_NUMBER);
        
        if (current_state < 0) {
            printf("❌ [GPIO] 读取GPIO状态失败\n");
            return -1;
        }
        
        // 检测从低到高的变化
        if (prev_state == 0 && current_state == 1) {
            printf("✅ [GPIO] GPIO-%d已按下！\n", GPIO_NUMBER);
            fflush(stdout);
            
            // 发送BLE消息到display
            send_to_display("BLE:4C 41 55 4E 43 48 0A");
            
            return 0; // 成功检测到按下
        }
        
        prev_state = current_state;
        usleep(GPIO_POLL_INTERVAL * 1000); // 转换为微秒
    }
    
    return -1; // 退出时返回失败
}

// 初始化IPC通信
static int init_ipc(void) {
    int retries = 5;
    int shm_fd;
    
    while (retries-- > 0) {
        // 创建共享内存
        shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
        if (shm_fd == -1) {
            printf("❌ [IPC] shm_open失败: %s\n", strerror(errno));
            usleep(500000); // 等待500ms后重试
            continue;
        }
        
        if (ftruncate(shm_fd, BUFFER_SIZE) == -1) {
            printf("❌ [IPC] ftruncate失败: %s\n", strerror(errno));
            close(shm_fd);
            usleep(500000);
            continue;
        }
        
        // 映射共享内存
        shared_memory = (char*)mmap(0, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (shared_memory == MAP_FAILED) {
            printf("❌ [IPC] mmap失败: %s\n", strerror(errno));
            close(shm_fd);
            usleep(500000);
            continue;
        }
        
        close(shm_fd);
        
        // 创建信号量
        semaphore = sem_open(SEM_NAME, O_CREAT, 0666, 0);
        if (semaphore == SEM_FAILED) {
            printf("❌ [IPC] sem_open失败: %s\n", strerror(errno));
            munmap(shared_memory, BUFFER_SIZE);
            usleep(500000);
            continue;
        }
        
        // 初始化共享内存
        memset(shared_memory, 0, BUFFER_SIZE);
        printf("✅ [IPC] IPC初始化成功 (尝试次数: %d)\n", 5 - retries);
        return 0;
    }
    
    printf("❌ [IPC] IPC初始化失败，已尝试5次\n");
    return -1;
}

// 清理IPC资源
static void cleanup_ipc(void) {
    if (shared_memory) {
        munmap(shared_memory, BUFFER_SIZE);
        shm_unlink(SHM_NAME);
        shared_memory = NULL;
    }
    if (semaphore) {
        sem_close(semaphore);
        sem_unlink(SEM_NAME);
        semaphore = NULL;
    }
    printf("✅ [IPC] IPC资源已清理\n");
}

// 发送消息给display
static void send_to_display(const char *message) {
    if (!shared_memory || !semaphore) {
        printf("❌ [IPC] IPC未初始化，无法发送消息\n");
        return;
    }
    
    // 复制消息到共享内存
    strncpy(shared_memory, message, BUFFER_SIZE - 1);
    shared_memory[BUFFER_SIZE - 1] = '\0'; // 确保字符串终止
    
    // 通知display有新消息
    if (sem_post(semaphore) == -1) {
        printf("❌ [IPC] sem_post失败: %s\n", strerror(errno));
    } else {
        printf("✅ [IPC] 消息已发送到display: %s\n", message);
    }
}

int main(int argc, char *argv[]) {
    const char *image_path = "/tmp/123.jpg";  // 默认图像路径
    int opt;

    // 解析命令行参数
    while ((opt = getopt(argc, argv, "f:h")) != -1) {
        switch (opt) {
            case 'f':
                image_path = optarg;
                break;
            case 'h':
                printf("设备端图像服务器使用说明:\n");
                printf("  %s [-f 图像路径] [--help]\n", argv[0]);
                printf("  -f: 指定图像文件路径（默认: /tmp/123.jpg）\n");
                printf("  --help: 显示帮助\n");
                return 0;
            default:
                printf("未知参数，使用 -h 查看帮助\n");
                return 1;
        }
    }

    printf("🚀 [DEVICE] 设备端图像服务器启动\n");
    printf("📁 [CONFIG] 图像路径: %s\n", image_path);
    printf("📡 [CONFIG] 监听端口: %d\n", DEVICE_LISTEN_PORT);
    printf("========================================\n");

    // 初始化IPC通信
    if (init_ipc() != 0) {
        printf("❌ [MAIN] IPC初始化失败，程序退出\n");
        return 1;
    }

    // 发送初始消息到display
    send_to_display("Image Server Started");

    // 启动服务器，持续等待手机连接
    start_device_server(image_path);
    
    // 清理IPC资源
    cleanup_ipc();
    return 0;
}