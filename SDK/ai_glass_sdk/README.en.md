# AI Glass SDK - Client Development Kit

[中文版本](README.md) | English Version

## Introduction

This SDK provides a complete client development kit for AI Core Service, supporting GPIO event subscription and camera access functions.

Note: the current release package contains headers, prebuilt libraries, and examples only. It does not ship a `src/` directory, so examples should link against `lib/libai_glass_sdk.a` or `lib/libai_glass_sdk.so`.

## 📦 SDK Contents

```
ai_glass_sdk/
├── include/              # Header files
│   ├── ai_gpio.h                  # GPIO event client API
│   ├── ai_ipc.h                   # IPC communication API
│   ├── ai_camera.h                # Camera client API
│   ├── ai_audio.h                 # Audio client API
│   └── ai_log.h                   # Log system API
├── lib/                  # Compiled library files
│   ├── libai_glass_sdk.a          # Static library
│   └── libai_glass_sdk.so         # Dynamic library
├── examples/             # Example programs
│   ├── gpio_example/              # GPIO event client example
│   ├── audio_play_example/        # Audio play client example
│   ├── camera_capture_example/    # Camera client example
│   └── text_event_example/        # ASR/LLM text event example
├── docs/                 # Client integration documentation
│   ├── GPIO_Event_Service.md      # GPIO event service full documentation
│   ├── Camera_Client_API.md       # Camera client API documentation
│   └── Audio_Client_API.md        # Audio client API documentation
├── README.md             # This file
└── VERSION               # Version information
```

## 🎯 Main Features

### 1. GPIO Event Subscription
- Supports multi-process concurrent listening for GPIO button events
- Asynchronous event callback mechanism
- Low latency (< 13ms)

### 2. Camera Access
- Zero-copy image transmission via shared memory
- Supports JPEG and NV12 formats
- Multi-client concurrent support

### 3. Audio Playback Control
- Unix Socket communication for audio playback control
- Supports volume adjustment, sample rate configuration
- Force play and stop functions

## 🚀 Quick Start

### 1. Build Example Programs

```bash
# The SDK package already ships prebuilt libraries.
cd ai_glass_sdk/examples/gpio_example && make
cd ../audio_play_example && make
cd ../camera_capture_example && make
cd ../text_event_example && make
```

### 2. Run Example Programs

#### GPIO Event Client
```bash
# Ensure server is started
./ai-core --enable-gpio --gpio-number 1

# Run GPIO client example
cd examples/gpio_example
./../build/gpio_example
```

#### Camera Client
```bash
# Ensure server enables camera
./ai-core --enable-camera --enable-jpeg

# Run camera client example
cd examples/camera_capture_example
./../build/camera_capture_example /tmp
```

#### Audio Playback Client
```bash
# Ensure server is started
./ai-core

# Play audio file
cd examples/audio_play_example
./../build/audio_play_example -f /path/to/audio.pcm -v 80 -r 48000
```

### 3. Integrate into Your Project

#### Link SDK Library
```bash
arm-rockchip831-linux-uclibcgnueabihf-gcc \
    -o my_app my_app.c \
    -I/path/to/ai_glass_sdk/include \
    -L/path/to/ai_glass_sdk/lib \
    -lai_glass_sdk \
    -lpthread -lrt
```

### 4. Minimal GPIO Event Client Example

```c
#include "ai_gpio.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

static volatile int running = 1;

void signal_handler(int sig) { running = 0; }

void my_callback(gpio_event_t event, int gpio, void *data) {
    if (event == GPIO_EVENT_PRESS) {
        printf("Button pressed GPIO%d\n", gpio);
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

## 📋 API Reference

### GPIO Event Client API

| API Function | Description |
|---------|------|
| `ai_gpio_event_client_create()` | Create client instance |
| `ai_gpio_event_client_connect()` | Connect to GPIO event service |
| `ai_gpio_event_client_subscribe()` | Subscribe to GPIO events (asynchronous callback) |
| `ai_gpio_event_client_unsubscribe()` | Unsubscribe |
| `ai_gpio_event_client_disconnect()` | Disconnect |
| `ai_gpio_event_client_destroy()` | Destroy client |
| `ai_gpio_event_client_is_service_alive()` | Check if service is alive |

### Camera Client API

| API Function | Description |
|---------|------|
| `ai_core_init()` | Initialize camera client |
| `ai_core_capture()` | Capture image data |
| `ai_core_free_data()` | Free image data |
| `ai_core_cleanup()` | Cleanup client resources |
| `ai_core_get_error_string()` | Get error string |

### Audio Client API

| API Function | Description |
|---------|------|
| `ai_audio_init()` | Initialize audio client |
| `ai_audio_play()` | Play audio file |
| `ai_audio_stop()` | Stop current playback |
| `ai_audio_cleanup()` | Cleanup client resources |
| `ai_audio_get_error_string()` | Get error string |
| `ai_audio_play_simple()` | Simple playback (use default parameters) |

### Log System API

| API Function | Description |
|---------|------|
| `log_info()` | Output info level log (with millisecond timestamp) |
| `log_error()` | Output error level log (with millisecond timestamp) |
| `log_debug()` | Output debug level log (with millisecond timestamp) |
| `log_warn()` | Output warn level log (with millisecond timestamp) |

## 📚 Documentation Index

### Core API Documentation
| Document | Description |
|------|------|
| [GPIO_Client_API.en.md](docs/GPIO_Client_API.en.md) | GPIO Client API Full Documentation (Event Subscription, Async Callback) |
| [Camera_Client_API.en.md](docs/Camera_Client_API.en.md) | Camera Client API Documentation (Zero-copy Image Capture) |
| [Audio_Client_API.en.md](docs/Audio_Client_API.en.md) | Audio Client API Documentation (Audio Playback Control) |
| [Log_API.en.md](docs/Log_API.en.md) | Log System API Documentation (Unified Log Output, Millisecond Timestamp) |

### Example Program Documentation
| Document | Description |
|------|------|
| [GPIO Event Client Example](examples/gpio_example/) | GPIO Event Subscription Full Example |
| [Camera Client Example](examples/camera_capture_example/) | Image Capture Full Example |
| [Audio Playback Client Example](examples/audio_play_example/) | PCM Playback and TTS Function Detailed Example |
| [Text Event Client Example](examples/text_event_example/) | Receive and print ASR/LLM text events |

## ⚙️ Prerequisites

1. **Server must be started first**
   ```bash
   # GPIO mode
   ./ai-core --enable-gpio --gpio-number 1

   # Camera mode
   ./ai-core --enable-camera --enable-jpeg

   # Combined mode
   ./ai-core --enable-gpio --enable-camera
   ```

2. **System Library Dependencies**
   - pthread (Thread library)
   - rt (Real-time extension, shared memory and semaphores)

3. **Cross-compilation Toolchain**
   - arm-rockchip831-linux-uclibcgnueabihf-gcc

## 📌 Notes

### GPIO Event Service
- Supports up to 64 concurrent clients
- Event latency < 13ms
- Clients can exit normally when server stops (will not block)

### Camera Service
- Supports JPEG and NV12 formats
- Shared memory size 2MB (enough for 1920x1080 image)
- Supports multi-client concurrent access
- Dynamic resource management (created on first client connection, cleaned up on last disconnect)

### Audio Playback Control
- Supports PCM format audio
- Sample rate: 8000-96000 Hz
- Channels: 1-8
- Bit width: 8/16/24/32 bit

## 🔧 Troubleshooting

### Client Connection Failure
```bash
# Check if server is running
ps aux | grep ai-core

# Check Unix Socket files
ls -la /tmp/ai-core_* /tmp/ai_gpio_event_*

# Check shared memory
ls -la /dev/shm/ai_*
```

### GPIO Events Not Received
```bash
# Check GPIO hardware
cat /sys/class/gpio/gpio1/value

# View server logs
# Server will output GPIO event detection info
```

### Camera Capture Timeout
```bash
# Check camera device
ls -la /dev/video*

# Check 3A initialization
# Server will output AIQ initialization logs on startup
```

## 📄 License

Follows the same license as the AI Core Service main project.

## 🔗 Related Links

- **Example Programs**: `examples/` - Contains GPIO, Camera, Audio client examples

## 📧 Support

For detailed integration documentation, please refer to the documents in the `docs/` directory.
