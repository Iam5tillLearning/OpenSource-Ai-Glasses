# AI Glass SDK - Client Development Kit

[中文版本](README.md) | English Version

## Introduction

This SDK provides a complete client development kit for AI Core Service, including GPIO event subscription, camera capture, audio playback, recording control, physical interaction control, media resource arbitration, display submission, and text event listening.

Note: example programs are linked against `lib/libai_glass_sdk.a` or `lib/libai_glass_sdk.so`. They no longer compile by directly referencing sources under `src/`.

## 📦 SDK Contents

```text
ai_glass_sdk/
├── include/              # Header files
│   ├── ai_gpio.h                  # GPIO event client API
│   ├── ai_ipc.h                   # IPC communication API
│   ├── ai_camera.h                # Camera client API
│   ├── ai_audio.h                 # Audio client API
│   ├── ai_display.h               # Display client API
│   ├── ai_text_event.h            # Text event client API
│   └── ai_log.h                   # Log system API
├── lib/                  # Prebuilt libraries
│   ├── libai_glass_sdk.a          # Static library
│   └── libai_glass_sdk.so         # Shared library
├── examples/             # Example programs
│   ├── gpio_example/                         # GPIO event subscription example
│   ├── audio_play_example/                   # Audio playback example
│   ├── camera_capture_example/               # Camera capture example
│   ├── disable_physical_interaction_example/ # Disable AI-Core physical actions example
│   ├── record_audio_example/                 # SDK-controlled recording example
│   ├── media_resource_control/               # Camera/audio resource switch console
│   ├── text_event_example/                   # ASR / LLM / System text stream example
│   ├── http_example/                         # HTTP client example
│   └── websocket_example/                    # WebSocket client example
├── docs/                 # Client integration docs
│   ├── GPIO_Client_API.en.md       # GPIO client API documentation
│   ├── Camera_Client_API.en.md     # Camera client API documentation
│   ├── Audio_Client_API.en.md      # Audio client API documentation
│   ├── Display_Client_API.en.md    # Display client API documentation
│   ├── Text_Event_Client_API.en.md # Text event client API documentation
│   └── Log_API.en.md               # Log API documentation
├── README.md             # Chinese guide
├── README.en.md          # English guide
├── Makefile              # SDK build script
└── VERSION               # Version information
```

## 🎯 Main Features

### 1. GPIO Event Subscription
- Multi-process GPIO button monitoring
- Asynchronous callback mechanism
- Low latency (< 13 ms)

### 2. Camera Access
- Zero-copy image transfer through shared memory
- JPEG and NV12 support
- Multi-client concurrent access

### 3. Audio Control
- PCM playback and audio control
- Recording start / stop / status query
- SDK-side control over AI-Core physical button business actions
- Unified resource arbitration for camera/audio release and resume

### 4. Display Service
- Shared-memory framebuffer transport
- Dual slots (main display + overlay)
- Multi-client focus management

### 5. Text Event Subscription
- Listen to ASR / LLM / System text streams
- Supports streaming output and final-result markers
- Dedicated receive thread without blocking the main loop

## 🚀 Quick Start

### 1. Build the SDK

```bash
# Build SDK libraries and the default example set
cd ai_glass_sdk
make
```

To build only selected examples:

```bash
cd examples/audio_play_example && make
cd ../camera_capture_example && make
cd ../disable_physical_interaction_example && make
cd ../record_audio_example && make
cd ../media_resource_control && make
```

### 2. Run Example Programs

#### GPIO Event Client
```bash
# Make sure the service is running (the sample listens to GPIO 75)
./ai-core --enable-gpio --gpio-number 1 --gpio-numbers 0,1,75

cd examples/gpio_example
./../build/gpio_example -g 75
```

#### Camera Client
```bash
# Make sure camera is enabled on the service side
./ai-core --enable-camera --enable-jpeg

cd examples/camera_capture_example
./../build/camera_capture_example /tmp
```

#### Audio Playback Client
```bash
./ai-core

cd examples/audio_play_example
./../build/audio_play_example -f /path/to/audio.pcm -v 80 -r 48000
```

#### Disable AI-Core Physical Interaction Actions
```bash
cd examples/disable_physical_interaction_example
./../build/disable_physical_interaction_example
```

#### SDK-Controlled Recording
```bash
# Recommended to enable the GPIO recording loop on the service side
./ai-core --enable-gpio

cd examples/record_audio_example
./../build/record_audio_example
```

#### Media Resource Control Console
```bash
cd examples/media_resource_control
./../build/media_resource_control
```

#### Text Event Client
```bash
cd examples/text_event_example
./../build/text_event_client
```

### 3. Integrate Into Your Own Project

```bash
arm-rockchip831-linux-uclibcgnueabihf-gcc \
    -o my_app my_app.c \
    -I/path/to/ai_glass_sdk/include \
    -L/path/to/ai_glass_sdk/lib \
    -lai_glass_sdk \
    -lpthread -lrt
```

### 4. Minimal GPIO Example

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
        printf("Button pressed GPIO%d\n", gpio);
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

## 📋 API Reference

### GPIO Event Client API

| API Function | Description |
| --- | --- |
| `ai_gpio_event_client_create()` | Create a client instance |
| `ai_gpio_event_client_connect()` | Connect to the GPIO event service |
| `ai_gpio_event_client_connect_gpio()` | Connect to a specific GPIO service |
| `ai_gpio_event_client_subscribe()` | Subscribe to GPIO events |
| `ai_gpio_event_client_unsubscribe()` | Unsubscribe |
| `ai_gpio_event_client_disconnect()` | Disconnect |
| `ai_gpio_event_client_destroy()` | Destroy the client |
| `ai_gpio_event_client_is_service_alive()` | Check service availability |

### Camera Client API

| API Function | Description |
| --- | --- |
| `ai_core_init()` | Initialize the camera client |
| `ai_core_capture()` | Capture image data |
| `ai_core_free_data()` | Free image data |
| `ai_core_cleanup()` | Cleanup the camera client |
| `ai_core_get_error_string()` | Get error text |

### Audio Client API

| API Function | Description |
| --- | --- |
| `ai_audio_init()` | Initialize the audio client |
| `ai_audio_play()` | Play an audio file |
| `ai_audio_stop()` | Stop the current playback |
| `ai_audio_set_button_response()` | Control whether AI-Core reacts to physical actions |
| `ai_audio_get_button_response()` | Query physical action response state |
| `ai_audio_set_sdk_control_mode()` | Set SDK control mode |
| `ai_audio_get_sdk_control_mode()` | Query SDK control mode |
| `ai_audio_record_start()` | Start recording |
| `ai_audio_record_stop()` | Stop recording and get the file path |
| `ai_audio_record_get_status()` | Query recording state |
| `ai_audio_suspend_resources()` | Ask AI-Core to release resources |
| `ai_audio_resume_resources()` | Ask AI-Core to reclaim resources |
| `ai_audio_get_resource_status()` | Query current resource state |
| `ai_audio_cleanup()` | Cleanup the client |
| `ai_audio_get_error_string()` | Get error text |
| `ai_audio_play_simple()` | Simplified playback helper |

### Display Client API

| API Function | Description |
| --- | --- |
| `ai_display_init()` | Initialize the display client |
| `ai_display_connect()` | Connect to the display service |
| `ai_display_get_framebuffer()` | Get the primary framebuffer pointer |
| `ai_display_get_framebuffer_slot()` | Get a framebuffer pointer for a specific slot |
| `ai_display_commit_frame()` | Commit a frame to the screen |
| `ai_display_request_focus()` | Request display focus |
| `ai_display_cleanup()` | Cleanup the display client |
| `ai_display_is_connected()` | Check connection state |
| `ai_display_get_error_string()` | Get error text |

### Text Event Client API

| API Function | Description |
| --- | --- |
| `ai_text_event_client_create()` | Create an event client |
| `ai_text_event_client_start()` | Connect and start listening |
| `ai_text_event_client_destroy()` | Destroy the client |

### Log API

| API Function | Description |
| --- | --- |
| `log_info()` | Output info logs with millisecond timestamps |
| `log_error()` | Output error logs with millisecond timestamps |
| `log_debug()` | Output debug logs with millisecond timestamps |
| `log_warn()` | Output warning logs with millisecond timestamps |

## 📚 Documentation Index

For the full document list, see [docs/README.en.md](docs/README.en.md).

### Core API Documents
| Document | Description |
| --- | --- |
| [GPIO_Client_API.en.md](docs/GPIO_Client_API.en.md) | Full GPIO client API guide |
| [Camera_Client_API.en.md](docs/Camera_Client_API.en.md) | Camera client API guide |
| [Audio_Client_API.en.md](docs/Audio_Client_API.en.md) | Audio client API guide |
| [Display_Client_API.en.md](docs/Display_Client_API.en.md) | Display client API guide |
| [Text_Event_Client_API.en.md](docs/Text_Event_Client_API.en.md) | Text event client API guide |
| [Log_API.en.md](docs/Log_API.en.md) | Log API guide |

### Example Documents
| Document | Description |
| --- | --- |
| [GPIO Event Client Example](examples/gpio_example/README.en.md) | Full GPIO event subscription example |
| [Camera Client Example](examples/camera_capture_example/README.en.md) | One-shot image capture example |
| [Audio Playback Example](examples/audio_play_example/README.en.md) | Audio playback example |
| [Disable Physical Interaction Example](examples/disable_physical_interaction_example/README.en.md) | Disable AI-Core auto physical-button actions through the SDK |
| [Record Audio Example](examples/record_audio_example/README.en.md) | Start/stop recording and copy the recorded file |
| [Media Resource Control Example](examples/media_resource_control/README.en.md) | Release and resume camera/audio resources |
| [Text Event Client Example](examples/text_event_example/README.en.md) | Listen to text streams |

## ⚙️ Prerequisites

1. **The server must be started first**
   ```bash
   # GPIO mode
   ./ai-core --enable-gpio --gpio-number 1

   # Camera mode
   ./ai-core --enable-camera --enable-jpeg

   # Combined mode
   ./ai-core --enable-gpio --enable-camera
   ```

2. **System library dependencies**
   - `pthread`
   - `rt`

3. **Cross-compilation toolchain**
   - `arm-rockchip831-linux-uclibcgnueabihf-gcc`

## 📌 Notes

### GPIO Event Service
- Supports up to 64 concurrent clients
- Event latency < 13 ms
- Clients can exit normally when the server stops

### Camera Service
- Supports JPEG and NV12 formats
- Shared memory size is 2 MB, enough for 1920x1080 images
- Multi-client concurrent access is supported
- Resources are created on the first client connection and cleaned on the last disconnect

### Audio Control
- Supports PCM playback, recording, physical interaction control, and resource arbitration
- `record_audio_example` currently depends on the `--enable-gpio` recording loop
- `media_resource_control` should be paired with a cleanly exited `rkipc`

## 🔧 Troubleshooting

### Client Connection Failure
```bash
ps aux | grep ai-core
ls -la /tmp/ai-core_* /tmp/ai_gpio_event_*
ls -la /dev/shm/ai_*
```

### GPIO Events Not Received
```bash
cat /sys/class/gpio/gpio1/value
```

### Camera Capture Timeout
```bash
ls -la /dev/video*
```

## 📄 License

This SDK follows the same license as the main AI Core Service project.

## 🔗 Related Links

- Main project: `../service/`
- Examples: `examples/`
- SDK doc center: `docs/README.en.md`

## 📧 Support

See the documents under `docs/` for integration details.
