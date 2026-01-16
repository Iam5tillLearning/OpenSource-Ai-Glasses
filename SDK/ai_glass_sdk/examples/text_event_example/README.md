# 文本事件客户端示例 (Text Event Client Example)

本示例演示了如何使用 AI Glass SDK 订阅并处理来自系统的文本事件流（ASR 语音转文字 和 LLM 大模型回复）。

## 功能说明
程序运行后会连接到 `ai-core` 服务端，并实时打印接收到的文本消息：
- **[ASR]**: 实时显示语音识别结果。
- **[LLM]**: 流式显示大模型的推理回复，并在结束时自动换行。

## 目录结构
```
.
├── Makefile       # 编译脚本
├── main.c         # 示例源码
└── README.md      # 本文档
```

## 快速开始

### 1. 前置条件
确保 `ai-core` 服务端已经启动，并且开启了相关文本服务。

### 2. 编译
在当前目录下执行 `make`：
```bash
make
```
编译成功后会生成可执行文件 `text_event_client_example`。

### 3. 运行
```bash
./text_event_client_example
```

## 代码解析 (main.c)

主要流程如下：

1.  **创建客户端**:
    ```c
    ai_text_event_client_t *client = ai_text_event_client_create();
    ```

2.  **初始化回调**:
    注册 `on_text_event` 函数来处理接收到的数据。
    ```c
    ai_text_event_client_init(client, on_text_event, NULL);
    ```

3.  **启动连接**:
    ```c
    ai_text_event_client_start(client);
    ```

4.  **数据处理 (回调函数)**:
    - 区分 `AI_TEXT_SOURCE_ASR` (语音识别) 和 `AI_TEXT_SOURCE_LLM` (大模型)。
    - 对于 LLM，通过检测 `AI_TEXT_TYPE_FINAL` 标记来判断单次回复是否结束。

5.  **销毁**:
    程序退出前调用 `ai_text_event_client_destroy(client)` 释放资源。
