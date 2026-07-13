# AI Display 客户端 API

## 概述

AI Display 客户端 API 提供与 `ai_display_service` 显示服务进行通信的接口。通过共享内存和 Unix Socket 实现高性能的帧缓冲提交和显示控制。

## 核心特性

- **双缓冲槽位设计**：支持主应用和 overlay 叠加层
- **零拷贝传输**：基于共享内存的帧缓冲传输
- **可靠通信**：Unix Socket 命令通道
- **多客户端支持**：焦点请求实现显示优先级管理

## 架构图

```
┌─────────────────┐     ┌─────────────────────┐
│  Client App     │     │  ai_display_service │
│                 │     │                     │
│  ┌───────────┐  │     │  ┌───────────┐      │
│  │framebuffer│◄─┼─SHM─┼─►│framebuffer│      │
│  └───────────┘  │     │  └───────────┘      │
│       │         │     │        ▲            │
│       ▼         │     │        │            │
│  ┌───────────┐  │     │  ┌─────┴─────┐      │
│  │  commit   │──┼─UDS─┼─►│  刷新屏幕 │      │
│  └───────────┘  │     │  └───────────┘      │
└─────────────────┘     └─────────────────────┘
```

## API 参考

### 核心函数

| 函数 | 描述 |
|------|------|
| `ai_display_init()` | 初始化客户端，返回客户端句柄 |
| `ai_display_connect()` | 连接到显示服务 |
| `ai_display_disconnect()` | 断开连接（可重连） |
| `ai_display_get_framebuffer()` | 获取主帧缓冲指针 (Slot 0) |
| `ai_display_get_framebuffer_slot()` | 获取指定槽位的帧缓冲指针 |
| `ai_display_commit_frame()` | 提交帧更新到屏幕 |
| `ai_display_request_focus()` | 请求显示焦点 |
| `ai_display_set_power_save_timeout()` | 设置息屏超时时间（0=禁用息屏） |
| `ai_display_cleanup()` | 清理资源 |

### 辅助函数

| 函数 | 描述 |
|------|------|
| `ai_display_is_connected()` | 检查连接状态 |
| `ai_display_get_error_string()` | 获取错误码的描述字符串 |

### 错误码

| 错误码 | 值 | 描述 |
|--------|-----|------|
| `AI_DISPLAY_SUCCESS` | 0 | 成功 |
| `AI_DISPLAY_ERROR_INIT` | -1 | 初始化失败 |
| `AI_DISPLAY_ERROR_CONNECT` | -2 | 连接失败 |
| `AI_DISPLAY_ERROR_SHM` | -3 | 共享内存操作失败 |
| `AI_DISPLAY_ERROR_SOCKET` | -4 | Socket 操作失败 |
| `AI_DISPLAY_ERROR_SEND` | -5 | 发送失败 |
| `AI_DISPLAY_ERROR_PARAM` | -6 | 参数错误 |
| `AI_DISPLAY_ERROR_NOT_READY` | -7 | 客户端未就绪 |

## 使用示例

### 基本用法

```c
#include "ai_display.h"
#include <string.h>

int main() {
    // 1. 初始化客户端
    ai_display_client_t *client = ai_display_init();
    if (!client) {
        printf("初始化失败\n");
        return -1;
    }

    // 2. 连接到服务
    int ret = ai_display_connect(client);
    if (ret != AI_DISPLAY_SUCCESS) {
        printf("连接失败: %s\n", ai_display_get_error_string(ret));
        ai_display_cleanup(client);
        return -1;
    }

    // 3. 获取帧缓冲
    uint8_t *framebuffer = ai_display_get_framebuffer(client);
    if (!framebuffer) {
        printf("获取帧缓冲失败\n");
        ai_display_cleanup(client);
        return -1;
    }

    // 4. 写入帧数据（示例：填充为黑色）
    memset(framebuffer, 0, AI_DISPLAY_FRAME_SIZE);

    // 5. 提交帧
    ret = ai_display_commit_frame(client, 0, 0, 0, AI_DISPLAY_WIDTH, AI_DISPLAY_HEIGHT);
    if (ret != AI_DISPLAY_SUCCESS) {
        printf("提交帧失败: %s\n", ai_display_get_error_string(ret));
    }

    // 6. 清理
    ai_display_cleanup(client);
    return 0;
}
```

### 使用 Overlay 槽位

```c
// 获取 overlay 帧缓冲 (Slot 1)
uint8_t *overlay = ai_display_get_framebuffer_slot(client, 1);
if (overlay) {
    // 绘制 overlay 内容...

    // 提交 overlay 帧
    ai_display_commit_frame(client, 1, 0, 0, AI_DISPLAY_WIDTH, AI_DISPLAY_HEIGHT);
}
```

### 请求显示焦点

```c
// 在多客户端场景下请求前台显示权
int ret = ai_display_request_focus(client);
if (ret == AI_DISPLAY_SUCCESS) {
    printf("已获得显示焦点\n");
}
```

### 息屏控制

```c
// 设置 10 秒
ai_display_set_power_save_timeout(client, 10);

// 禁用自动息屏功能
ai_display_set_power_save_timeout(client, 0);
```

## 编译链接

```bash
# 编译
gcc -I/path/to/ai_glass_sdk/include -c your_app.c -o your_app.o

# 链接（静态库）
gcc your_app.o -L/path/to/ai_glass_sdk/lib -lai_glass_sdk -lrt -o your_app

# 链接（动态库）
gcc your_app.o -L/path/to/ai_glass_sdk/lib -lai_glass_sdk -lrt -o your_app
```

## 注意事项

1. **服务端先启动**：调用 `ai_display_connect()` 前，`ai_display_service` 必须已启动
2. **线程安全**：单个客户端句柄不应在多线程中并发使用
3. **帧缓冲格式**：帧缓冲使用 4BPP 格式，大小为 `AI_DISPLAY_FRAME_SIZE` 字节
4. **资源清理**：使用完毕后务必调用 `ai_display_cleanup()` 释放资源
