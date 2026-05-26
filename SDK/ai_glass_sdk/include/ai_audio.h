#ifndef ai_audio_H
#define ai_audio_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @file ai_audio.h
 * @brief AI Media Service 音频播放客户端API
 * @version 1.0
 * @date 2025-10-10
 */

// =============================================================================
// 音频参数结构
// =============================================================================

/**
 * @brief 音频播放参数
 */
typedef struct {
    const char *file_path;    // PCM文件路径（必填）
    int volume;               // 音量 (0-100)，-1表示使用默认值
    int force;                // 强制播放标志 (0=排队, 1=打断当前播放)
    int sample_rate;          // 采样率 (8000-96000)，-1表示使用默认值
    int channels;             // 声道数 (1-8)，-1表示使用默认值
    int bit_width;            // 位宽 (8/16/24/32)，-1表示使用默认值
} ai_audio_params_t;

/**
 * @brief 音频客户端句柄（不透明类型）
 */
typedef struct ai_audio ai_audio_t;

// =============================================================================
// 错误码定义
// =============================================================================

#define AI_AUDIO_SUCCESS           0    // 成功
#define AI_AUDIO_ERROR_INIT       -1    // 初始化失败
#define AI_AUDIO_ERROR_CONNECT    -2    // 连接失败
#define AI_AUDIO_ERROR_SEND       -3    // 发送失败
#define AI_AUDIO_ERROR_PARAM      -4    // 参数错误
#define AI_AUDIO_ERROR_RESPONSE   -5    // 服务端响应错误
#define AI_AUDIO_ERROR_STATE      -6    // 状态错误（例如重复开始录音）
#define AI_AUDIO_ERROR_TIMEOUT    -7    // 读取超时，可重试

// =============================================================================
// 资源控制定义
// =============================================================================

#define AI_AUDIO_RESOURCE_CAMERA  0x01
#define AI_AUDIO_RESOURCE_AUDIO   0x02
#define AI_AUDIO_RESOURCE_ALL     (AI_AUDIO_RESOURCE_CAMERA | AI_AUDIO_RESOURCE_AUDIO)

#define AI_MEDIA_HOLDER_ID_MAX 64
#define AI_MEDIA_ENDPOINT_PATH_MAX 108

#define AI_AUDIO_STREAM_DEFAULT_SOCKET_PATH "/tmp/ai-core_audio_stream"
#define AI_AUDIO_STREAM_MAGIC 0x41415346u
#define AI_AUDIO_STREAM_VERSION 1
#define AI_AUDIO_STREAM_CODEC_G711A 1
#define AI_AUDIO_STREAM_MAX_PAYLOAD 4096

typedef struct {
    int camera_suspended;    // 1=已释放给外部应用，0=ai-core持有
    int audio_suspended;     // 1=已释放给外部应用，0=ai-core持有
} ai_audio_resource_status_t;

typedef struct {
    int (*release_resources)(int resource_mask, void *user_data);
    int (*acquire_resources)(int resource_mask, void *user_data);
    int (*get_resource_status)(int *camera_owned, int *audio_owned, void *user_data);
} ai_media_holder_ops_t;

typedef struct {
    const char *holder_id;
    int owned_mask;
    int reclaim_timeout_ms;
    ai_media_holder_ops_t ops;
    void *user_data;
} ai_media_holder_registration_t;

typedef struct {
    int holder_registered;
    int owned_mask;
    int reclaim_pending;
    int camera_suspended;
    int audio_suspended;
    int holder_generation;
    int loan_active;
    int loan_auto_return;
    int return_pending;
    char holder_id[AI_MEDIA_HOLDER_ID_MAX];
    char endpoint_path[AI_MEDIA_ENDPOINT_PATH_MAX];
} ai_media_arbitration_status_t;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t version;
    uint16_t header_size;

    uint32_t codec;
    uint32_t sample_rate;
    uint16_t channels;
    uint16_t bits_per_sample;

    uint32_t frame_samples;
    uint32_t payload_size;

    uint64_t capture_ts_us;
    uint64_t seq;
} ai_audio_stream_frame_header_t;

typedef struct {
    ai_audio_stream_frame_header_t header;
    unsigned char payload[AI_AUDIO_STREAM_MAX_PAYLOAD];
} ai_audio_stream_packet_t;

typedef struct {
    int fd;
    char socket_path[AI_MEDIA_ENDPOINT_PATH_MAX];
} ai_audio_stream_handle_t;

// =============================================================================
// 核心API
// =============================================================================

/**
 * @brief 初始化音频客户端
 * @param socket_path Unix Socket路径，NULL表示使用默认路径
 * @return 客户端句柄，失败返回NULL
 */
ai_audio_t* ai_audio_init(const char *socket_path);

/**
 * @brief 播放音频文件
 * @param client 客户端句柄
 * @param params 播放参数
 * @return 0成功，负数表示错误码
 */
int ai_audio_play(ai_audio_t *client, const ai_audio_params_t *params);

/**
 * @brief 停止当前播放
 * @param client 客户端句柄
 * @return 0成功，负数表示错误码
 */
int ai_audio_stop(ai_audio_t *client);

/**
 * @brief 设置是否禁用 ai-core 物理交互动作
 * @param client 客户端句柄
 * @param disabled 1=禁用 AI-Core 物理交互动作并保留 GPIO 事件，0=恢复默认动作
 * @return 0成功，负数表示错误码
 */
int ai_audio_set_disable_aicore_physical_actions(ai_audio_t *client, int disabled);

/**
 * @brief 查询是否禁用 ai-core 物理交互动作
 * @param client 客户端句柄
 * @param disabled 输出参数，1=已禁用，0=未禁用
 * @return 0成功，负数表示错误码
 */
int ai_audio_get_disable_aicore_physical_actions(ai_audio_t *client, int *disabled);

/**
 * @brief 通过SDK命令启动录音（无需物理按键）
 * @param client 客户端句柄
 * @return 0成功，负数表示错误码
 */
int ai_audio_record_start(ai_audio_t *client);

/**
 * @brief 通过SDK命令停止录音并返回录音文件路径
 * @param client 客户端句柄
 * @param output_path 输出参数，可为NULL（不需要路径时）
 * @param output_path_size output_path缓冲区大小
 * @return 0成功，负数表示错误码
 */
int ai_audio_record_stop(ai_audio_t *client, char *output_path, int output_path_size);

/**
 * @brief 查询当前是否正在录音
 * @param client 客户端句柄
 * @param recording 输出参数，1=录音中，0=未录音
 * @return 0成功，负数表示错误码
 */
int ai_audio_record_get_status(ai_audio_t *client, int *recording);

/**
 * @brief 请求 ai-core 释放指定资源（供外部应用接管）
 * @param client 客户端句柄
 * @param resource_mask 资源位掩码，使用 AI_AUDIO_RESOURCE_* 组合
 * @return 0成功，负数表示错误码
 */
int ai_audio_suspend_resources(ai_audio_t *client, int resource_mask);

/**
 * @brief 请求 ai-core 回收指定资源（恢复 ai-core 持有）
 * @param client 客户端句柄
 * @param resource_mask 资源位掩码，使用 AI_AUDIO_RESOURCE_* 组合
 * @return 0成功，负数表示错误码
 */
int ai_audio_resume_resources(ai_audio_t *client, int resource_mask);

/**
 * @brief 查询当前资源持有状态
 * @param client 客户端句柄
 * @param status 输出状态
 * @return 0成功，负数表示错误码
 */
int ai_audio_get_resource_status(ai_audio_t *client, ai_audio_resource_status_t *status);

/**
 * @brief 注册当前外部媒体 holder
 * @param client 客户端句柄
 * @param registration holder 注册信息
 * @return 0成功，负数表示错误码
 */
int ai_media_register_holder(ai_audio_t *client,
                             const ai_media_holder_registration_t *registration);

/**
 * @brief 注销当前外部媒体 holder
 * @param client 客户端句柄
 * @param holder_id holder 标识
 * @return 0成功，负数表示错误码
 */
int ai_media_unregister_holder(ai_audio_t *client, const char *holder_id);

/**
 * @brief 查询当前媒体仲裁状态
 * @param client 客户端句柄
 * @param status 输出状态
 * @return 0成功，负数表示错误码
 */
int ai_media_get_arbitration_status(ai_audio_t *client,
                                    ai_media_arbitration_status_t *status);

int ai_audio_stream_subscribe(const char *socket_path,
                              ai_audio_stream_handle_t *handle);

int ai_audio_stream_read_packet(ai_audio_stream_handle_t *handle,
                                ai_audio_stream_packet_t *packet);

void ai_audio_stream_unsubscribe(ai_audio_stream_handle_t *handle);

/**
 * @brief 清理音频客户端
 * @param client 客户端句柄
 */
void ai_audio_cleanup(ai_audio_t *client);

/**
 * @brief 获取错误信息字符串
 * @param error_code 错误码
 * @return 错误信息字符串
 */
const char* ai_audio_get_error_string(int error_code);

// =============================================================================
// 辅助函数
// =============================================================================

/**
 * @brief 创建默认播放参数
 * @param file_path PCM文件路径
 * @return 参数结构体
 */
ai_audio_params_t ai_audio_create_default_params(const char *file_path);

/**
 * @brief 简化的播放函数（使用默认参数）
 * @param client 客户端句柄
 * @param file_path PCM文件路径
 * @return 0成功，负数表示错误码
 */
int ai_audio_play_simple(ai_audio_t *client, const char *file_path);

// =============================================================================
// TTS API (文本转语音播放)
// =============================================================================

/**
 * @brief TTS播放参数
 */
typedef struct {
    const char *text;         // 待播报的文本（必填）
    int volume;               // 音量 (0-100)，-1表示使用默认值
    int force;                // 强制播放标志 (0=排队, 1=打断当前播放)
    int use_cache;            // 是否使用缓存 (0=不缓存, 1=使用缓存)
} ai_audio_tts_params_t;

/**
 * @brief 播放TTS文本
 * @param client 客户端句柄
 * @param params TTS播放参数
 * @return 0成功，负数表示错误码
 *
 * 工作流程：
 * 1. 客户端通过Unix Socket发送TTS命令到服务端
 * 2. 服务端计算文本MD5并检查缓存
 * 3. 如果缓存未命中，调用云端TTS API生成音频
 * 4. 服务端将生成的PCM文件加入播放队列
 * 5. 播放完成后返回结果
 */
int ai_audio_play_tts(ai_audio_t *client, const ai_audio_tts_params_t *params);

/**
 * @brief 简化的TTS播放函数（使用默认参数）
 * @param client 客户端句柄
 * @param text 待播报的文本
 * @return 0成功，负数表示错误码
 *
 * 默认参数：音量=默认，排队播放，使用缓存
 */
int ai_audio_play_tts_simple(ai_audio_t *client, const char *text);

/**
 * @brief 播放适合短提示的TTS文本
 * @param client 客户端句柄
 * @param text 待播报的文本
 * @return 0成功，负数表示错误码
 *
 * 该接口会先过滤不适合朗读或会破坏TTS命令协议的字符，再使用固定提示音参数播放：
 * 音量=80，排队播放，使用缓存。
 */
int ai_audio_play_toast(ai_audio_t *client, const char *text);

/**
 * @brief 使用默认音频Socket播放适合短提示的TTS文本
 * @param text 待播报的文本
 * @return 0成功，负数表示错误码
 *
 * 等价于创建默认音频客户端后调用 `ai_audio_play_toast()`。
 */
int ai_audio_play_toast_text(const char *text);

/**
 * @brief 创建默认TTS参数
 * @param text 待播报的文本
 * @return 参数结构体
 */
ai_audio_tts_params_t ai_audio_create_default_tts_params(const char *text);

#ifdef __cplusplus
}
#endif

#endif // ai_audio_H
