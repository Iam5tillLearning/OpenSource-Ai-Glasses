# AI Glass SDK Documentation Center

[中文版本](README.md) | English Version

Welcome to the AI Glass SDK Documentation Center. This contains all the documentation needed to use the AI Glass Client SDK.

---

## 📚 Documentation Classification

### 🎯 Quick Start
- [SDK Quick Start](../README.en.md) - SDK overview and basic usage
- [Installation and Compilation](../README.en.md#-quick-start) - How to compile SDK and example programs

### 🔌 Client API Documentation
- [GPIO Client API](GPIO_Client_API.en.md) - GPIO button event subscription and asynchronous callback
- [Camera Client API](Camera_Client_API.en.md) - Image capture and zero-copy transmission
- [Audio Client API](Audio_Client_API.en.md) - Audio playback and resource control
- [Display Client API](Display_Client_API.en.md) - Framebuffer submission and display focus management
- [BLE Text Client API](BLE_Client_API.en.md) - Subscribe/send `datatype` text messages
- [Text Event Client API](Text_Event_Client_API.en.md) - ASR / LLM / System text stream listener
- [Log System API](Log_API.en.md) - Unified log output and millisecond timestamp

### 🔧 Example Program Documentation
- [GPIO Event Client](../examples/gpio_example/) - GPIO event subscription example
- [Camera Client](../examples/camera_capture_example/) - Image capture example
- [Audio Playback Client](../examples/audio_play_example/) - Audio playback example
- [Disable AI-Core Physical Actions Example](../examples/disable_aicore_physical_actions_example/) - Disable AI-Core auto physical button actions
- [Query AI-Core Physical Actions Example](../examples/query_aicore_physical_actions_example/) - Query physical action state without modifying runtime state
- [Record Audio Example](../examples/record_audio_example/) - Start/stop recording and copy to a fixed path
- [Media Resource Control](../examples/media_resource_control/) - Camera/audio resource release and resume example
- [Text Event Client](../examples/text_event_example/) - Text stream listener example
- [BLE Roundtrip Demo](../examples/ble_demo/) - Android and glasses-side BLE text roundtrip example

---

## 📖 Recommended Reading Path

### 1. Beginners
1. Read [SDK Quick Start](../README.en.md) to understand basic concepts
2. View [GPIO Client API](GPIO_Client_API.en.md) to learn event subscription
3. Run [GPIO Event Client Example](../examples/gpio_example/)

### 2. Camera Development
1. Read [Camera Client API](Camera_Client_API.en.md)
2. View [Camera Client Example](../examples/camera_capture_example/)
3. Understand shared memory zero-copy mechanism

### 3. Audio Development
1. Read [Audio Client API](Audio_Client_API.en.md)
2. View [Audio Playback Client Example](../examples/audio_play_example/)
3. To disable AI-Core physical actions, view [Disable AI-Core Physical Actions Example](../examples/disable_aicore_physical_actions_example/)
4. To trigger recording from the SDK, view [Record Audio Example](../examples/record_audio_example/)
5. For app switching, view [Media Resource Control](../examples/media_resource_control/)

### 4. BLE Text Development
1. Read [BLE Text Client API](BLE_Client_API.en.md)
2. Confirm `bt_service` exposes `/var/run/ai_ble.sock`
3. View [BLE Roundtrip Demo](../examples/ble_demo/)
4. Subscribe by `datatype`, such as `display.text`

---

## 🏗️ SDK Architecture Overview

```
External Application
    ↓
AI Glass SDK (This SDK)
    ↓
AI Media Service (Server)
    ↓
Hardware Resources (GPIO, Camera, Audio)
```

### Supported Function Modules
- **GPIO Event Subscription** - Multi-process GPIO event listening
- **Camera Access** - Zero-copy image transmission
- **Audio Control** - PCM playback, recording control, and resource arbitration
- **Display Service** - Framebuffer transport and focus management
- **BLE Text Channel** - Route BLE text messages by `datatype`
- **Text Event Subscription** - ASR / LLM / System text stream listener

---

## 🔗 Quick Links

### Common API Quick Reference
- `ai_gpio_event_client_create()` - Create GPIO client
- `ai_core_init()` - Initialize camera client
- `ai_audio_init()` - Initialize audio client
- `ai_display_init()` - Initialize display client
- `ai_display_commit_frame()` - Commit display frame
- `ai_ble_client_create()` - Create BLE text client
- `ai_ble_send()` - Send BLE text message
- `log_info()` - Output info log (with timestamp)
- `log_error()` - Output error log (with timestamp)

### Example Program Paths
- GPIO Example: `../examples/gpio_example/`
- Audio Example: `../examples/audio_play_example/`
- Camera Example: `../examples/camera_capture_example/`
- Disable AI-Core Physical Actions Example: `../examples/disable_aicore_physical_actions_example/`
- Query AI-Core Physical Actions Example: `../examples/query_aicore_physical_actions_example/`
- Record Audio Example: `../examples/record_audio_example/`
- Media Resource Example: `../examples/media_resource_control/`
- BLE Roundtrip Demo: `../examples/ble_demo/`

### Header File Locations
- GPIO API: `../include/ai_gpio.h`
- Camera API: `../include/ai_camera.h`
- Audio API: `../include/ai_audio.h`
- Display API: `../include/ai_display.h`
- BLE API: `../include/ai_ble.h`
- Text Event API: `../include/ai_text_event.h`
- IPC Base: `../include/ai_ipc.h`
- Log API: `../include/ai_log.h`

---

## ❓ Get Help

### Common Issues
1. **Client Connection Failure** - Check if server is started
2. **GPIO Events Not Received** - Confirm GPIO hardware configuration
4. **Camera Capture Timeout** - Check device permissions and 3A initialization

### Documentation Feedback
If you find issues or have improvement suggestions during use, please feedback via:
- Submit Issue to project repository
- Contact technical support team

---

*Last Updated: 2026-05-14*
