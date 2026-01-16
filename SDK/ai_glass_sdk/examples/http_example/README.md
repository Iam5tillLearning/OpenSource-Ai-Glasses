# HTTP 客户端示例

本示例演示如何使用 HTTP 客户端 API 进行 HTTP 请求。

## 功能特性

- HTTP GET 请求
- HTTP POST JSON 请求
- 自定义 HTTP 请求（带自定义头）
- 支持 HTTPS（使用 mbedTLS）

## 编译

```bash
cd ai_glass_sdk/examples/http_example
make
```

## 运行

```bash
# 运行内置示例
../build/http_example

# 请求指定URL
../build/http_example http://httpbin.org/get
../build/http_example https://www.baidu.com

# 查看帮助
../build/http_example --help
```

## 代码示例

### 简单 GET 请求

```c
#include "http_client.h"

http_response_t resp;
if (http_get("http://api.example.com/data", &resp) == 0) {
    printf("Response: %.*s\n", (int)resp.body_len, resp.body);
    http_response_free(&resp);
}
```

### POST JSON

```c
const char *json = "{\"key\":\"value\"}";
if (http_post_json("http://api.example.com/submit", json, &resp) == 0) {
    printf("Status: %d\n", resp.status_code);
    http_response_free(&resp);
}
```

### 自定义请求

```c
http_request_t req = {
    .url = "http://httpbin.org/headers",
    .method = HTTP_GET,
    .headers = "User-Agent: AI-Core/1.0\r\nX-Custom-Header: test\r\n",
    .timeout_ms = 10000
};

http_response_t resp;
if (http_request(&req, &resp) == 0) {
    printf("Status: %d\n", resp.status_code);
    http_response_free(&resp);
}
```

## 注意事项

1. **超时设置**: HTTP 请求默认 5 秒超时，可通过 `timeout_ms` 参数调整
2. **错误处理**: 务必检查返回值并正确释放资源
3. **内存管理**: HTTP 响应需要调用 `http_response_free()` 释放

## 测试服务器

- HTTP 测试: https://httpbin.org/
