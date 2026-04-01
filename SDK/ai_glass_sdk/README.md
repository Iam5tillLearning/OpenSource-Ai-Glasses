# AI Glass SDK - 客户端开发套件

中文版本 | [English Version](README.en.md)

## 简介

本SDK为AI Core Service提供完整的客户端开发套件，支持GPIO事件订阅和摄像头调用功能。

说明：当前发布包为“头文件 + 预编译库 + 示例程序”形态，不包含 `src/` 源码目录；示例程序应直接链接 `lib/libai_glass_sdk.a` 或 `lib/libai_glass_sdk.so`。

## 📦 SDK内容

```
ai_glass_sdk/
├── include/              # 头文件
│   ├── ai_gpio.h                  # GPIO事件客户端API
│   ├── ai_ipc.h                   # IPC通信API
│   ├── ai_camera.h          # 摄像头客户端API
│   ├── ai_audio.h                 # 音频客户端API
│   └── ai_log.h                   # 日志系统API
├── lib/                  # 编译后的库文件
│   ├── libai_glass_sdk.a           # 静态库
│   └── libai_glass_sdk.so          # 动态库
├── examples/             # 示例程序
│   ├── gpio_example/              # GPIO事件客户端示例
│   ├── audio_play_example/        # 音频播放客户端示例
│   ├── camera_capture_example/    # 摄像头客户端示例
│   └── text_event_example/        # ASR/LLM文本事件示例
├── docs/                 # 客户端接入文档
│   ├── GPIO_Event_Service.md           # GPIO事件服务完整文档
│   ├── Camera_Client_API.md            # 摄像头客户端API文档
│   └── Audio_Client_API.md             # 音频客户端API文档
├── README.md            # 本文件
└── VERSION              # 版本信息
```

## 🎯 主要功能

### 1. GPIO事件订阅
- 支持多进程同时监听GPIO按键事件
- 异步事件回调机制
- 低延迟（< 13ms）

### 2. 摄像头调用
- 通过共享内存零拷贝图像传输
- 支持JPEG和NV12格式
- 多客户端并发支持

### 3. 音频播放控制
- Unix Socket通信控制音频播放
- 支持音量调节、采样率配置
- 强制播放和停止功能

## 🚀 快速开始

### 1. 编译示例程序

```bash
# SDK发布包已包含预编译库，直接编译示例即可
cd ai_glass_sdk/examples/gpio_example && make
cd ../audio_play_example && make
cd ../camera_capture_example && make
cd ../text_event_example && make
```

### 2. 运行示例程序

#### GPIO事件客户端
```bash
# 确保服务端已启动
./ai-core --enable-gpio --gpio-number 1

# 运行GPIO客户端示例
cd examples/gpio_example
./../build/gpio_example
```

#### 摄像头客户端
```bash
# 确保服务端启用摄像头
./ai-core --enable-camera --enable-jpeg

# 运行摄像头客户端示例
cd examples/camera_capture_example
./../build/camera_capture_example /tmp
```

#### 音频播放客户端
```bash
# 确保服务端已启动
./ai-core

# 播放音频文件
cd examples/audio_play_example
./../build/audio_play_example -f /path/to/audio.pcm -v 80 -r 48000
```

### 3. 集成到自己的项目

#### 链接SDK库
```bash
arm-rockchip831-linux-uclibcgnueabihf-gcc \
    -o my_app my_app.c \
    -I/path/to/ai_glass_sdk/include \
    -L/path/to/ai_glass_sdk/lib \
    -lai_glass_sdk \
    -lpthread -lrt
```

### 4. GPIO事件客户端最小示例

```c
#include "ai_gpio.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

static volatile int running = 1;

void signal_handler(int sig) { running = 0; }

void my_callback(gpio_event_t event, int gpio, void *data) {
    if (event == GPIO_EVENT_PRESS) {
        printf("按键按下 GPIO%d\n", gpio);
    }
}

int main() {
    gpio_event_client_t client = {0};
    signal(SIGINT, signal_handler);

    ai_gpio_event_client_create(&client);
    ai_gpio_event_client_connect(&client);
    ai_gpio_event_client_subscribe(&client, my_callback, NULL);

    while (running) sleep(1);

    ai_gpio_event_client_unsubscribe(&client);
    ai_gpio_event_client_destroy(&client);
    return 0;
}
```

## 📋 API参考

### GPIO事件客户端API

| API函数 | 说明 |
|---------|------|
| `ai_gpio_event_client_create()` | 创建客户端实例 |
| `ai_gpio_event_client_connect()` | 连接到GPIO事件服务 |
| `ai_gpio_event_client_subscribe()` | 订阅GPIO事件（异步回调） |
| `ai_gpio_event_client_unsubscribe()` | 取消订阅 |
| `ai_gpio_event_client_disconnect()` | 断开连接 |
| `ai_gpio_event_client_destroy()` | 销毁客户端 |
| `ai_gpio_event_client_is_service_alive()` | 检查服务是否可用 |

### 摄像头客户端API

| API函数 | 说明 |
|---------|------|
| `ai_core_init()` | 初始化摄像头客户端 |
| `ai_core_capture()` | 捕获图像数据 |
| `ai_core_free_data()` | 释放图像数据 |
| `ai_core_cleanup()` | 清理客户端资源 |
| `ai_core_get_error_string()` | 获取错误信息 |

### 音频客户端API

| API函数 | 说明 |
|---------|------|
| `ai_audio_init()` | 初始化音频客户端 |
| `ai_audio_play()` | 播放音频文件 |
| `ai_audio_stop()` | 停止当前播放 |
| `ai_audio_cleanup()` | 清理客户端资源 |
| `ai_audio_get_error_string()` | 获取错误信息 |
| `ai_audio_play_simple()` | 简化播放（使用默认参数） |

### 日志系统API

| API函数 | 说明 |
|---------|------|
| `log_info()` | 输出信息级别日志（带毫秒级时间戳） |
| `log_error()` | 输出错误级别日志（带毫秒级时间戳） |
| `log_debug()` | 输出调试级别日志（带毫秒级时间戳） |
| `log_warn()` | 输出警告级别日志（带毫秒级时间戳） |

## 📚 文档索引

### 核心API文档
| 文档 | 说明 |
|------|------|
| [GPIO_Client_API.md](docs/GPIO_Client_API.md) | GPIO客户端API完整文档（事件订阅、异步回调） |
| [Camera_Client_API.md](docs/Camera_Client_API.md) | 摄像头客户端API文档（零拷贝图像捕获） |
| [Audio_Client_API.md](docs/Audio_Client_API.md) | 音频客户端API文档（音频播放控制） |
| [Log_API.md](docs/Log_API.md) | 日志系统API文档（统一日志输出、毫秒级时间戳） |

### 示例程序文档
| 文档 | 说明 |
|------|------|
| [GPIO事件客户端示例](examples/gpio_example/) | GPIO事件订阅完整示例 |
| [摄像头客户端示例](examples/camera_capture_example/) | 图像捕获完整示例 |
| [音频播放客户端示例](examples/audio_play_example/) | PCM播放和TTS功能详细示例 |
| [文本事件客户端示例](examples/text_event_example/) | 接收并打印 ASR/LLM 文本事件 |

## ⚙️ 前置条件

1. **服务端必须先启动**
   ```bash
   # GPIO模式
   ./ai-core --enable-gpio --gpio-number 1

   # 摄像头模式
   ./ai-core --enable-camera --enable-jpeg

   # 组合模式
   ./ai-core --enable-gpio --enable-camera
   ```

2. **系统库依赖**
   - pthread（线程库）
   - rt（实时扩展，共享内存和信号量）

3. **交叉编译工具链**
   - arm-rockchip831-linux-uclibcgnueabihf-gcc

## 📌 注意事项

### GPIO事件服务
- 最多支持64个并发客户端
- 事件延迟 < 13ms
- 服务端停止后，客户端可正常退出（不会阻塞）

### 摄像头服务
- 支持JPEG和NV12两种格式
- 共享内存大小2MB（足够1920x1080图像）
- 支持多客户端并发访问
- 动态资源管理（首个客户端连接时创建，最后一个断开时清理）

### 音频播放控制
- 支持PCM格式音频
- 采样率：8000-96000 Hz
- 通道数：1-8
- 位宽：8/16/24/32 bit

## 🔧 故障排查

### 客户端连接失败
```bash
# 检查服务端是否运行
ps aux | grep ai-core

# 检查Unix Socket文件
ls -la /tmp/ai-core_* /tmp/ai_gpio_event_*

# 检查共享内存
ls -la /dev/shm/ai_*
```

### GPIO事件收不到
```bash
# 检查GPIO硬件
cat /sys/class/gpio/gpio1/value

# 查看服务端日志
# 服务端会输出GPIO事件检测信息
```

### 摄像头捕获超时
```bash
# 检查摄像头设备
ls -la /dev/video*

# 检查3A初始化
# 服务端启动时会输出AIQ初始化日志
```

## 📄 许可证

遵循与AI Core Service主项目相同的许可证。

## 🔗 相关链接

- **示例程序**: `examples/` - 包含GPIO、摄像头、音频三种客户端示例

## 📧 技术支持

详细接入文档请参考 `docs/` 目录下的各个文档文件。
