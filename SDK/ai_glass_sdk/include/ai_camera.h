/**
 * AI Media Client API Header
 *
 * 为外部程序提供AI媒体共享内存接口
 * 支持图像、音频等多媒体数据共享
 *
 * 使用说明:
 * 1. 调用 ai_core_init() 初始化客户端
 * 2. 调用 ai_core_capture() 捕获媒体数据
 * 3. 处理返回的媒体数据
 * 4. 调用 ai_core_cleanup() 清理资源
 */

#ifndef AI_MEDIA_CLIENT_H
#define AI_MEDIA_CLIENT_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// 常量定义
#define AI_MEDIA_CTRL_SOCKET_PATH "/tmp/ai-core_camera_ctrl"
#define AI_MEDIA_SHM_NAME "/ai-core_shm"
#define AI_MEDIA_SHM_SIZE (4 * 1024 * 1024 + 1024)  // 4MB媒体数据 + 1KB控制区域

#define AI_MEDIA_FORMAT_JPEG  0
#define AI_MEDIA_FORMAT_NV12  1

// 错误码定义
#define AI_MEDIA_SUCCESS          0
#define AI_MEDIA_ERROR_INIT      -1
#define AI_MEDIA_ERROR_TIMEOUT   -2
#define AI_MEDIA_ERROR_CAPTURE   -3
#define AI_MEDIA_ERROR_MEMORY    -4

// 媒体数据信息结构体
typedef struct {
    uint8_t *data;          // 媒体数据指针
    size_t size;            // 媒体数据大小（字节）
    int width;              // 宽度（图像数据）
    int height;             // 高度（图像数据）
    int format;             // 数据格式 (0=JPEG, 1=NV12)
    int sequence;           // 帧序号
} ai_core_data_t;

// 客户端上下文(内部使用，用户不需要了解具体内容)
typedef struct ai_core_client_ctx ai_core_client_t;

/**
 * 初始化AI媒体客户端
 * @return 成功返回客户端句柄，失败返回NULL
 */
ai_core_client_t* ai_core_init(void);

/**
 * 捕获媒体数据
 * @param client 客户端句柄
 * @param data 输出媒体数据信息
 * @param timeout_ms 超时时间（毫秒），0表示不超时
 * @return AI_MEDIA_SUCCESS成功，其他值表示错误码
 */
int ai_core_capture(ai_core_client_t *client, ai_core_data_t *data, int timeout_ms);

/**
 * 释放媒体数据内存
 * @param data 媒体数据信息结构体
 */
void ai_core_free_data(ai_core_data_t *data);

/**
 * 清理AI媒体客户端资源
 * @param client 客户端句柄
 */
void ai_core_cleanup(ai_core_client_t *client);

/**
 * 获取错误信息字符串
 * @param error_code 错误码
 * @return 错误信息字符串
 */
const char* ai_core_get_error_string(int error_code);

#ifdef __cplusplus
}
#endif

#endif // AI_MEDIA_CLIENT_H
