# AI Glass SDK - 客户端开发套件

中文版本 | [English Version](README.en.md)

## 简介

本 SDK 为 AI Core Service 提供完整的客户端开发套件，支持 GPIO 事件订阅、摄像头抓拍、音频播放、录音控制、物理交互控制、媒体资源仲裁、显示提交、BLE 文本收发和文本事件监听。

说明：示例程序统一通过 `lib/libai_glass_sdk.a` 或 `lib/libai_glass_sdk.so` 链接 SDK。

说明：当前发布包为“头文件 + 预编译库 + 示例程序”形态，不包含 `src/` 源码目录；示例程序应直接链接 `lib/libai_glass_sdk.a` 或 `lib/libai_glass_sdk.so`。

## v0.7.0 SDK 更新

- 新增 `ai_ble.h` 与 `BLE_Client_API.md`，支持 BLE 文本通道接入。
- 新增录音控制 API：`ai_audio_record_start()`、`ai_audio_record_stop()`、`ai_audio_record_get_status()`。
- 新增物理动作控制/查询 API：`ai_audio_set_disable_aicore_physical_actions()`、`ai_audio_get_disable_aicore_physical_actions()`。
- 新增相机/音频资源释放、回收和状态查询 API。
- 补齐显示提交与文本事件监听文档入口。
- 新增只读查询物理动作状态示例，便于现场排查时不改变运行态。
- 示例程序统一链接预编译 SDK 库，并使用 `examples/build/` 统一输出目录。

## 📦 SDK内容

```text
ai_glass_sdk/
├── include/              # 头文件
│   ├── ai_gpio.h                  # GPIO 事件客户端 API
│   ├── ai_ipc.h                   # IPC 通信 API
│   ├── ai_camera.h                # 摄像头客户端 API
│   ├── ai_audio.h                 # 音频客户端 API
│   ├── ai_display.h               # 显示客户端 API
│   ├── ai_ble.h                   # BLE 文本客户端 API
│   ├── ai_text_event.h            # 文本事件客户端 API
│   └── ai_log.h                   # 日志系统 API
├── lib/                  # 预编译库文件
│   ├── libai_glass_sdk.a          # 静态库
│   └── libai_glass_sdk.so         # 动态库
├── examples/             # 示例程序
│   ├── gpio_example/                         # GPIO 事件订阅示例
│   ├── audio_play_example/                   # 音频播放示例
│   ├── camera_capture_example/               # 摄像头抓拍示例
│   ├── disable_aicore_physical_actions_example/ # 禁用 AI-Core 物理动作示例
│   ├── query_aicore_physical_actions_example/   # 只读查询 AI-Core 物理动作状态示例
│   ├── record_audio_example/                 # SDK 控制录音示例
│   ├── media_resource_control/               # 相机/音频资源切换控制台示例
│   ├── text_event_example/                   # ASR/LLM/System 文本流监听示例
│   ├── http_example/                         # HTTP 客户端示例
│   └── websocket_example/                    # WebSocket 客户端示例
├── docs/                 # 客户端接入文档
│   ├── GPIO_Client_API.md           # GPIO 客户端 API 文档
│   ├── Camera_Client_API.md         # 摄像头客户端 API 文档
│   ├── Audio_Client_API.md          # 音频客户端 API 文档
│   ├── Display_Client_API.md        # 显示客户端 API 文档
│   ├── BLE_Client_API.md            # BLE 文本客户端 API 文档
│   ├── Text_Event_Client_API.md     # 文本事件客户端 API 文档
│   └── Log_API.md                   # 日志系统 API 文档
├── README.md            # 本文件
├── README.en.md         # 英文说明
├── Makefile             # SDK 编译脚本
└── VERSION              # 版本信息
```

## 🎯 主要功能

### 1. GPIO 事件订阅
- 支持多进程同时监听 GPIO 按键事件
- 异步事件回调机制
- 低延迟（< 13ms）

### 2. 摄像头调用
- 通过共享内存零拷贝图像传输
- 支持 JPEG 和 NV12 格式
- 多客户端并发支持

### 3. 音频控制
- PCM 播放与音频控制
- 录音开始/停止/状态查询
- SDK 控制 AI-Core 是否响应物理按键业务动作
- 统一资源仲裁（相机/音频释放与回收）

### 4. 显示服务
- 基于共享内存的帧缓冲传输
- 双缓冲槽位（主显示 + Overlay）
- 多客户端焦点管理

### 5. 文本事件订阅
- 监听 ASR / LLM / System 文本流
- 支持流式输出和最终结果标记
- 独立接收线程，不阻塞主循环

### 6. BLE 文本通道
- 通过 `bt_service` 暴露的本地 Unix Socket 统一接入 BLE 文本消息
- 支持按 `datatype` 订阅，例如 `display.text`
- 支持从本地应用发送 UTF-8 JSON 文本到手机侧 notify

## 🚀 快速开始

### 1. 编译 SDK 或示例程序

```bash
# SDK 发布包已包含预编译库；如需一次性构建默认示例集合，可直接执行
cd ai_glass_sdk
make
```

如果只想单独编译某个示例，也可以直接进入对应目录：

```bash
cd examples/gpio_example && make
cd ../audio_play_example && make
cd ../camera_capture_example && make
cd ../disable_aicore_physical_actions_example && make
cd ../query_aicore_physical_actions_example && make
cd ../record_audio_example && make
cd ../media_resource_control && make
cd ../text_event_example && make
cd ../http_example && make
cd ../websocket_example && make
```

### 2. 运行示例程序

#### GPIO 事件客户端
```bash
# 确保服务端已启动（示例中监听 GPIO 75）
./ai-core --enable-gpio --gpio-number 1 --gpio-numbers 0,1,75

cd examples/gpio_example
./../build/gpio_example -g 75
```

#### 摄像头客户端
```bash
# 确保服务端启用摄像头
./ai-core --enable-camera --enable-jpeg

cd examples/camera_capture_example
./../build/camera_capture_example /tmp
```

#### 音频播放客户端
```bash
# 确保服务端已启动
./ai-core

cd examples/audio_play_example
./../build/audio_play_example -f /path/to/audio.pcm -v 80 -r 48000
```

#### 禁用 AI-Core 物理动作
```bash
cd examples/disable_aicore_physical_actions_example
./../build/disable_aicore_physical_actions_example
```

#### 只读查询 AI-Core 物理动作状态
```bash
cd examples/query_aicore_physical_actions_example
./../build/query_aicore_physical_actions_example
```

说明：该示例只查询 `disable_aicore_physical_actions` 当前状态，不会像 `disable_aicore_physical_actions_example` 一样修改服务端状态，适合现场排查使用。

#### SDK 控制录音
```bash
# 推荐服务端启用 GPIO 录音链路
./ai-core --enable-gpio

cd examples/record_audio_example
./../build/record_audio_example
```

#### 媒体资源切换控制台
```bash
cd examples/media_resource_control
./../build/media_resource_control
```

#### 文本事件客户端
```bash
cd examples/text_event_example
./../build/text_event_client
```

### 3. 集成到自己的项目

#### 链接 SDK 库
```bash
arm-rockchip831-linux-uclibcgnueabihf-gcc \
    -o my_app my_app.c \
    -I/path/to/ai_glass_sdk/include \
    -L/path/to/ai_glass_sdk/lib \
    -lai_glass_sdk \
    -lpthread -lrt
```

### 4. GPIO 最小示例

```c
#include "ai_gpio.h"
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

static volatile int running = 1;

static void signal_handler(int sig) {
    (void)sig;
    running = 0;
}

static void my_callback(gpio_event_t event, int gpio, void *data) {
    (void)data;
    if (event == GPIO_EVENT_PRESS) {
        printf("按键按下 GPIO%d\n", gpio);
    }
}

int main(void) {
    gpio_event_client_t client = {0};
    signal(SIGINT, signal_handler);

    ai_gpio_event_client_create(&client);
    ai_gpio_event_client_connect(&client);
    ai_gpio_event_client_subscribe(&client, my_callback, NULL);

    while (running) {
        sleep(1);
    }

    ai_gpio_event_client_unsubscribe(&client);
    ai_gpio_event_client_destroy(&client);
    return 0;
}
```

## 📋 API参考

### GPIO 事件客户端 API

| API函数 | 说明 |
| --- | --- |
| `ai_gpio_event_client_create()` | 创建客户端实例 |
| `ai_gpio_event_client_connect()` | 连接到 GPIO 事件服务（默认 GPIO 1） |
| `ai_gpio_event_client_connect_gpio()` | 连接到指定 GPIO 服务 |
| `ai_gpio_event_client_subscribe()` | 订阅 GPIO 事件（异步回调） |
| `ai_gpio_event_client_unsubscribe()` | 取消订阅 |
| `ai_gpio_event_client_disconnect()` | 断开连接 |
| `ai_gpio_event_client_destroy()` | 销毁客户端 |
| `ai_gpio_event_client_is_service_alive()` | 检查服务是否可用 |

### 摄像头客户端 API

| API函数 | 说明 |
| --- | --- |
| `ai_core_init()` | 初始化摄像头客户端 |
| `ai_core_capture()` | 捕获图像数据 |
| `ai_core_free_data()` | 释放图像数据 |
| `ai_core_cleanup()` | 清理客户端资源 |
| `ai_core_get_error_string()` | 获取错误信息 |

### 音频客户端 API

| API函数 | 说明 |
| --- | --- |
| `ai_audio_init()` | 初始化音频客户端 |
| `ai_audio_play()` | 播放音频文件 |
| `ai_audio_stop()` | 停止当前播放 |
| `ai_audio_set_disable_aicore_physical_actions()` | 设置是否禁用 AI-Core 物理动作 |
| `ai_audio_get_disable_aicore_physical_actions()` | 查询是否禁用 AI-Core 物理动作 |
| `ai_audio_record_start()` | 启动录音 |
| `ai_audio_record_stop()` | 停止录音并获取录音文件路径 |
| `ai_audio_record_get_status()` | 查询当前录音状态 |
| `ai_audio_suspend_resources()` | 请求 AI-Core 释放资源 |
| `ai_audio_resume_resources()` | 请求 AI-Core 回收资源 |
| `ai_audio_get_resource_status()` | 查询当前资源状态 |
| `ai_audio_cleanup()` | 清理客户端资源 |
| `ai_audio_get_error_string()` | 获取错误信息 |
| `ai_audio_play_simple()` | 简化播放（使用默认参数） |

### 显示客户端 API

| API函数 | 说明 |
| --- | --- |
| `ai_display_init()` | 初始化显示客户端 |
| `ai_display_connect()` | 连接到显示服务 |
| `ai_display_get_framebuffer()` | 获取主帧缓冲指针 |
| `ai_display_get_framebuffer_slot()` | 获取指定槽位帧缓冲指针 |
| `ai_display_commit_frame()` | 提交帧更新到屏幕 |
| `ai_display_request_focus()` | 请求显示焦点 |
| `ai_display_cleanup()` | 清理客户端资源 |
| `ai_display_is_connected()` | 检查连接状态 |
| `ai_display_get_error_string()` | 获取错误信息 |

### 文本事件客户端 API

| API函数 | 说明 |
| --- | --- |
| `ai_text_event_client_create()` | 创建事件客户端 |
| `ai_text_event_client_start()` | 连接并开始监听 |
| `ai_text_event_client_destroy()` | 销毁客户端 |

### BLE 文本客户端 API

| API函数 | 说明 |
| --- | --- |
| `ai_ble_client_create()` | 创建 BLE 文本客户端 |
| `ai_ble_client_start()` | 启动后台接收线程并连接 `/var/run/ai_ble.sock` |
| `ai_ble_register_datatype()` | 订阅指定 `datatype` 并注册回调 |
| `ai_ble_unregister_datatype()` | 取消订阅指定 `datatype` |
| `ai_ble_send()` | 通过 BLE notify 向手机端发送 UTF-8 JSON 文本 |
| `ai_ble_client_stop()` | 停止 BLE 文本客户端 |
| `ai_ble_client_destroy()` | 销毁 BLE 文本客户端 |

### 日志系统 API

| API函数 | 说明 |
| --- | --- |
| `log_info()` | 输出信息级别日志（带毫秒级时间戳） |
| `log_error()` | 输出错误级别日志（带毫秒级时间戳） |
| `log_debug()` | 输出调试级别日志（带毫秒级时间戳） |
| `log_warn()` | 输出警告级别日志（带毫秒级时间戳） |

## 📚 文档索引

完整文档目录请参考：[docs/README.md](docs/README.md)

### 核心 API 文档
| 文档 | 说明 |
| --- | --- |
| [GPIO_Client_API.md](docs/GPIO_Client_API.md) | GPIO 客户端 API 完整文档 |
| [Camera_Client_API.md](docs/Camera_Client_API.md) | 摄像头客户端 API 文档 |
| [Audio_Client_API.md](docs/Audio_Client_API.md) | 音频客户端 API 文档 |
| [Display_Client_API.md](docs/Display_Client_API.md) | 显示客户端 API 文档 |
| [BLE_Client_API.md](docs/BLE_Client_API.md) | BLE 文本客户端 API 文档 |
| [Text_Event_Client_API.md](docs/Text_Event_Client_API.md) | 文本事件客户端 API 文档 |
| [Log_API.md](docs/Log_API.md) | 日志系统 API 文档 |

### 示例程序文档
| 文档 | 说明 |
| --- | --- |
| [GPIO事件客户端示例](examples/gpio_example/README.md) | GPIO 事件订阅完整示例 |
| [摄像头客户端示例](examples/camera_capture_example/README.md) | 单帧抓拍并保存图像 |
| [音频播放客户端示例](examples/audio_play_example/README.md) | 音频播放示例 |
| [禁用 AI-Core 物理动作示例](examples/disable_aicore_physical_actions_example/README.md) | 通过 SDK 禁用 AI-Core 自动物理按键动作 |
| [只读查询 AI-Core 物理动作状态示例](examples/query_aicore_physical_actions_example/README.md) | 只查询 `disable_aicore_physical_actions` 当前状态，不修改服务端运行态 |
| [SDK 控制录音示例](examples/record_audio_example/README.md) | SDK 控制开始/停止录音并复制到固定路径 |
| [媒体资源切换控制台示例](examples/media_resource_control/README.md) | 相机/音频资源释放与回收示例 |
| [文本事件客户端示例](examples/text_event_example/README.md) | 文本流监听完整示例 |

## ⚙️ 前置条件

1. **服务端必须先启动**
   ```bash
   # GPIO 模式
   ./ai-core --enable-gpio --gpio-number 1

   # 摄像头模式
   ./ai-core --enable-camera --enable-jpeg

   # 组合模式
   ./ai-core --enable-gpio --enable-camera
   ```

2. **系统库依赖**
   - `pthread`
   - `rt`

3. **交叉编译工具链**
   - `arm-rockchip831-linux-uclibcgnueabihf-gcc`

## 📌 注意事项

### GPIO 事件服务
- 最多支持 64 个并发客户端
- 事件延迟 < 13ms
- 服务端停止后，客户端可正常退出（不会阻塞）

### 摄像头服务
- 支持 JPEG 和 NV12 两种格式
- 共享内存大小 4 MB（足够 1920x1080 图像）
- 支持多客户端并发访问
- 动态资源管理：首个客户端连接时创建，最后一个断开时清理

### 音频控制
- 支持 PCM 播放、录音、物理交互控制和资源仲裁
- `record_audio_example` 当前依赖 `--enable-gpio` 录音链路
- `media_resource_control` 建议与正常退出的 `rkipc` 配合使用

### BLE 文本通道
- 依赖 `bt_service` 提供 `/var/run/ai_ble.sock`
- 数据包为包含 `datatype` 与 `data` 的 UTF-8 JSON 文本
- 编码后的整包不得超过 180 字节

## 🔧 故障排查

### 客户端连接失败
```bash
ps aux | grep ai-core
ls -la /tmp/ai-core_* /tmp/ai_gpio_event_*
ls -la /dev/shm/ai_*
```

### GPIO 事件收不到
```bash
cat /sys/class/gpio/gpio1/value
```

### 摄像头捕获超时
```bash
ls -la /dev/video*
```

### BLE 文本通道连接失败
```bash
ps aux | grep bt_service
ls -la /var/run/ai_ble.sock
```

## 📄 许可证

遵循与 AI Core Service 主项目相同的许可证。

## 🔗 相关链接

- 主项目：`../service/`
- 示例程序：`examples/`
- SDK 文档中心：`docs/README.md`

## 📧 技术支持

详细接入文档请参考 `docs/` 目录下的各个文档文件。
