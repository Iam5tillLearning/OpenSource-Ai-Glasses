# Text Event Client SDK 开发文档

> **说明**: 本文档描述了 `ai_glass_sdk` 中的文本事件客户端接口。该接口用于接收来自系统的 ASR（语音转文字）和 LLM（大模型生成）文本流事件。

## 1. 概述

`ai_text_event` 模块提供了一个轻量级的客户端，允许应用程序通过回调函数实时接收系统产生的文本事件。主要应用场景包括：
- 实时显示语音识别结果
- 显示大模型的流式回复
- 处理系统通知或提示

## 2. 数据结构

### 2.1 文本来源枚举 (`ai_text_source_t`)

```c
typedef enum {
    AI_TEXT_SOURCE_ASR = 0,  // 语音转文字 (ASR)
    AI_TEXT_SOURCE_LLM = 1,  // 大模型 (LLM)
    AI_TEXT_SOURCE_SYS = 2   // 系统通知 (SYSTEM)
} ai_text_source_t;
```

### 2.2 文本事件结构体 (`ai_text_event_t`)

```c
typedef struct {
    ai_text_source_t source; // 文本来源
    const char *text;        // 文本内容 (UTF-8)
    int is_final;           // 是否为最终结果 (1: 最终结果, 0: 中间结果)
} ai_text_event_t;
```

### 2.3 回调函数原型

```c
/**
 * @brief 文本事件回调函数
 * @param event 事件数据指针
 * @param user_data 用户自定义数据
 */
typedef void (*ai_text_event_cb)(const ai_text_event_t *event, void *user_data);
```

## 3. 接口函数

### 3.1 创建客户端

```c
/**
 * @brief 创建文本事件客户端
 * @param cb 事件回调函数
 * @param user_data 用户自定义数据，将在回调中传回
 * @return 成功返回客户端句柄，失败返回 NULL
 */
ai_text_event_client_t* ai_text_event_client_create(ai_text_event_cb cb, void *user_data);
```

### 3.2 启动客户端

```c
/**
 * @brief 启动客户端并连接服务
 * @param client 客户端句柄
 * @return 成功返回 0，失败返回 -1
 */
int ai_text_event_client_start(ai_text_event_client_t *client);
```

### 3.3 销毁客户端

```c
/**
 * @brief 销毁客户端并释放资源
 * @param client 客户端句柄
 */
void ai_text_event_client_destroy(ai_text_event_client_t *client);
```

## 4. 使用示例

以下代码展示了如何创建一个简单的文本事件监听器：

```c
#include <stdio.h>
#include <unistd.h>
#include "ai_text_event.h"

// 事件回调函数
void on_text_event(const ai_text_event_t *event, void *user_data) {
    const char *source_str = "UNKNOWN";
    switch (event->source) {
        case AI_TEXT_SOURCE_ASR: source_str = "ASR"; break;
        case AI_TEXT_SOURCE_LLM: source_str = "LLM"; break;
        case AI_TEXT_SOURCE_SYS: source_str = "SYS"; break;
    }

    printf("[%s] %s (final=%d)\n", source_str, event->text, event->is_final);
}

int main() {
    // 1. 创建客户端
    ai_text_event_client_t *client = ai_text_event_client_create(on_text_event, NULL);
    if (!client) {
        fprintf(stderr, "Failed to create client\n");
        return -1;
    }

    // 2. 启动客户端
    if (ai_text_event_client_start(client) != 0) {
        fprintf(stderr, "Failed to start client\n");
        ai_text_event_client_destroy(client);
        return -1;
    }

    printf("Client started. Waiting for events...\n");

    // 3. 主循环 (实际应用中通常有自己的主循环)
    while (1) {
        sleep(1);
    }

    // 4. 销毁客户端
    ai_text_event_client_destroy(client);
    return 0;
}
```

## 5. 注意事项

1. **回调线程**: 回调函数通常在单独的接收线程中执行，请勿在回调中执行耗时操作，如果需要更新 UI 或进行复杂处理，建议将数据投递到主线程队列。
2. **生命周期**: 确保 `ai_text_event_client_destroy` 在程序退出前被调用以避免资源释放不彻底。
3. **中文支持**: `text` 字段为 UTF-8 编码，确保显示终端或 UI 控件支持 UTF-8。
