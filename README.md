# 🥽 AI Smart Glasses

> [!IMPORTANT]
> # 🚧 More documentation is being added 🚧


**An open-source Linux-based smart glasses platform for medical, industrial, educational, and consumer applications**

[中文](README.zh.md) | **English**

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](https://github.com/ezxrdev/OpenSource-Ai-Glasses/actions)
[![License](https://img.shields.io/badge/license-Apache%202.0-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.6.1-green.svg)](https://github.com/Iam5tilllearning/OpenSource-Ai-Glasses/releases)


---
<img width="1024" height="747" alt="image" src="https://github.com/user-attachments/assets/af4de9f5-f502-4a74-8a2a-f5a762ff83b9" />

## 📋 Project Overview

This is a Linux-based open-source smart glasses project in early development stage (45% documentation completeness).

**Developer Community** (Scan to join WeChat group) | **Contact**: iam5stilllearning@foxmail.com

> 🛒 [**Get Dev Kit**](https://www.ebay.com/itm/136920980546) — Pre-installed firmware, ready to use

<img width="200" alt="WeChat Developer Group" src="docs/Images/weixin.png" />



> [!CAUTION]
> **Firmware Compatibility Notice**: v0.6.1 firmware is **only compatible with hardware produced after January 1, 2026**. If your device was purchased/produced before this date, please use v0.6.0 or earlier firmware versions.

## ✅ Current Version Features (v0.6.1)

- **Firmware Version: v0.6.1**
  - Compatible with new hardware produced after January 1, 2026. Enters network configuration mode on boot. After configuration, long press the left temple button to start AI conversation.
- **New SDK Available**: Developers can develop applications based on the SDK, avoiding the difficulty of developing Linux programs from scratch. No need to manage parallel hardware operations or deal with ISP/audio devices. Focus on upper-layer business logic.
    - **GPIO Event Subscription**: Supports multi-process concurrent listening for button events, providing asynchronous callback mechanism with latency < 13ms.
    - **Efficient Camera Access**: Zero-copy image transmission based on shared memory, supports JPEG and NV12 formats, supports multi-client concurrent access.
    - **Audio Playback Control**: Provides audio control interface based on Unix Socket, supports volume adjustment and sample rate configuration.
    - **Inter-Process Communication**: Encapsulates reliable communication mechanism based on Unix Domain Socket.
    - **System Logging**: Provides unified logging interface with precise millisecond timestamps.

<details>
<summary>📜 Version History summary</summary>

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

<img width="800" alt="Hardware Layout" src="resources/media/layout.png" />

## ✨ Key Features

- 🖥️ **Display**: 30°FOV 640×480 monocular display (optional)
- 📸 **Camera**: 1080P video recording
- 🔊 **Audio**: microphone + speaker system
- 📡 **Connectivity**: WiFi 802.11b/g/n, Bluetooth 5.3, USB 2.0
- ⚡ **Performance**: Single Cortex-A7 core, 8GB storage
- 🔋 **Battery**: 180mAh, 2hr music, 3hr display, 45min recording
- ⚖️ **Lightweight**: Only 43g
- 🧠 **Sensors**: Optional geomagnetic sensor, IMU
- 🐧 **OS**: Embedded Linux-based system

## 🚀 Quick Start

> **💡 Note**: Your device comes with pre-installed firmware and is ready to use. You only need to rebuild and flash firmware when modifying the system or upgrading.

### Using Docker Development Environment (Recommended)

We recommend using Docker as the development environment, which supports both firmware development and application development. Two image types available:

**Complete Image** (Recommended for beginners) - Includes full SDK, ready to use:
```bash
# Pull image from Docker Hub
docker pull aiglasses/rk-rv1106b:v0.6.0

# Run development container
docker run -it --name rk1106_dev aiglasses/rk-rv1106b:v0.6.0 bash -l

# Build firmware (only when modifying system)
./build.sh

# Copy firmware to host (only when flashing needed)
docker cp rk1106_dev:/opt/aiglass_dev_env/output/image/update.img ./update.img

# Develop applications
# The Docker environment includes a complete toolchain for application development
```

**Bare Image** (Recommended for developers using VS Code/Claude Code/Cursor) - Mount host SDK:
```bash
# Pull bare image
docker pull aiglasses/rk-rv1106b-bare:v0.6.0

# Run with system SDK directory mounted (System SDK download: [International](https://drive.google.com/drive/folders/1mZnHhKv-sV4owZLXMEmUNvj3Vq2AGbXa?usp=drive_link) | [Mainland China](https://pan.quark.cn/s/efe46ae65bfd))
docker run -it \
  -v /path/to/system_sdk/aiglass_dev_env:/opt/aiglass_dev_env \
  --name rk1106_dev_bare \
  aiglasses/rk-rv1106b-bare:v0.6.0 bash -l
```

> **Note**: Bare image is ideal for firmware development using host IDEs (VS Code, Claude Code, Cursor, etc.), enabling intelligent code completion, AI-assisted programming, and real-time firmware code synchronization.

> **Clarification**: The "system SDK" here refers to `aiglass_dev_env` for firmware development, not the application development SDK which will be provided separately.

> **Note**: If you need to re-enter an exited container, run `docker start rk1106_dev` to start it, then use `docker exec -it rk1106_dev bash -l` to enter.

See [Docker Deployment Guide](docs/DOCKER_DEPLOYMENT.en.md) for details.

**Firmware Flashing**: After compilation, please refer to [Firmware Flashing Guide](docs/FIRMWARE_FLASHING.en.md) to flash the firmware to your device.

**Application Development**: The Docker environment provides a complete development toolchain for building user-level applications. See [Application Development Guide](docs/APPLICATION_DEVELOPMENT.en.md) for details.


## 📖 Usage Guide

### 1. Network Configuration
- **Automatic**: The device automatically enters network configuration mode on boot.
- **Manual**: At any time, continuously short press the button 10 times to manually enter network configuration mode.

### 2. AI Conversation
- **Operation**: After configuration, long press the left temple button to speak, release to send, and wait for the AI response.


## 📦 SDK Development

This project provides a complete C/C++ SDK, allowing developers to easily access underlying hardware capabilities and build their own applications.

**SDK Core Features:**
*   **GPIO Event Subscription**: Low-latency access to button presses and other GPIO events
*   **Camera Access**: Zero-copy image data retrieval via shared memory
*   **Audio Playback Control**: control audio playback, supports TTS
*   **Inter-Process Communication**: Reliable communication based on Unix Domain Sockets

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
- [🐳 Docker Deployment Guide](docs/DOCKER_DEPLOYMENT.en.md) | [中文](docs/DOCKER_DEPLOYMENT.md)
- [💻 Application Development Guide](docs/APPLICATION_DEVELOPMENT.en.md) | [中文](docs/APPLICATION_DEVELOPMENT.md)
- [⚡ Firmware Flashing Guide](docs/FIRMWARE_FLASHING.en.md) | [中文](docs/FIRMWARE_FLASHING.md)

## 🛠️ Development

### Build from Source

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install build-essential git cmake

# Clone and build
git clone https://github.com/Iam5tillLearning/OpenSource-Ai-Glasses.git
cd OpenSource-Ai-Glasses
mkdir build && cd build
cmake ..
make -j4

# Flash to device
sudo make flash
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

1. 🍴 Fork the repository
2. 🌿 Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. 💻 Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. 📤 Push to the branch (`git push origin feature/AmazingFeature`)
5. 🔃 Create a Pull Request

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

**Last Updated**: 2026-01-08 | **Version**: v0.6.1
