# 🥽 AI Smart Glasses

> [!IMPORTANT]
> # 🚧 More documentation is being added 🚧


**An open-source Linux-based smart glasses platform for medical, industrial, educational, and consumer applications**

[中文](README.zh.md) | **English**

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](https://github.com/ezxrdev/OpenSource-Ai-Glasses/actions)
[![License](https://img.shields.io/badge/license-Apache%202.0-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.7.0-green.svg)](https://github.com/Iam5tilllearning/OpenSource-Ai-Glasses/releases)


---
<img width="1024" height="747" alt="image" src="https://github.com/user-attachments/assets/af4de9f5-f502-4a74-8a2a-f5a762ff83b9" />

## 📋 Project Overview

This is a Linux-based open-source smart glasses project in early development stage (45% documentation completeness).

The open-source scope is primarily the system software and application stack. Development is not limited to our integrated hardware kit: any RV1106B-based development board can be used as a starting point, with peripheral adaptation based on your own hardware design.

**Developer Community** | **Contact**: iam5tilllearning@foxmail.com

| WeChat Group | Discord Community |
|:---:|:---:|
| <img width="200" alt="WeChat Developer Group" src="docs/Images/weixin.png" /> | <a href="https://discord.gg/7KqjKFZ7xA"><img width="200" alt="Discord Community" src="docs/Images/discord.jpg" /></a> |
| Scan to join WeChat group | [Join Discord](https://discord.gg/7KqjKFZ7xA) |




## 🧩 Hardware Options

You can develop this project in two ways:

- Use your own RV1106B-based development board to work on the open-source system and software stack. This option is cheaper, but bring-up can involve many issues, and you may spend considerable time on system compatibility, drivers, and related problems. If your goal is to quickly develop AI glasses interaction applications, we recommend buying our carefully tuned AI glasses device.
- Use our deeply integrated AI glasses kit if you want a faster bring-up path with pre-wired peripherals, preloaded firmware, and less hardware integration work.

[**Optional: Get the Official AI Glasses Kit**](https://item.taobao.com/item.htm?id=1007109700786) — *Recommended for faster bring-up, not required for development*

[**Backup purchase link**](https://item.taobao.com/item.htm?ft=t&id=1044923880613) — *Use this if the primary purchase page is unavailable*

> [!CAUTION]
> **Official Hardware Compatibility Notice**: The v0.6.x and v0.7.x reference firmware builds are **only compatible with integrated hardware produced after January 1, 2026**. If your official kit or same-reference hardware was purchased/produced before this date, please use v0.6.0 or earlier firmware versions.

## ✅ Current Version Features (v0.7.0)

- **Firmware Version: v0.7.0**
  - Compatible with new hardware produced after January 1, 2026.
- **OSAIG Mobile Client**: First standalone Android mobile client release for connecting, managing, and debugging OSAIG glasses.
- **Device Management**: Device binding, device list management, online/local connection status, device detail page, status trends, and basic device information including battery, storage, memory, and system version.
- **RTSP Real-Time Video Streaming**: The glasses can expose the camera feed through RTSP for the mobile client and for cloud-side video AI recognition pipelines. The reference main stream is `rtsp://<device_lan_ip>:554/live/0`.
- **BLE Text Interaction**: Adds a BLE text channel and mobile BLE debug entry for sending preset commands and text content to the glasses.
- **Audio Device Handling Refactor**: Improves audio input/output coordination and supports simultaneous recording and playback.
- **SDK Updates**: The SDK documentation now covers recording control, AI-Core physical action control/query, media resource arbitration, display submission, text event listening, BLE text send/receive, and updated example build entries.
- **Release Assets**:
  - `Firmware_RV1106B_RK962_IMX219.AudioVersion.img`
  - `Firmware_RV1106B_RK962_IMX219.DisplayVersion.img`
  - `osaig-0.3.0+7-release.apk`
- **Release Details**: <https://github.com/Iam5tillLearning/OpenSource-Ai-Glasses/releases/tag/v0.7.0>

<details>
<summary>📜 Version History summary</summary>

### v0.7.0 (2026-05-08)
- Added the first standalone OSAIG mobile client for device binding, management, debugging, update guidance, device information, and status trends.
- Added RTSP real-time video streaming for the glasses camera feed. The mobile client can view the stream with end-to-end/network/playback latency metrics, and developers can also consume the RTSP feed from cloud-side video AI recognition pipelines.
- Added BLE text interaction from the mobile client, including preset debug commands and a reserved command structure for future photo, video, device-control, and debugging extensions.
- Refactored audio device handling to improve recording/playback coexistence and provide a stronger base for voice interaction and audio/video debugging.
- Added SDK documentation coverage for BLE text send/receive, recording control, physical action control/query, media resource arbitration, display submission, and text event listening.
- Release assets include both AudioVersion/DisplayVersion firmware images and the OSAIG Android APK `osaig-0.3.0+7-release.apk`.

### v0.6.5 (2026-03-16)
- Fixed multiple stability issues across the project, with emphasis on AI Core service reliability.
- Enhanced edge-case and unexpected failure handling.
- Added two release variants: AudioVersion and DisplayVersion (choose by hardware type).

### v0.6.4 (2026-02-10)
- Display-enabled version feature upgrade with new system-level `display-service`.
- Integrated with Application Development SDK for third-party graphical rendering and display.
- Added experimental `launcher-app` demo for graphical interaction workflows.
- Both `display-service` and `launcher-app` are fully open-sourced for learning and customization.

### v0.6.3
- Firmware Version: v0.6.3
- Bluetooth support added (Bluetooth Name: OSAIG)
- Note: Xiaomi phones have compatibility issues
- Display version users need to handle display issues manually

### v0.6.2
- Firmware Version: v0.6.2
- SDK: Added display module (UI/Image) & power save settings
- SDK: Added ASR and LLM text retrieval capability
- Optimized 30s screen auto-off logic
- Upgraded to GPIO Hub architecture
- Optimized 3D model structure
- Added cJSON, removed old display app

### v0.6.1
- Firmware Version: v0.6.1
- Compatible with new hardware produced after January 1, 2026

### v0.6.0
- Firmware Version: v0.6.0
- New SDK released

### v0.5.0
- Firmware Version: v0.5.0
- Implemented complete WiFi provisioning logic
- Detailed audio prompts

### v0.4.0
- Encapsulated a SDK based on the core server
- Developers can implement photo taking, GPIO, recording, etc. based on this SDK

### v0.3.1
- Implemented system sentinel

### v0.3.0
- Implemented core server, responsible for photo taking, GPIO, recording, and new AI conversation

### v0.2.3
- Optimized cable slot layout

### v0.2.2
- Modified glasses front frame model, reserved position for optical engine and waveguide

### v0.2.1
- Modified temple model, determined sound cavity and antenna cavity

### v0.2.0
- First version of frame 3D printing molding

### v0.1.1
- Massively optimized AI conversation latency, first packet end-to-end latency around 1s

### v0.1.0
- Implemented basic AI conversation

</details>

## 🔍 Hardware Layout

<img width="800" alt="Hardware Layout" src="docs/Images/layout.png" />

## ✨ Key Features

- 🖥️ **Display**: 30°FOV 640×480 monocular display (optional)
- 📸 **Camera**: 1080P video recording and RTSP real-time video streaming
- 🔊 **Audio**: microphone + speaker system
- 📡 **Connectivity**: WiFi 802.11b/g/n, Bluetooth 5.3, USB 2.0
- ⚡ **Performance**: Single Cortex-A7 core, 8GB storage
- 🔋 **Battery**: 180mAh, 2hr music, 3hr display, 45min recording
- ⚖️ **Lightweight**: Only 43g
- 🧠 **Sensors**: Optional geomagnetic sensor, IMU
- 🐧 **OS**: Embedded Linux-based system

## 🚀 Quick Start

> **💡 Note**: If you purchased the [official AI glasses kit](https://item.taobao.com/item.htm?id=1007109700786) linked above, it comes with pre-installed firmware and is ready to use. If you are developing on your own RV1106B board, start from the host setup and flashing guide below.

### Using Native Host Environment (Recommended)

We recommend developing directly on Ubuntu/Debian hosts, or Ubuntu/Debian running in WSL2 on Windows.

> **Single source for development environment**: `https://github.com/makevary/AIGLASS_DEV_ENV`

```bash
# Get development environment
git clone https://github.com/makevary/AIGLASS_DEV_ENV.git
cd AIGLASS_DEV_ENV

# Fix/install build dependencies (Debian/Ubuntu and WSL2 Debian/Ubuntu)
./setup_build_env.sh

# Build firmware (choose one command only)
./build.sh
./build.sh --without-display

# Firmware output
ls -lh output/image/update.img
```

Build option selection:
- Device with display capability: `./build.sh`
- Device without display capability: `./build.sh --without-display`

> **Note**: `setup_build_env.sh` relies on `apt-get`, so it supports Debian/Ubuntu systems (including Ubuntu/Debian in WSL2).

See [Development Environment Setup Guide](docs/ENV_SETUP.en.md) for details.

**Firmware Flashing**: After compilation, please refer to [Firmware Flashing Guide](docs/FIRMWARE_FLASHING.en.md) to flash the firmware to your device.

**Application Development**: The host environment provides a complete development workflow for user-level applications. See [Application Development Guide](docs/APPLICATION_DEVELOPMENT.en.md) for details.


## 📖 Usage Guide

### 1. Network Configuration
- **Automatic**: The device automatically enters network configuration mode on boot.
- **Manual**: At any time, continuously short press the button 10 times to manually enter network configuration mode.
- **Current method**: Use an Android phone to share a Wi-Fi QR code, then let the glasses scan the QR code to connect.
- **Band limitation**: Only 2.4GHz Wi-Fi is supported. 5GHz/5G Wi-Fi is not supported.
- **App status**: The standalone OSAIG Android mobile client is available in the v0.7.0 release assets. It currently focuses on device binding, management, debugging, update guidance, BLE debugging, and real-time video viewing.

### 2. RTSP Video Streaming for Cloud AI

- **Main stream**: `rtsp://<device_lan_ip>:554/live/0`
- **Sub stream**: `rtsp://<device_lan_ip>:554/live/1`
- **Typical use**: cloud-side video AI recognition, remote inspection, visual debugging, and mobile live preview.
- **Guide**: [RTSP Video Streaming Guide](docs/RTSP_VIDEO_STREAMING.en.md)

### 3. AI Conversation
- **Operation**: After configuration, long press the left temple button to speak, release to send, and wait for the AI response.


## 📦 SDK Development

This project provides a complete C/C++ SDK, allowing developers to easily access underlying hardware capabilities and build their own applications.

**SDK Core Features:**
*   **GPIO Event Subscription**: Low-latency access to button presses and other GPIO events
*   **Camera Access**: Zero-copy image data retrieval via shared memory
*   **Audio Control**: audio playback, TTS, recording start/stop/status, and improved recording/playback coexistence
*   **Physical Action Control**: enable, disable, and query AI-Core physical-button business actions while keeping GPIO event delivery available
*   **Media Resource Arbitration**: request AI-Core to release or reclaim camera/audio resources for external applications
*   **Display Submission**: shared-memory framebuffer submission and display focus management
*   **Text Event Listening**: subscribe to ASR, LLM, and system text events
*   **BLE Text Channel**: send and receive UTF-8 JSON text messages through the glasses BLE gateway
*   **Inter-Process Communication**: Reliable communication based on Unix Domain Sockets

**v0.7.0 SDK Documentation Updates:**
*   Added BLE text client API documentation and the `ai_ble.h` header entry.
*   Added recording control APIs: `ai_audio_record_start()`, `ai_audio_record_stop()`, and `ai_audio_record_get_status()`.
*   Added physical action control/query APIs: `ai_audio_set_disable_aicore_physical_actions()` and `ai_audio_get_disable_aicore_physical_actions()`.
*   Added media resource arbitration APIs for camera/audio release, resume, and status query.
*   Added query-only physical action example to avoid changing runtime state during field checks.
*   Updated examples to link against the prebuilt SDK libraries and use the unified build output directory.

**SDK Location**: [`SDK/ai_glass_sdk`](SDK/ai_glass_sdk)

**Resource Navigation:**
*   📖 [SDK Documentation](SDK/ai_glass_sdk/README.en.md) - Detailed SDK instructions and usage
*   📚 [API Reference](SDK/ai_glass_sdk/docs/README.en.md) - Detailed API documentation
*   💡 [Examples](SDK/ai_glass_sdk/examples) - Complete example code for GPIO, camera, audio, etc.

**Integration Guide:**
Please refer to the "Integration" section in the [SDK README](SDK/ai_glass_sdk/README.en.md#3-integrate-into-your-project) to learn how to link and use the SDK in your application.


## 🏗️ System Architecture

<img width="1200" height="1050" alt="image" src="https://github.com/user-attachments/assets/79d7c2cd-09e7-43ab-b8d6-11b6b64cd5e0" />


## 📚 Documentation

- [📖 User Manual](docs/USER_GUIDE.en.md) - Read this after glasses assembly
- [🧰 Development Environment Setup Guide](docs/ENV_SETUP.en.md) | [中文](docs/ENV_SETUP.md)
- [💻 Application Development Guide](docs/APPLICATION_DEVELOPMENT.en.md) | [中文](docs/APPLICATION_DEVELOPMENT.md)
- [📡 RTSP Video Streaming Guide](docs/RTSP_VIDEO_STREAMING.en.md) | [中文](docs/RTSP_VIDEO_STREAMING.md)
- [⚡ Firmware Flashing Guide](docs/FIRMWARE_FLASHING.en.md) | [中文](docs/FIRMWARE_FLASHING.md)

## 🛠️ Development

### Build from Source

```bash
# Get the full development workspace
git clone https://github.com/makevary/AIGLASS_DEV_ENV.git
cd AIGLASS_DEV_ENV

# Install/fix build dependencies
./setup_build_env.sh

# Build firmware
./build.sh

# Output image
ls -lh output/image/update.img
```

### Development Tools

- **IDE**: VS Code with C/C++ extension
- **Debugger**: GDB + OpenOCD
- **Profiler**: perf, valgrind
- **Version Control**: Git

### API Overview

```c
#include "ai_glasses_api.h"

// Initialize device
int device_init(device_config_t *config);

// Capture photo
int capture_photo(const char *filename);

// Display text
int display_text(const char *text, int x, int y);

// Play audio
int play_audio(const char *filename);

// Get sensor data
int get_sensor_data(sensor_data_t *data);
```

## 🎯 Use Cases

### 🏥 Medical Applications

<details>
<summary>Medical AI Smart Glasses Scenarios</summary>

#### Patient Information Display
Doctors or nurses can instantly see patient name, bed number, main diagnosis, allergy history, and key vital signs in the corner of their vision upon entering patient wards, eliminating the need to repeatedly check medical records or computers.

#### Real-time Vital Signs Monitoring
The glasses can read and integrate data from bedside monitors and infusion pumps in real-time. If patient heart rate, blood oxygen, blood pressure and other indicators show abnormal fluctuations, the system will immediately highlight warnings in the field of view and emit gentle but clear alert sounds through bone conduction headphones.

#### Auxiliary Operation and Procedure Verification
When performing infusion, medication administration and other operations, the glasses' camera automatically scans drug barcodes and patient wristbands to verify "three checks and seven matches" information. If drug dosage errors, patient mismatches or allergy risks are found, warnings will immediately appear in a prominent way to prevent medical errors.

#### Contact-free Information Access
Doctors can use voice commands or gestures to virtually call up patient electronic medical records, imaging reports (such as CT/MRI), and convert spoken ward rounds into text and store them in the system during ward rounds or operations, achieving "what you see is what you record," greatly freeing up hands.

#### Remote Expert Collaboration
In complex consultations or emergency rescues, junior doctors can share real-time first-person video with remote experts. Experts can annotate, circle key points, and provide guidance via voice communication on shared video streams, as if experts were present on site, improving grassroots medical capabilities.

</details>

### 🏭 Industrial Applications

<details>
<summary>Substation Application Scenarios</summary>

#### Read Operation Tickets
Regardless of paper or electronic operation tickets, the glasses can scan and automatically extract key information (such as which equipment to operate, whether to close or open), eliminating the need for manual input and verification character by character.

#### Recognize On-site Equipment
Wearing glasses while walking in the substation, it acts like an experienced inspector, able to recognize in real-time through cameras and AI whether the current equipment is a circuit breaker, disconnector, or grounding switch.

#### Safety Rules
The system has built-in all power safety procedures and "five prevention" logic. It can compare the identified operation commands with the actual equipment status currently seen to determine if the next operation will cause problems.

#### Timely Voice Warnings
Once it finds that I might go to the wrong interval or operate the wrong equipment, it will immediately warn with voice, such as "Error! This is switch 102, please verify!", preventing mistakes. The entire process must be real-time without delay.

#### Independent On-site Work
All calculations and judgments support local deployment, ensuring functionality even in network-unstable emergency or ICU areas.

</details>

<details>
<summary>Maintenance Scenarios</summary>

#### Real-time Video Calls and Screen Sharing
On-site maintenance personnel can share first-person real-time video of faulty equipment with backend expert teams through the glasses camera. Experts can see the situation as clearly as if they were present on site, precisely understanding the on-site conditions while freeing hands during maintenance.

#### AR Annotation and Real-time Guidance
Experts can perform AR annotations (such as drawing circles, arrow indicators, text annotations) on shared video streams, directly "projecting" them into the field personnel's vision, precisely guiding them to "tighten this screw," "measure voltage at that point," greatly improving communication efficiency.

#### Multi-party Consultation and Knowledge Accumulation
Support multiple experts to simultaneously join one video session for "multi-party consultation," quickly solving complex problems. The entire guidance process can be recorded and archived to form maintenance case libraries for specific faults, used for subsequent training.

#### File and Drawing Instant Access
On-site personnel can request experts to remotely push drawings, manuals, or 3D model files through voice commands. Experts can directly send materials and display them on one side of the maintenance personnel's glasses field of view for reference while working.

</details>

### 🎓 Educational Applications

<details>
<summary>AR Intelligent Operation Guidance</summary>

#### Visualized Operation Lists
Break down complex SOPs (Standard Operating Procedures) into step-by-step AR instructions, directly superimposed and displayed on real equipment in the operator's field of vision. The current operation step to be performed will be highlighted, automatically entering the next step upon completion.

#### Tool and Material Recognition
The glasses can recognize whether the operator picked up is the tool or material specified for the current step. If the wrong one is picked up, it will immediately issue a warning to prevent equipment damage or assembly problems caused by using the wrong tool.

#### Automatic Step Confirmation and Recording
The system automatically determines whether a step is completed through visual recognition (such as "screw tightened," "cable properly connected") and automatically records completion time and operator information, achieving paperless and error-proof process confirmation.

#### Voice Navigation When Hands Are Busy
When operators have both hands occupied, they can control the guidance process playback through voice commands like "next," "previous," "repeat," completely freeing hands to focus on the operation itself.

#### New Employee Training and Skill Transfer
New employees can quickly get started with AR guidance, reducing training costs and error rates. Best practices and operating techniques of experienced workers can also be solidified through AR processes for efficient knowledge transfer and standardized operations.

</details>

## 🤝 Contributing

We welcome contributions!

### How to Contribute

> [!IMPORTANT]
> **Development environment single source**: obtain it from `https://github.com/makevary/AIGLASS_DEV_ENV`.
>
> **Clone Directory Requirement**: If you need to compile code in `src` or `samples`, clone this project under the `AIGLASS_DEV_ENV` workspace root to satisfy build-time relative path dependencies.
>
> Correct directory structure example:
> ```
> /path/to/AIGLASS_DEV_ENV/
> ├── OpenSource-Ai-Glasses/    # This project
> ├── luckfox-pico/             # System SDK
> └── ...
> ```

1. 🍴 Fork the repository
2. 📥 Clone your fork locally (run under the `AIGLASS_DEV_ENV` workspace root), and initialize submodules
   ```bash
   cd /path/to/AIGLASS_DEV_ENV  # Navigate to the development workspace root
   git clone https://github.com/YOUR_USERNAME/OpenSource-Ai-Glasses.git
   cd OpenSource-Ai-Glasses
   git submodule update --init --recursive
   ```
3. 🌿 Create a feature branch (`git checkout -b feature/AmazingFeature`)
4. 💻 Commit your changes (`git commit -m 'Add some AmazingFeature'`)
5. 📤 Push to the branch (`git push origin feature/AmazingFeature`)
6. 🔃 Create a Pull Request

### Development Areas

- 🐛 **Bug Fixes**: Report and fix issues
- ✨ **New Features**: Propose and implement new capabilities
- 📚 **Documentation**: Improve guides and API docs
- 🧪 **Testing**: Add tests and improve coverage
- 🌐 **Internationalization**: Add language support

## 📄 License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- [Linux Foundation](https://www.linuxfoundation.org/) for Linux OS support
- [OpenCV](https://opencv.org/) for computer vision capabilities
- [BlueZ](https://www.bluez.org/) for Bluetooth protocol stack
- Community contributors and testers

## 📞 Contact

- **Project Maintainer**: [Iam5tilllearning](mailto:iam5tilllearning@foxmail.com)
- **Issues & Bugs**: [GitHub Issues](https://github.com/Iam5tilllearning/OpenSource-Ai-Glasses/issues)
- **Discussions**: [GitHub Discussions](https://github.com/Iam5tilllearning/OpenSource-Ai-Glasses/discussions)

## 🌟 Star History

[![Star History Chart](https://api.star-history.com/svg?repos=Iam5tilllearning/OpenSource-Ai-Glasses&type=Date)](https://star-history.com/#Iam5tilllearning/OpenSource-Ai-Glasses&Date)

---

<div align="center">

**⭐ If this project helped you, please give it a star!**

Made with ❤️ by the open-source community

</div>

---

**Last Updated**: 2026-05-18 | **Version**: v0.7.0
