/**
 * =============================================================================
 * AI Glass SDK - 文本事件客户端API
 * =============================================================================
 *
 * @file ai_text_event.h
 * @brief AI Glass SDK 文本事件接口头文件
 * @version 1.0
 * @date 2026-01-15
 *
 * 【模块概述】
 * 本模块提供接收 ASR（语音转文字）和 LLM（大模型回复）文本流的客户端 API。
 * 通过 Unix Domain Socket 订阅 `ai-core` 广播的文本事件。
 *
 */

#ifndef AI_TEXT_EVENT_H
#define AI_TEXT_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// 常量与枚举定义
// =============================================================================

#define AI_TEXT_EVENT_SOCKET_PATH "/var/run/ai_text_event.sock"

/**
 * 文本来源枚举
 */
typedef enum {
    AI_TEXT_SOURCE_UNKNOWN = 0,
    AI_TEXT_SOURCE_ASR     = 1,  // 用户语音识别结果
    AI_TEXT_SOURCE_LLM     = 2   // AI 回复内容
} ai_text_source_t;

/**
 * 文本类型枚举
 */
typedef enum {
    AI_TEXT_TYPE_UNKNOWN   = 0,
    AI_TEXT_TYPE_PARTIAL   = 1, // 流式中间结果（正在生成）
    AI_TEXT_TYPE_FINAL     = 2  // 最终确定的结果
} ai_text_type_t;

// =============================================================================
// 数据结构定义
// =============================================================================

/**
 * 文本事件结构体
 */
typedef struct {
    int source;     // ai_text_source_t
    int type;       // ai_text_type_t
    char *text;     // 文本内容 (UTF-8)
    int length;     // 内容长度
} ai_text_event_t;

/**
 * 文本事件客户端句柄（不透明类型）
 */
typedef struct ai_text_event_client_ctx ai_text_event_client_t;

/**
 * 事件回调函数原型
 * @param event 接收到的文本事件数据
 * @param user_data 用户注册时传递的上下文指针
 */
typedef void (*ai_text_event_cb)(ai_text_event_t *event, void *user_data);

// =============================================================================
// 核心 API
// =============================================================================

/**
 * 创建文本事件客户端
 * 【功能】分配并初始化客户端上下文
 * 【返回】成功返回客户端句柄，失败返回NULL
 */
ai_text_event_client_t* ai_text_event_client_create(void);

/**
 * 初始化客户端并注册回调
 * 【功能】配置回调函数
 * 【参数】client - 客户端句柄
 *         cb - 事件回调函数
 *         user_data - 用户数据
 * 【返回】0成功，-1失败
 */
int ai_text_event_client_init(ai_text_event_client_t *client, ai_text_event_cb cb, void *user_data);

/**
 * 启动客户端
 * 【功能】连接到服务并开始接收事件（通常会启动一个后台线程）
 * 【参数】client - 客户端句柄
 * 【返回】0成功，-1失败
 */
int ai_text_event_client_start(ai_text_event_client_t *client);

/**
 * 停止客户端
 * 【功能】停止接收并断开连接
 * 【参数】client - 客户端句柄
 */
void ai_text_event_client_stop(ai_text_event_client_t *client);

/**
 * 销毁客户端
 * 【功能】释放资源
 * 【参数】client - 客户端句柄
 */
void ai_text_event_client_destroy(ai_text_event_client_t *client);

#ifdef __cplusplus
}
#endif

#endif // AI_TEXT_EVENT_H
