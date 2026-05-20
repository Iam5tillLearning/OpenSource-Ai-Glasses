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

#include "ai_ipc.h"

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

#define AI_CAMERA_PHOTO_DIR "/userdata/media/photos"
#define AI_CAMERA_VIDEO_DIR "/userdata/media/videos"
#define AI_CAMERA_ACTION_PATH_MAX 256

// 错误码定义
#define AI_MEDIA_SUCCESS          0
#define AI_MEDIA_ERROR_INIT      -1
#define AI_MEDIA_ERROR_TIMEOUT   -2
#define AI_MEDIA_ERROR_CAPTURE   -3
#define AI_MEDIA_ERROR_MEMORY    -4
#define AI_MEDIA_ERROR_STREAM    -5
#define AI_MEDIA_ERROR_NOT_READY -6

// 媒体数据信息结构体
typedef struct {
    uint8_t *data;          // 媒体数据指针
    size_t size;            // 媒体数据大小（字节）
    int width;              // 宽度（图像数据）
    int height;             // 高度（图像数据）
    int format;             // 数据格式 (0=JPEG, 1=NV12)
    int sequence;           // 帧序号
} ai_core_data_t;

typedef struct {
    int recording;
    char path[AI_CAMERA_ACTION_PATH_MAX];
    uint64_t bytes;
    uint64_t frames;
} ai_camera_video_status_t;

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

/**
 * 请求 ai-core 拍照并保存到设备本地。
 * @param path 输出保存路径，可为 NULL
 * @param path_size path 缓冲区大小
 * @param timeout_ms 超时时间（毫秒），0 使用默认超时
 * @return AI_MEDIA_SUCCESS 成功，其他值表示错误码
 */
int ai_camera_take_photo(char *path, size_t path_size, int timeout_ms);

/**
 * 请求 ai-core 开始 H.265 录像。
 * @param path 输出录像文件路径，可为 NULL
 * @param path_size path 缓冲区大小
 * @return AI_MEDIA_SUCCESS 成功，其他值表示错误码
 */
int ai_camera_video_start(char *path, size_t path_size);

/**
 * 请求 ai-core 停止 H.265 录像。
 * @param status 输出停止后的状态，可为 NULL
 * @return AI_MEDIA_SUCCESS 成功，其他值表示错误码
 */
int ai_camera_video_stop(ai_camera_video_status_t *status);

/**
 * 查询 ai-core H.265 录像状态。
 * @param status 输出录像状态
 * @return AI_MEDIA_SUCCESS 成功，其他值表示错误码
 */
int ai_camera_video_get_status(ai_camera_video_status_t *status);

// =============================================================================
// 编码后视频码流订阅接口
// =============================================================================

#define AI_VIDEO_STREAM_CTRL_SOCKET_PATH "/tmp/ai-core_video_stream_ctrl"
#define AI_VIDEO_STREAM_SHM_NAME "/ai-core_video_stream"
#define AI_VIDEO_STREAM_MAGIC 0x41565348u
#define AI_VIDEO_STREAM_VERSION 1u
#define AI_VIDEO_STREAM_SLOT_COUNT 8u
#define AI_VIDEO_STREAM_SLOT_PAYLOAD_SIZE (512u * 1024u)

#define AI_VIDEO_STREAM_CODEC_H265 1u

#define AI_VIDEO_STREAM_SLOT_EMPTY 0u
#define AI_VIDEO_STREAM_SLOT_WRITING 1u
#define AI_VIDEO_STREAM_SLOT_READY 2u

#define AI_VIDEO_STREAM_FRAME_FLAG_KEYFRAME 0x00000001u

typedef struct {
    uint32_t state;
    uint32_t codec;
    uint32_t flags;
    uint32_t payload_size;
    uint64_t seq;
    uint64_t pts_us;
    uint64_t capture_ts_us;
    uint32_t width;
    uint32_t height;
    uint32_t reserved[8];
} ai_video_stream_slot_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t header_size;
    uint32_t slot_count;
    uint32_t slot_payload_size;
    uint32_t producer_active;
    uint32_t subscriber_count;
    uint32_t codec;
    uint32_t width;
    uint32_t height;
    uint64_t latest_seq;
    ai_video_stream_slot_t slots[AI_VIDEO_STREAM_SLOT_COUNT];
    unsigned char payloads[AI_VIDEO_STREAM_SLOT_COUNT][AI_VIDEO_STREAM_SLOT_PAYLOAD_SIZE];
} ai_video_stream_shm_t;

typedef struct {
    int control_fd;
    ai_shared_memory_t shm;
    ai_video_stream_shm_t *ring;
    uint64_t last_seq;
    uint64_t last_ping_ms;
} ai_video_stream_handle_t;

typedef struct {
    const unsigned char *data;
    size_t size;
    uint32_t codec;
    uint32_t flags;
    uint64_t seq;
    uint64_t pts_us;
    uint64_t capture_ts_us;
    uint32_t width;
    uint32_t height;
    uint32_t slot_index;
} ai_video_stream_frame_t;

int ai_video_stream_subscribe(const char *subscriber_name, ai_video_stream_handle_t *handle);
int ai_video_stream_read_frame(ai_video_stream_handle_t *handle,
                               ai_video_stream_frame_t *frame,
                               int timeout_ms);
void ai_video_stream_release_frame(ai_video_stream_handle_t *handle,
                                   ai_video_stream_frame_t *frame);
void ai_video_stream_unsubscribe(ai_video_stream_handle_t *handle);

#ifdef __cplusplus
}
#endif

#endif // AI_MEDIA_CLIENT_H
