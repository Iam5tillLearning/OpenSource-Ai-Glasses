# Audio Client API 开发指南

中文版本 | [English Version](Audio_Client_API.en.md)

> **版本**: v1.0 | **日期**: 2025-10-10 | **状态**: ✅ 生产就绪

---

## 📖 目录

- [快速开始](#快速开始)
- [API参考](#api参考)
- [完整示例](#完整示例)
- [故障排查](#故障排查)
- [命令行工具](#命令行工具)

---

## 🚀 快速开始

### 功能简介

控制音频播放，支持PCM文件播放和TTS文本转语音。

### 编译客户端程序

#### 链接SDK库

```bash
# 先编译SDK库
cd ai_glass_sdk
make

# 编译自己的程序
arm-rockchip831-linux-uclibcgnueabihf-gcc \
    -o my_audio_app \
    my_audio_app.c \
    -I/path/to/ai_glass_sdk/include \
    -L/path/to/ai_glass_sdk/lib \
    -lai_glass_sdk
```

### 运行示例程序

```bash
# 编译SDK示例程序
cd ai_glass_sdk/examples/audio_play_example
make

# 推送并运行SDK示例程序
adb push ../build/audio_play_example /path/you/like
./audio_play_example -f /path/to/audio.pcm -v 80
```

### 最小客户端代码

```c
#include "ai_audio.h"
#include <stdio.h>

int main() {
    // 1. 初始化客户端
    ai_audio_t *client = ai_audio_init(NULL);  // 使用默认路径
    if (!client) {
        printf("初始化失败\n");
        return -1;
    }

    // 2. 播放音频（使用默认参数）
    int result = ai_audio_play_simple(client, "/oem/usr/bin/resources/beep.pcm");
    if (result != AI_AUDIO_SUCCESS) {
        printf("播放失败: %s\n", ai_audio_get_error_string(result));
    }

    // 3. 清理资源
    ai_audio_cleanup(client);
    return 0;
}
```

### 高级播放示例

```c
#include "ai_audio.h"

int main() {
    ai_audio_t *client = ai_audio_init(NULL);

    // 创建播放参数
    ai_audio_params_t params = {
        .file_path = "/path/to/audio.pcm",
        .volume = 80,              // 音量80%
        .force = 1,                // 强制播放（打断当前）
        .sample_rate = 48000,      // 48kHz采样率
        .channels = 2,             // 双声道
        .bit_width = 16            // 16位
    };

    // 播放
    ai_audio_play(client, &params);

    // 清理
    ai_audio_cleanup(client);
    return 0;
}
```

### 主要特性

- ✅ 简单易用的C语言API
- ✅ 支持PCM文件播放和TTS文本转语音
- ✅ 多种音频参数配置（采样率、声道、位宽）
- ✅ 强制播放模式（打断当前播放）
- ✅ 排队播放模式
- ✅ 音量控制 (0-100)
- ✅ 停止当前播放
- ✅ MD5智能缓存（TTS功能）
- ✅ SDK控制录音（开始/停止/状态）
- ✅ SDK控制 ai-core 是否响应物理按键业务动作
- ✅ SDK统一资源仲裁（相机/音频释放与回收）

---

## 📋 API参考

### 数据结构

#### ai_audio_t

音频客户端句柄（不透明类型），通过 `ai_audio_init()` 创建。

#### ai_audio_params_t

音频播放参数结构：

```c
typedef struct {
    const char *file_path;    // PCM文件路径（必填，播放PCM时使用）
    int volume;               // 音量 (0-100)，-1表示使用默认值
    int force;                // 强制播放标志 (0=排队, 1=打断当前播放)
    int sample_rate;          // 采样率 (8000-96000)，-1表示使用默认值
    int channels;             // 声道数 (1-8)，-1表示使用默认值
    int bit_width;            // 位宽 (8/16/24/32)，-1表示使用默认值
} ai_audio_params_t;
```

#### ai_audio_tts_params_t

TTS文本转语音参数结构：

```c
typedef struct {
    const char *text;         // 要转换的文本（必填）
    int volume;               // 音量 (0-100)，-1表示使用默认值
    int force;                // 强制播放标志 (0=排队, 1=打断当前播放)
    int use_cache;            // 是否使用缓存 (0=不使用, 1=使用，推荐)
} ai_audio_tts_params_t;
```

**字段说明**：
- **file_path**: PCM文件的完整路径（仅PCM播放时使用）
- **text**: 要转换的文本内容（仅TTS播放时使用）
- **volume**: 音量百分比，0=静音，100=最大
- **force**: 0=排队播放，1=立即打断当前播放
- **use_cache**: TTS缓存开关，推荐启用以提高响应速度
- **sample_rate**: 音频采样率（Hz）
- **channels**: 声道数量
- **bit_width**: 每个采样点的位数

#### 错误码

```c
#define AI_AUDIO_SUCCESS           0    // 成功
#define AI_AUDIO_ERROR_INIT       -1    // 初始化失败
#define AI_AUDIO_ERROR_CONNECT    -2    // 连接失败
#define AI_AUDIO_ERROR_SEND       -3    // 发送失败
#define AI_AUDIO_ERROR_PARAM      -4    // 参数错误
#define AI_AUDIO_ERROR_RESPONSE   -5    // 服务端响应错误
#define AI_AUDIO_ERROR_STATE      -6    // 状态错误（例如重复开始录音）
#define AI_AUDIO_ERROR_TIMEOUT    -7    // 流订阅读取超时，可重试
```

### 核心API

#### ai_audio_init()

初始化音频客户端。

```c
ai_audio_t* ai_audio_init(const char *socket_path);
```

**参数**：
- `socket_path` - 服务端连接路径，传NULL使用默认路径

**返回值**：
- 成功：客户端句柄指针
- 失败：NULL

**说明**：
- 创建客户端实例，不建立实际连接
- 连接在每次发送命令时动态建立

---

#### ai_audio_play()

播放音频文件。

```c
int ai_audio_play(ai_audio_t *client, const ai_audio_params_t *params);
```

**参数**：
- `client` - 客户端句柄
- `params` - 播放参数

**返回值**：
- `AI_AUDIO_SUCCESS` (0) - 成功
- `AI_AUDIO_ERROR_CONNECT` (-2) - 连接服务端失败
- `AI_AUDIO_ERROR_SEND` (-3) - 发送命令失败
- `AI_AUDIO_ERROR_PARAM` (-4) - 参数错误
- `AI_AUDIO_ERROR_RESPONSE` (-5) - 服务端返回错误

**说明**：
- `force=0`: 将音频添加到播放队列末尾
- `force=1`: 立即停止当前播放，播放新音频
- 未指定的参数（值为-1）将使用服务端默认值

---

#### ai_audio_stop()

停止当前播放。

```c
int ai_audio_stop(ai_audio_t *client);
```

**参数**：
- `client` - 客户端句柄

**返回值**：
- `AI_AUDIO_SUCCESS` (0) - 成功
- 负数 - 错误码

**说明**：
- 立即停止当前正在播放的音频
- 清空播放队列

---

#### ai_audio_set_disable_aicore_physical_actions()

设置是否禁用 ai-core 物理动作（录音/拍照/抢话）。

```c
int ai_audio_set_disable_aicore_physical_actions(ai_audio_t *client, int disabled);
```

**参数**：
- `client` - 客户端句柄
- `disabled` - 1=禁用 ai-core 物理动作并保留 GPIO 事件，0=恢复默认动作

**返回值**：
- `AI_AUDIO_SUCCESS` (0) - 成功
- 负数 - 错误码

**说明**：
- 启用后，ai-core 不再自动执行物理按键触发的录音/拍照/抢话动作
- GPIO 事件广播仍保留，可由 SDK 或其他模块继续消费（可并存）

---

#### ai_audio_get_disable_aicore_physical_actions()

查询是否禁用 ai-core 物理动作。

```c
int ai_audio_get_disable_aicore_physical_actions(ai_audio_t *client, int *disabled);
```

**参数**：
- `client` - 客户端句柄
- `disabled` - 输出参数，1=已禁用，0=未禁用

**返回值**：
- `AI_AUDIO_SUCCESS` (0) - 成功
- 负数 - 错误码

---

#### ai_audio_record_start()

通过 SDK 命令启动录音（不依赖物理按键）。

```c
int ai_audio_record_start(ai_audio_t *client);
```

**参数**：
- `client` - 客户端句柄

**返回值**：
- `AI_AUDIO_SUCCESS` (0) - 成功
- `AI_AUDIO_ERROR_STATE` (-6) - 已在录音中
- 其他负数 - 错误码

**前提**：
- ai-core 需以 `--enable-gpio` 启动（录音控制线程使用 GPIO 触发模式）
- 建议同时启用 `--disable-aicore-physical-actions`，禁用 ai-core 默认物理交互动作

---

#### ai_audio_record_stop()

通过 SDK 命令停止录音，并返回录音文件路径。

```c
int ai_audio_record_stop(ai_audio_t *client, char *output_path, int output_path_size);
```

**参数**：
- `client` - 客户端句柄
- `output_path` - 输出路径缓冲区（可为NULL）
- `output_path_size` - 缓冲区大小

**返回值**：
- `AI_AUDIO_SUCCESS` (0) - 成功
- 负数 - 错误码

**说明**：
- 默认返回路径通常为 `/tmp/my_recording.pcm`

---

#### ai_audio_record_get_status()

查询当前录音状态。

```c
int ai_audio_record_get_status(ai_audio_t *client, int *recording);
```

**参数**：
- `client` - 客户端句柄
- `recording` - 输出参数，1=录音中，0=未录音

**返回值**：
- `AI_AUDIO_SUCCESS` (0) - 成功
- 负数 - 错误码

---

#### 资源仲裁 API

用于应用切换场景（例如 launcher 进入/退出 IPC），统一控制 ai-core 对相机/音频资源的持有状态。

```c
#define AI_AUDIO_RESOURCE_CAMERA  0x01
#define AI_AUDIO_RESOURCE_AUDIO   0x02
#define AI_AUDIO_RESOURCE_ALL     (AI_AUDIO_RESOURCE_CAMERA | AI_AUDIO_RESOURCE_AUDIO)

typedef struct {
    int camera_suspended;  // 1=已释放给外部应用，0=ai-core持有
    int audio_suspended;   // 1=已释放给外部应用，0=ai-core持有
} ai_audio_resource_status_t;

int ai_audio_suspend_resources(ai_audio_t *client, int resource_mask);
int ai_audio_resume_resources(ai_audio_t *client, int resource_mask);
int ai_audio_get_resource_status(ai_audio_t *client, ai_audio_resource_status_t *status);
```

**典型时序**：
- 进入 IPC 前：`ai_audio_suspend_resources(client, AI_AUDIO_RESOURCE_ALL)`
- IPC 退出后：`ai_audio_resume_resources(client, AI_AUDIO_RESOURCE_ALL)`
- 过程中可轮询：`ai_audio_get_resource_status(...)`

**示例程序**: `ai_glass_sdk/examples/media_resource_control/`

> 说明：该资源仲裁 API 只用于外部应用确实需要直接持有 camera/audio 的场景。当前 RTSP 实时视频流由 `ai-core` 持有 camera/audio，并通过 H.265 共享主码流供 `rkipc` 订阅，不再用本 API 作为 RTSP 进入/退出路径。

**使用场景**：
- 进入外部 IPC 应用前，释放相机/音频给外部应用
- 外部 IPC 应用退出后，把资源交还给 `ai-core`

---

#### Holder 仲裁 API（第二阶段）

用于当前持有外部媒体资源的应用向 `ai-core` 注册自己，并暴露统一 reclaim 能力。

```c
#define AI_MEDIA_HOLDER_ID_MAX 64
#define AI_MEDIA_ENDPOINT_PATH_MAX 108

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
    char holder_id[AI_MEDIA_HOLDER_ID_MAX];
    char endpoint_path[AI_MEDIA_ENDPOINT_PATH_MAX];
    unsigned int holder_generation;
    int loan_active;
    int loan_auto_return;
    int return_pending;
} ai_media_arbitration_status_t;

int ai_media_register_holder(ai_audio_t *client,
                             const ai_media_holder_registration_t *registration);
int ai_media_unregister_holder(ai_audio_t *client, const char *holder_id);
int ai_media_get_arbitration_status(ai_audio_t *client,
                                    ai_media_arbitration_status_t *status);
```

**说明**：
- 当前已实现单 holder、`audio` 优先的统一 reclaim / auto-return
- holder app 只需要注册本地 `ops`；reclaim Unix socket endpoint 由 SDK 在进程内自动创建并注册给 `ai-core`
- 当前 `endpoint_path` 只用于状态查询和调试展示，不需要 app 显式配置
- `holder_generation` 用于标识 holder 实例代际，避免把资源还给旧实例
- `loan_active=1` 表示当前 `audio` 是 `ai-core` 主动借回但尚未归还
- `loan_auto_return=1` 表示本次借回在本地使用结束后需要自动返还
- `return_pending=1` 表示当前已进入自动返还流程

**典型时序**：
- 外部应用先用 `ai_audio_suspend_resources()` 拿资源
- 拿到资源后调用 `ai_media_register_holder()`
- 需要回收时，`ai-core` 通过统一 reclaim endpoint 反向请求 holder 释放资源
- 如果这次回收属于 `ai-core` 主动借回，且本地使用结束，`ai-core` 会再通过统一 `ACQUIRE` 自动把资源还回原 holder
- holder 退出前调用 `ai_media_unregister_holder()`

**语义边界**：
- 如果资源是外部应用主动归还给 `ai-core`，则视为所有权转移，不自动返还
- 只有 `ai-core` 主动借回建立了 loan 上下文，才会触发自动返还

---

#### 音频镜像流订阅 API（第一阶段）

用于外部应用从 `ai-core` 订阅常驻 `mic` 的旁路音频镜像流。

```c
#define AI_AUDIO_STREAM_DEFAULT_SOCKET_PATH "/tmp/ai-core_audio_stream"
#define AI_AUDIO_STREAM_MAGIC 0x41415346u
#define AI_AUDIO_STREAM_VERSION 1
#define AI_AUDIO_STREAM_CODEC_G711A 1
#define AI_AUDIO_STREAM_MAX_PAYLOAD 4096

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

int ai_audio_stream_subscribe(const char *socket_path,
                              ai_audio_stream_handle_t *handle);
int ai_audio_stream_read_packet(ai_audio_stream_handle_t *handle,
                                ai_audio_stream_packet_t *packet);
void ai_audio_stream_unsubscribe(ai_audio_stream_handle_t *handle);
```

**说明**：
- 第一阶段使用本地 `AF_UNIX + SOCK_SEQPACKET`
- 第一阶段默认输出 `G711A`
- `capture_ts_us` 取自 `ai-core` 采集/编码链时间戳，不是 socket 接收时刻
- 该 API 只提供旁路镜像流，不改变 `ai-core` 当前物理按键、录音、云端对话主链语义

---

#### ai_audio_cleanup()

清理客户端资源。

```c
void ai_audio_cleanup(ai_audio_t *client);
```

**参数**：
- `client` - 客户端句柄

**说明**：
- 释放客户端内存
- 不影响服务端的播放状态

---

#### ai_audio_get_error_string()

获取错误码对应的错误信息。

```c
const char* ai_audio_get_error_string(int error_code);
```

**参数**：
- `error_code` - 错误码

**返回值**：
- 错误信息字符串

---

### 辅助API

#### ai_audio_create_default_params()

创建默认播放参数。

```c
ai_audio_params_t ai_audio_create_default_params(const char *file_path);
```

**参数**：
- `file_path` - PCM文件路径

**返回值**：
- 参数结构体，所有可选参数设置为-1

**示例**：
```c
ai_audio_params_t params = ai_audio_create_default_params("/path/to/audio.pcm");
params.volume = 80;  // 只设置音量
params.force = 1;    // 设置强制播放
ai_audio_play(client, &params);
```

---

#### ai_audio_play_tts()

播放TTS文本转语音。

```c
int ai_audio_play_tts(ai_audio_t *client, const ai_audio_tts_params_t *params);
```

**参数**：
- `client` - 客户端句柄
- `params` - TTS播放参数

**返回值**：
- `AI_AUDIO_SUCCESS` (0) - 成功
- `AI_AUDIO_ERROR_CONNECT` (-2) - 连接服务端失败
- `AI_AUDIO_ERROR_SEND` (-3) - 发送命令失败
- `AI_AUDIO_ERROR_PARAM` (-4) - 参数错误
- `AI_AUDIO_ERROR_RESPONSE` (-5) - 服务端返回错误

**说明**：
- 将文本转换为语音并播放
- 支持智能缓存，相同文本会复用缓存
- `use_cache=0`: 强制重新生成，适用于动态内容

---

#### ai_audio_play_tts_simple()

简化的TTS播放函数。

```c
int ai_audio_play_tts_simple(ai_audio_t *client, const char *text);
```

**参数**：
- `client` - 客户端句柄
- `text` - 要转换的文本

**返回值**：
- 错误码

**说明**：
- 使用默认参数播放TTS
- 等价于使用默认TTS参数调用 `ai_audio_play_tts()`

---

#### ai_audio_play_toast()

播放适合短提示的TTS文本。

```c
int ai_audio_play_toast(ai_audio_t *client, const char *text);
```

**参数**：
- `client` - 客户端句柄
- `text` - 要播报的短提示文本

**返回值**：
- 错误码

**说明**：
- SDK 会先过滤不适合朗读或会破坏TTS命令协议的字符
- 固定使用音量 `80`、排队播放、启用缓存
- 适合操作反馈、状态提示等可重复短文案

---

#### ai_audio_play_toast_text()

使用默认音频Socket播放适合短提示的TTS文本。

```c
int ai_audio_play_toast_text(const char *text);
```

**参数**：
- `text` - 要播报的短提示文本

**返回值**：
- 错误码

**说明**：
- 等价于创建默认音频客户端后调用 `ai_audio_play_toast()`
- 调用方不需要手动管理 `ai_audio_t` 生命周期

---

#### ai_audio_play_simple()

简化的PCM播放函数。

```c
int ai_audio_play_simple(ai_audio_t *client, const char *file_path);
```

**参数**：
- `client` - 客户端句柄
- `file_path` - PCM文件路径

**返回值**：
- 错误码

**说明**：
- 使用默认参数播放PCM文件
- 等价于 `ai_audio_play(client, &ai_audio_create_default_params(file_path))`

---

## 💡 完整示例

### 示例1：基本播放

```c
#include "ai_audio.h"
#include <stdio.h>

int main() {
    ai_audio_t *client = ai_audio_init(NULL);
    if (!client) {
        printf("Failed to initialize client\n");
        return -1;
    }

    int result = ai_audio_play_simple(client, "/oem/usr/bin/resources/beep.pcm");
    if (result != AI_AUDIO_SUCCESS) {
        printf("Play failed: %s\n", ai_audio_get_error_string(result));
    } else {
        printf("Playing audio...\n");
    }

    ai_audio_cleanup(client);
    return 0;
}
```

### 示例2：自定义参数播放

```c
#include "ai_audio.h"
#include <stdio.h>

int main() {
    ai_audio_t *client = ai_audio_init(NULL);

    // 配置播放参数
    ai_audio_params_t params = {
        .file_path = "/data/alert.pcm",
        .volume = 90,
        .force = 1,           // 强制播放
        .sample_rate = 48000,
        .channels = 2,
        .bit_width = 16
    };

    printf("Playing audio with custom parameters...\n");
    int result = ai_audio_play(client, &params);
    if (result != AI_AUDIO_SUCCESS) {
        printf("Error: %s\n", ai_audio_get_error_string(result));
    }

    ai_audio_cleanup(client);
    return 0;
}
```

### 示例3：播放多个音频文件

```c
#include "ai_audio.h"
#include <stdio.h>
#include <unistd.h>

int main() {
    ai_audio_t *client = ai_audio_init(NULL);

    const char *files[] = {
        "/data/sound1.pcm",
        "/data/sound2.pcm",
        "/data/sound3.pcm"
    };

    for (int i = 0; i < 3; i++) {
        printf("Playing %s...\n", files[i]);

        ai_audio_params_t params = ai_audio_create_default_params(files[i]);
        params.force = 0;  // 排队播放，不打断

        int result = ai_audio_play(client, &params);
        if (result != AI_AUDIO_SUCCESS) {
            printf("Failed to play %s: %s\n", files[i],
                   ai_audio_get_error_string(result));
        }

        sleep(1);  // 等待1秒再添加下一个
    }

    ai_audio_cleanup(client);
    return 0;
}
```

### 示例4：TTS文本转语音

```c
#include "ai_audio.h"
#include <stdio.h>

int main() {
    ai_audio_t *client = ai_audio_init(NULL);

    // 简单TTS播放
    printf("Playing TTS...\n");
    int result = ai_audio_play_tts_simple(client, "你好，欢迎使用AI语音助手");
    if (result != AI_AUDIO_SUCCESS) {
        printf("TTS failed: %s\n", ai_audio_get_error_string(result));
    }

    // 带参数的TTS播放
    ai_audio_tts_params_t tts_params = {
        .text = "这是一条重要通知",
        .volume = 90,
        .force = 0,      // 排队播放
        .use_cache = 1   // 使用缓存
    };

    ai_audio_play_tts(client, &tts_params);

    // 动态内容（禁用缓存）
    ai_audio_tts_params_t dynamic_params = {
        .text = "当前时间：2025年10月16日",
        .volume = 80,
        .force = 0,
        .use_cache = 0   // 禁用缓存，因为内容是动态的
    };

    ai_audio_play_tts(client, &dynamic_params);

    ai_audio_cleanup(client);
    return 0;
}
```

### 示例5：紧急播放（打断当前）

```c
#include "ai_audio.h"
#include <stdio.h>

void play_alert(ai_audio_t *client) {
    // 使用PCM文件播放警报音
    ai_audio_params_t params = {
        .file_path = "/oem/usr/bin/resources/alert.pcm",
        .volume = 100,        // 最大音量
        .force = 1,           // 立即打断当前播放
        .sample_rate = 16000,
        .channels = 1,
        .bit_width = 16
    };

    ai_audio_play(client, &params);
}

void play_alert_tts(ai_audio_t *client) {
    // 使用TTS播放紧急通知
    ai_audio_tts_params_t tts_params = {
        .text = "警告！检测到异常情况",
        .volume = 100,
        .force = 1,           // 立即打断当前播放
        .use_cache = 1
    };

    ai_audio_play_tts(client, &tts_params);
}

int main() {
    ai_audio_t *client = ai_audio_init(NULL);

    // 正常播放背景音乐
    ai_audio_play_simple(client, "/data/background.pcm");

    // 模拟紧急情况
    sleep(2);
    printf("Alert! Playing emergency sound...\n");
    play_alert(client);  // 会打断背景音乐

    sleep(3);
    printf("Alert TTS! Playing emergency message...\n");
    play_alert_tts(client);  // 使用TTS播放紧急消息

    ai_audio_cleanup(client);
    return 0;
}
```

---

## 🔧 故障排查

### 1. 客户端初始化失败

**错误**：`ai_audio_init()` 返回 NULL

**原因**：
- 内存分配失败

**解决**：
- 检查系统可用内存
- 确认没有内存泄漏

---

### 2. 连接服务端失败

**错误**：API函数返回 `AI_AUDIO_ERROR_CONNECT`

**原因**：
- 服务端未启动
- 连接路径错误

**解决**：
```bash
# 检查服务端是否运行
ps aux | grep ai-core
```

---

### 3. TTS播放失败

**错误**：TTS函数返回非零值

**原因**：
- TTS服务器不可达
- 网络连接问题
- 文本编码问题

**解决**：
- 检查网络连接和TTS服务器配置
- 确保文本使用UTF-8编码

---

### 4. 参数错误

**错误**：函数返回 `AI_AUDIO_ERROR_PARAM`

**原因**：
- 参数值超出范围
- 必填参数为空

**解决**：
- 确认参数在有效范围内：
  - volume: 0-100
  - sample_rate: 8000-96000
  - channels: 1-8
  - bit_width: 8/16/24/32
- 确保必填参数（file_path或text）不为空

---

## ⚙️ 系统要求

### 前提条件

服务端（ai-core）必须已启动。

### 音频文件要求

- **格式**: PCM（Raw audio）
- **编码**: 无压缩
- **采样率**: 8000-96000 Hz
- **声道数**: 1-8
- **位宽**: 8/16/24/32 bit

### 编译依赖

- **交叉编译工具链**: arm-rockchip831-linux-uclibcgnueabihf-gcc
- **系统库**: 标准C库

---

## 📊 性能特性

| 特性 | 说明 |
|------|------|
| **连接模式** | 短连接（每次命令建立新连接） |
| **响应延迟** | < 50ms |
| **并发支持** | 支持多个客户端 |
| **播放模式** | 排队播放 / 强制播放 |
| **音频格式** | PCM（未压缩） |
| **采样率范围** | 8kHz - 96kHz |

---

---

## 🛠️ 命令行工具

除了编程接口，SDK还提供了方便的命令行工具：

### 快速使用

```bash
# PCM文件播放
./audio_play_example -f /path/to/audio.pcm -v 80

# TTS文本播放
./audio_play_example -t "你好世界" -v 90

# 强制播放（打断当前）
./audio_play_example -f /tmp/urgent.pcm -F

# 停止播放
./audio_play_example -S
```

### 详细说明

完整的命令行工具使用说明，请参见：
**📖 [Audio Play Example 使用指南](../examples/audio_play_example/README.md)**

---

## 🔗 相关文档

- **示例程序**: `ai_glass_sdk/examples/audio_play_example/`
- **头文件**: `ai_glass_sdk/include/ai_audio.h`
- **SDK README**: `ai_glass_sdk/README.md`

---

**版本**: v1.0
**日期**: 2025-10-10
**状态**: ✅ 生产就绪
