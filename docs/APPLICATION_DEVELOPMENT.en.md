# Application Development Guide

English | [中文](APPLICATION_DEVELOPMENT.md)

This document explains how to develop user-level applications for AI Smart Glasses in a **native host environment**.

## 📋 Environment Preparation

Before development, make sure:

- ✅ Development environment is obtained from `https://github.com/makevary/AIGLASS_DEV_ENV`
- ✅ `./setup_build_env.sh` has been run in the `AIGLASS_DEV_ENV` root
- ✅ Device is connected to host via USB
- ✅ ADB is installed and can communicate with the device

## ✅ Environment Verification

```bash
# Enter workspace root
cd /path/to/AIGLASS_DEV_ENV

# Verify ADB
adb devices

# Verify cross compiler
arm-rockchip831-linux-uclibcgnueabihf-gcc --version
```

## 🛠️ Cross-Compilation Toolchain

- **Architecture**: ARM Cortex-A7
- **C Compiler**: `arm-rockchip831-linux-uclibcgnueabihf-gcc`
- **C++ Compiler**: `arm-rockchip831-linux-uclibcgnueabihf-g++`

Common checks:

```bash
arm-rockchip831-linux-uclibcgnueabihf-gcc --version
arm-rockchip831-linux-uclibcgnueabihf-g++ --version
arm-rockchip831-linux-uclibcgnueabihf-ld --version
```

## 🚀 Quick Start: Hello World

### 1. Create source code

```bash
cd /path/to/AIGLASS_DEV_ENV
mkdir -p workspace/hello_world
cd workspace/hello_world

cat > hello.c << 'EOF_C'
#include <stdio.h>

int main(void) {
    printf("Hello from AI Smart Glasses!\\n");
    return 0;
}
EOF_C
```

### 2. Build

```bash
arm-rockchip831-linux-uclibcgnueabihf-gcc -O2 -o hello hello.c
file hello
```

### 3. Deploy to device

```bash
adb push hello /userdata/apps/hello
adb shell "chmod +x /userdata/apps/hello && /userdata/apps/hello"
```

Expected output:

```text
Hello from AI Smart Glasses!
```

## 📝 Host-side Workflow

1. Edit code on host (VS Code/CLion/any IDE)
2. Cross-compile on host
3. Push via ADB
4. Run and inspect logs on device
5. Iterate

## 📡 Cloud Video AI Through RTSP

For cloud-side video AI recognition, use the glasses RTSP stream instead of repeatedly pulling single camera snapshots.

Reference stream URLs:

```text
rtsp://<device_lan_ip>:554/live/0
rtsp://<device_lan_ip>:554/live/1
```

During USB development, you can forward the RTSP port to the host:

```bash
adb forward tcp:8554 tcp:554
ffprobe -v error -rtsp_transport tcp rtsp://127.0.0.1:8554/live/0
```

For LAN or cloud integration details, see [RTSP Video Streaming Guide](RTSP_VIDEO_STREAMING.en.md).

## 💡 Build Tips

```bash
# Warnings + optimization
arm-rockchip831-linux-uclibcgnueabihf-gcc -Wall -Wextra -O2 -o myapp myapp.c

# Debug build
arm-rockchip831-linux-uclibcgnueabihf-gcc -g -O0 -o myapp_dbg myapp.c

# Link common libs
arm-rockchip831-linux-uclibcgnueabihf-gcc -O2 -o myapp myapp.c -lpthread -lm
```

## 🛠️ Troubleshooting

### 1. Cross compiler not found

Re-run in workspace root:

```bash
./setup_build_env.sh
```

### 2. ADB connection failed

```bash
adb kill-server
adb start-server
adb devices
```

## 📚 Related Docs

- [Development Environment Setup Guide](ENV_SETUP.en.md)
- [Firmware Flashing Guide](FIRMWARE_FLASHING.en.md)
- [RTSP Video Streaming Guide](RTSP_VIDEO_STREAMING.en.md)
