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
- [Audio Client API](Audio_Client_API.en.md) - Audio playback and TTS functions
- [Log System API](Log_API.en.md) - Unified log output and millisecond timestamp

### 🗣️ TTS Function Topic
- [TTS Client API](TTS_Client_API.en.md) - Complete guide to TTS function usage, configuration, protocol, and troubleshooting

### 🔧 Example Program Documentation
- [GPIO Event Client](../examples/gpio_client/) - GPIO event subscription example
- [Camera Client](../examples/example_media_client/) - Image capture example
- [Audio Playback Client](../examples/audio_play_client/) - PCM playback and TTS function example

---

## 📖 Recommended Reading Path

### 1. Beginners
1. Read [SDK Quick Start](../README.en.md) to understand basic concepts
2. View [GPIO Client API](GPIO_Client_API.en.md) to learn event subscription
3. Run [GPIO Event Client Example](../examples/gpio_client/)

### 2. Camera Development
1. Read [Camera Client API](Camera_Client_API.en.md)
2. View [Camera Client Example](../examples/example_media_client/)
3. Understand shared memory zero-copy mechanism

### 3. Audio Development
1. Read [Audio Client API](Audio_Client_API.en.md)
2. View [Audio Playback Client Example](../examples/audio_play_client/)
3. For TTS function, read [TTS Topic Documentation](#-tts-function-topic)

### 4. TTS Function Development
1. Read [TTS Client API](TTS_Client_API.en.md)
2. Configure TTS server and client according to the guide
3. Check the troubleshooting section in the guide if you encounter problems

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
- **Audio Playback Control** - PCM playback and TTS text-to-speech
- **TTS Cache** - Intelligent MD5 cache to reduce cloud calls

---

## 🔗 Quick Links

### Common API Quick Reference
- `ai_gpio_event_client_create()` - Create GPIO client
- `ai_core_init()` - Initialize camera client
- `ai_audio_init()` - Initialize audio client
- `ai_audio_play_tts_simple()` - Simple TTS playback
- `log_info()` - Output info log (with timestamp)
- `log_error()` - Output error log (with timestamp)

### Example Program Paths
- GPIO Example: `../examples/gpio_client/`
- Audio Example: `../examples/audio_play_client/`
- Camera Example: `../examples/example_media_client/`

### Header File Locations
- GPIO API: `../include/ai_gpio.h`
- Camera API: `../include/ai_camera.h`
- Audio API: `../include/ai_audio.h`
- IPC Base: `../include/ai_ipc.h`
- Log API: `../include/ai_log.h`

---

## ❓ Get Help

### Common Issues
1. **Client Connection Failure** - Check if server is started
2. **GPIO Events Not Received** - Confirm GPIO hardware configuration
3. **TTS Playback Failure** - View troubleshooting section in [TTS Client API](TTS_Client_API.en.md)
4. **Camera Capture Timeout** - Check device permissions and 3A initialization

### Documentation Feedback
If you find issues or have improvement suggestions during use, please feedback via:
- Submit Issue to project repository
- Contact technical support team

---

*Last Updated: 2025-10-27*
