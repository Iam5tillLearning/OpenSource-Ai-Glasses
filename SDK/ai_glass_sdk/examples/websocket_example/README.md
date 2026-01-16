# WebSocket 客户端示例

本示例演示如何使用 WebSocket 客户端 API 进行实时通信。

## 功能特性

- WebSocket 连接/断开
- 发送/接收文本消息
- 发送/接收二进制数据
- 心跳保活机制
- 支持 WSS（使用 mbedTLS）

## 编译

```bash
cd ai_glass_sdk/examples/websocket_example
make
```

## 运行

```bash
# 运行Echo示例（使用默认URL）
../build/ws_example

# 指定URL
../build/ws_example ws://localhost:8080/

# 心跳保活示例
../build/ws_example keepalive

# 二进制数据示例
../build/ws_example binary

# 指定模式和URL
../build/ws_example echo wss://ws.postman-echo.com/raw
../build/ws_example keepalive ws://localhost:8080/
```

## 代码示例

### 基本使用

```c
#include "ws_client.h"
#include <pthread.h>

// 回调函数
void my_callback(ws_event_type_t event, const char *data, size_t len, void *user_data) {
    switch (event) {
        case WS_EVENT_CONNECTED:
            printf("已连接\n");
            break;
        case WS_EVENT_MESSAGE:
            printf("收到: %.*s\n", (int)len, data);
            break;
        case WS_EVENT_DISCONNECTED:
            printf("已断开\n");
            break;
        case WS_EVENT_ERROR:
            printf("错误: %.*s\n", (int)len, data);
            break;
    }
}

// 线程函数
void *ws_thread(void *arg) {
    ws_client_run((ws_client_t *)arg);
    return NULL;
}

// 使用
ws_client_t *client = ws_client_create("ws://example.com/ws", my_callback, NULL);
pthread_t thread;
pthread_create(&thread, NULL, ws_thread, client);

// 发送消息
ws_client_send_text(client, "Hello!");

// 清理
ws_client_stop(client);
pthread_join(thread, NULL);
ws_client_destroy(client);
```

### 发送二进制数据

```c
uint8_t binary_data[] = {0x01, 0x02, 0x03, 0x04};
ws_client_send_binary(client, binary_data, sizeof(binary_data));
```

### 心跳保活

```c
while (running) {
    ws_client_send_text(client, "{\"type\":\"ping\"}");
    sleep(30);  // 每30秒发送一次心跳
}
```

## 注意事项

1. **线程安全**: WebSocket 客户端内部已使用 mutex 保护，可在多线程环境中使用
2. **阻塞调用**: `ws_client_run()` 是阻塞调用，建议在独立线程中运行
3. **优雅退出**: 使用 `ws_client_stop()` 停止事件循环
4. **资源释放**: 停止后需调用 `ws_client_destroy()` 释放资源

## 测试服务器

- WebSocket 测试: wss://ws.postman-echo.com/raw
- WebSocket 测试: wss://echo.websocket.events/
