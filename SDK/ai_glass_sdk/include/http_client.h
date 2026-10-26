#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// HTTP请求方法
typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE
} http_method_t;

// HTTP响应结构
typedef struct {
    int status_code;        // HTTP状态码 (200, 404, etc.)
    char *body;             // 响应体
    size_t body_len;        // 响应体长度
    char *headers;          // 响应头（可选）
    size_t headers_len;     // 响应头长度
} http_response_t;

// HTTP请求配置
typedef struct {
    const char *url;        // 请求URL
    http_method_t method;   // 请求方法
    const char *headers;    // 自定义请求头（可选，格式: "Key1: Value1\r\nKey2: Value2\r\n"）
    const char *body;       // 请求体（POST/PUT用）
    size_t body_len;        // 请求体长度
    int timeout_ms;         // 超时时间（毫秒）
} http_request_t;

/**
 * 执行HTTP请求（同步）
 *
 * @param req 请求配置
 * @param resp 响应结构（需要调用http_response_free释放）
 * @return 0成功，-1失败
 */
int http_request(const http_request_t *req, http_response_t *resp);

/**
 * 释放响应结构
 */
void http_response_free(http_response_t *resp);

/**
 * 便捷函数：HTTP GET
 */
int http_get(const char *url, http_response_t *resp);

/**
 * 便捷函数：HTTP POST（通用）
 *
 * @param url 请求URL
 * @param content_type Content-Type头（如 "text/plain", "application/octet-stream"）
 * @param body 请求体
 * @param body_len 请求体长度
 * @param resp 响应结构
 * @return 0成功，-1失败
 */
int http_post(const char *url, const char *content_type,
              const char *body, size_t body_len, http_response_t *resp);

/**
 * 便捷函数：HTTP POST (JSON)
 */
int http_post_json(const char *url, const char *json_body, http_response_t *resp);

/**
 * 便捷函数：HTTP POST (表单数据)
 * Content-Type: application/x-www-form-urlencoded
 *
 * @param url 请求URL
 * @param form_data URL编码的表单数据（如 "name=value&foo=bar"）
 * @param resp 响应结构
 * @return 0成功，-1失败
 */
int http_post_form(const char *url, const char *form_data, http_response_t *resp);

/**
 * 便捷函数：HTTP PUT
 *
 * @param url 请求URL
 * @param content_type Content-Type头
 * @param body 请求体
 * @param body_len 请求体长度
 * @param resp 响应结构
 * @return 0成功，-1失败
 */
int http_put(const char *url, const char *content_type,
             const char *body, size_t body_len, http_response_t *resp);

/**
 * 便捷函数：HTTP DELETE
 */
int http_delete(const char *url, http_response_t *resp);

#ifdef __cplusplus
}
#endif

#endif // HTTP_CLIENT_H
