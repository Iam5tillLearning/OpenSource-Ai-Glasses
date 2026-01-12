#ifndef WS_CLIENT_H
#define WS_CLIENT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// WebSocket事件类型
typedef enum {
    WS_EVENT_CONNECTED,     // 连接成功
    WS_EVENT_DISCONNECTED,  // 连接断开
    WS_EVENT_MESSAGE,       // 收到消息
    WS_EVENT_ERROR          // 错误
} ws_event_type_t;

// WebSocket事件回调
typedef void (*ws_event_cb)(ws_event_type_t event, const char *data, size_t len, void *user_data);

// WebSocket客户端句柄
typedef struct ws_client ws_client_t;

/**
 * 创建WebSocket客户端
 *
 * @param url WebSocket服务器URL (ws://example.com/path 或 wss://...)
 * @param callback 事件回调函数
 * @param user_data 用户数据（回调时传回）
 * @return 客户端句柄，失败返回NULL
 */
ws_client_t *ws_client_create(const char *url, ws_event_cb callback, void *user_data);

/**
 * 发送文本消息
 */
int ws_client_send_text(ws_client_t *client, const char *text);

/**
 * 发送二进制消息
 */
int ws_client_send_binary(ws_client_t *client, const void *data, size_t len);

/**
 * 运行事件循环（阻塞）
 * 通常在独立线程中调用
 */
void ws_client_run(ws_client_t *client);

/**
 * 停止事件循环
 */
void ws_client_stop(ws_client_t *client);

/**
 * 设置SSL/TLS配置 (仅对wss://有效)
 * 注意：证书内容必须是PEM格式的字符串
 *
 * @param client 客户端句柄
 * @param ca_cert CA证书内容 (NULL表示不验证服务器证书)
 * @param cert 客户端证书内容 (可选)
 * @param key 客户端私钥内容 (可选)
 */
void ws_client_set_ssl(ws_client_t *client, const char *ca_cert, const char *cert, const char *key);

/**
 * 设置是否跳过证书验证
 *
 * @param client 客户端句柄
 * @param skip 1表示跳过验证，0表示不跳过
 */
void ws_client_set_skip_verification(ws_client_t *client, int skip);

/**
 * 销毁WebSocket客户端
 */
void ws_client_destroy(ws_client_t *client);

#ifdef __cplusplus
}
#endif

#endif // WS_CLIENT_H
