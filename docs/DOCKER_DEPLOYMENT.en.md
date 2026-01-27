# Docker Deployment Guide

English | [中文](DOCKER_DEPLOYMENT.md)

This document provides a deployment guide for Docker-based AI Smart Glasses development environment.

## 🎯 Environment Overview

The Docker development environment provides a complete toolchain supporting:
- 🔧 **Firmware Development**: Compile and customize system firmware (requires system SDK)
- 💻 **Application Development**: Develop user-level applications (using cross-compilation toolchain)
- 🛠️ **Cross Compilation**: Compile programs for RV1106B chip
- 🧪 **Testing & Debugging**: Complete development and debugging tools

> [!IMPORTANT]
> **Starting from v0.6.1, only Bare image is provided**. Bare image does not include system SDK, you need to mount the host's system SDK directory to the container. This approach is ideal for development using VS Code, Claude Code, Cursor, etc.

> **Terminology**:
> - **System SDK** (or System Development SDK): Used for firmware compilation and system customization, i.e., `rv1106b_rv1103b_linux_ipc_v1.0.0_20241016`
> - **Software Development SDK**: API libraries and tools for application development (provided separately to developers)

## 📋 System Requirements

- **Recommended System**: Ubuntu 22.04 LTS (Windows users can use WSL2+Ubuntu 22.04)
- **Docker Version**: 20.10 or higher
- **Disk Space**: At least 10GB available
- **Memory**: 8GB or more recommended

## 🐳 Method 1: Pull Image from Docker Hub (Recommended)

### 1. Pull Image

If you can access Docker Hub normally, pull the image directly:

```bash
docker pull aiglasses/rk-rv1106b-bare:v0.6.1
```

### 2. Download System SDK

Download system SDK package from the following link:

🔗 **Download Link**: [International (Google Drive)](https://drive.google.com/drive/folders/1mZnHhKv-sV4owZLXMEmUNvj3Vq2AGbXa?usp=drive_link) | [Mainland China (Quark)](https://pan.quark.cn/s/efe46ae65bfd)

File list:
- `aiglass_dev_env.tar.gz` - System SDK package
- `aiglasses_rv1106b_bare_docker.tar` - Bare image (optional, use when cannot access Docker Hub)

### 3. Run Container

After pulling, run the container (requires mounting system SDK directory):

```bash
docker run -it \
  -v /path/to/aiglass_dev_env:/opt/aiglass_dev_env \
  --name rk1106_dev \
  aiglasses/rk-rv1106b-bare:v0.6.1 bash -l
```

**Note**: Must use `bash -l` parameter, otherwise the environment will have issues.

## 💾 Method 2: Download Image from Cloud Storage

If you cannot access Docker Hub, you can download the Docker image tar file from cloud storage.

### 1. Download Image

Download Docker image files from the following link (both complete and bare images available):

🔗 **Download Link**: [International (Google Drive)](https://drive.google.com/drive/folders/1mZnHhKv-sV4owZLXMEmUNvj3Vq2AGbXa?usp=drive_link) | [Mainland China (Quark)](https://pan.quark.cn/s/efe46ae65bfd)

File list:
- `aiglasses_rv1106b_dev_docker.tar` - Complete image
- `aiglasses_rv1106b_bare_docker.tar` - Bare image (optional)

### 2. Import Image

After downloading, import the image:

**Import Complete Image**:
```bash
docker import aiglasses_rv1106b_dev_docker.tar aiglasses/rk-rv1106b:ready
```

**Import Bare Image** (optional):
```bash
docker import aiglasses_rv1106b_bare_docker.tar aiglasses/rk-rv1106b-bare:ready
```

**Note**: Replace the filename with your actual downloaded filename.

### 3. Run Container

After importing, run the container:

**Using Complete Image**:
```bash
docker run -it --name rk1106_dev aiglasses/rk-rv1106b:ready bash -l
```

**Using Bare Image** (requires mounting system SDK):
```bash
docker run -it \
  -v /path/to/system_sdk/rv1106b_rv1103b_linux_ipc_v1.0.0_20241016:/opt/aiglass_dev_env \
  --name rk1106_dev_bare \
  aiglasses/rk-rv1106b-bare:ready bash -l
```

## 🎨 Image Type Selection

The project provides two Docker images. Choose the appropriate version based on your development habits:

### Complete Image (Recommended for Beginners)

**Image Name**: `aiglasses/rk-rv1106b:ready`

**Features**:
- ✅ Includes complete system development SDK (for firmware compilation)
- ✅ Ready to use out of the box, no additional configuration needed
- ✅ Suitable for quick start and simple development

**Use Cases**:
- Quick experience and learning
- No need to frequently modify firmware code
- Container-based development meets requirements

**How to Run**:
```bash
docker run -it --name rk1106_dev aiglasses/rk-rv1106b:ready bash -l
```

**Enter Running Container**:

```bash
# Must also include -l parameter
docker exec -it rk1106_dev bash -l
```

### Bare Image (Recommended for IDE Users)

**Image Name**: `aiglasses/rk-rv1106b-bare:ready`

**Features**:
- 🎯 Does not include system development SDK, smaller image size
- 🔗 Requires mounting host system SDK directory
- 💻 System SDK code on host, can use any preferred IDE/editor
- 🔄 Host firmware code changes take effect immediately in container, no copying needed
- 🚀 Container only provides compilation environment, firmware development done on host

**Suitable for These Developers**:
- ✅ **Using Modern IDEs**: VS Code, Claude Code, Cursor, IntelliJ IDEA, etc.
- ✅ **Need AI-Assisted Programming**: Claude Code, GitHub Copilot, Cursor, etc.
- ✅ **Prefer Graphical Editors**: Not comfortable using vim/nano in containers
- ✅ **Need Intelligent Code Hints**: Auto-completion, syntax checking, refactoring, etc.
- ✅ **Team Collaboration**: Managing code versions with Git on host
- ✅ **Multi-Project Development**: Editing multiple projects simultaneously in different terminals/IDE windows

**Typical Workflow**:
1. Open system SDK directory on host with VS Code/Claude Code
2. Edit firmware code in IDE (enjoy code hints, AI assistance, etc.)
3. Enter Docker container in terminal to execute firmware compilation
4. View compilation results, continue modifications in IDE
5. Use host Git tools for version management

**Preparation**:

First prepare the system development SDK on the host. Taking Windows users as an example:
1. Download aiglass_dev_env.tar.gz to any path. Here we use D:\aiglass_dev_env.tar.gz as an example
2. Enter WSL2 from command line
```bash
wsl
```
3. Create directory ~/DockerMountPoint
```bash
mkdir ~/DockerMountPoint
```
4. Extract to ~/DockerMountPoint
```bash
tar -xzf /mnt/d/aiglass_dev_env.tar.gz -C ~/DockerMountPoint
```

**How to Run**:

```bash
# Enter WSL2 environment
wsl

# Run container and mount system SDK directory (example uses v0.6.0, replace with your version)
docker run -it -v ~/DockerMountPoint/aiglass_dev_env:/opt/aiglass_dev_env --name rk1106_dev_bare aiglasses/rk-rv1106b-bare:v0.6.0 /bin/bash -l

# Tip: If you get a "cannot connect to docker" error, go to Docker Desktop settings -> Resources -> WSL Integration, check "Enable integration with my default WSL distro", also check your Ubuntu in the list below, then Apply & Restart
```

**Important Notes**:
- 🔴 `-v` parameter mounts host system SDK directory to container's `/opt/aiglass_dev_env`
- 🔴 Mount path must be complete system SDK directory (containing `aiglass_dev_env`)
- 🔴 After container starts, firmware code modifications on host take effect immediately in container
- 🔴 Must use `/bin/bash -l` or `bash -l`, `-l` parameter cannot be omitted

**Compile Firmware**:

```bash
# In container
cd /opt/aiglass_dev_env
./build.sh
```

**Enter Running Bare Container**:

```bash
# Must also include -l parameter
docker exec -it rk1106_dev_bare bash -l
```

### Image Comparison

| Feature | Complete Image (ready) | Bare Image (bare:ready) |
|---------|----------------------|------------------------|
| Includes System SDK | ✅ Yes | ❌ No (requires mount) |
| Image Size | Larger (~5GB) | Smaller (~2GB) |
| Startup Speed | Fast | Fast |
| IDE Support | Container editing | Host IDE |
| Firmware Code Sync | Requires docker cp | Real-time sync |
| Suitable for Beginners | ✅ Yes | ❌ No |
| Suitable for IDE Users | ❌ No | ✅ Yes |
| Suitable for Team Collaboration | Average | ✅ Yes |

### Selection Recommendations

**Choose Complete Image if you**:
- 🌟 Just starting to learn smart glasses development
- 🌟 Comfortable using vim/nano in terminal
- 🌟 Only need simple testing or learning, not extensive coding
- 🌟 Want quick experience without configuring mount paths
- 🌟 Mainly doing application development, rarely modifying firmware

**Choose Bare Image if you**:
- 🔥 Use **VS Code**, **Claude Code**, **Cursor**, etc. IDEs on host for development
- 🔥 Need **AI programming assistants** (GitHub Copilot, Claude Code, Cursor, etc.)
- 🔥 Need IDE's **intelligent code hints, auto-completion, syntax checking** features
- 🔥 Prefer browsing and editing code in graphical editors rather than vim/nano
- 🔥 Need to frequently modify firmware code and see changes in real-time
- 🔥 Use Git GUI tools (VS Code Git, GitKraken, etc.) for version management
- 🔥 Team collaboration development, need code review and merge on host

## 🔧 Development Environment Usage

> **Important**: Regardless of which image you use, you must use the `-l` parameter (login shell) when entering the container, otherwise environment variables will not load correctly, causing compilation failures.

### Enter Running Container

If the container is already created and running in the background, use the following command to enter the development environment:

```bash
docker exec -it rk1106_dev bash -l
```

**Important Notes**:
- Must include `-l` parameter (login shell), otherwise environment variables and toolchain configuration won't load correctly
- Without `-l` parameter will cause compilation environment exceptions

### Develop Applications

The Docker environment includes a complete development toolchain for developing user-level applications.

#### Cross-Compilation Toolchain

Environment includes cross-compilation toolchain configured for RV1106B chip:

**Toolchain Location**: `/opt/new/rv1106b_rv1103b_linux_ipc_v1.0.0_20241016/tools/linux/toolchain/arm-rockchip831-linux-uclibcgnueabihf`

#### Compile Applications

```bash
# Enter container
docker exec -it rk1106_dev bash -l

# Go to working directory
cd /workspace  # or your project directory

# Compile C program using cross-compilation toolchain
arm-rockchip831-linux-uclibcgnueabihf-gcc -o myapp myapp.c

# Compile C++ program using cross-compilation toolchain
arm-rockchip831-linux-uclibcgnueabihf-g++ -o myapp myapp.cpp

# Example: with optimization options
arm-rockchip831-linux-uclibcgnueabihf-gcc -O2 -o myapp myapp.c

# Example: linking libraries
arm-rockchip831-linux-uclibcgnueabihf-gcc -o myapp myapp.c -lpthread -lm
```

**Tip**: Recommend using volume mount to map host code directory into container for easier development.

### Compile Firmware (Optional)

> **Note**: Device comes with pre-installed firmware. Only need to compile firmware when modifying system or upgrading.

After entering container, execute the following command to compile firmware:

```bash
./build.sh
```

Compilation process may take from several minutes to tens of minutes, depending on your hardware configuration.

### Get Compilation Artifacts

#### Firmware Files

After compilation completes, firmware files are located at:

```
/opt/aiglass_dev_env/output/image
```

You can copy firmware to host using:

```bash
# Execute on host
docker cp rk1106_dev:/opt/aiglass_dev_env/output/image ./firmware_output
```

#### Applications

**Step 1**: Copy compiled application to host:

```bash
# Execute on host
docker cp rk1106_dev:/workspace/myapp ./myapp
```

**Step 2**: Push program to device using ADB:

```bash
# Push program to device's specified directory
adb push myapp /userdata/

# Or push to other directories, like temp directory
adb push myapp /tmp/

# Or push to user directory (programs in this directory can execute directly anywhere without cd)
adb push myapp /usr/bin/
```

**Step 3**: Run program on device:

```bash
# Enter device command line
adb shell

# Navigate to program directory
cd /userdata/

# Add execute permission
chmod +x myapp

# Run program
./myapp
```

**Complete Example**:

```bash
# 1. Copy from container to host
docker cp rk1106_dev:/workspace/myapp ./myapp

# 2. Push to device
adb push myapp /userdata/myapp

# 3. Run on device
adb shell "cd /userdata && chmod +x myapp && ./myapp"
```

## 🛠️ Common Issues

### 1. Container Environment Abnormal After Startup

**Problem**: Cannot find compilation tools or environment variables incorrect after entering container

**Solution**: Ensure using `bash -l` parameter to enter container:

```bash
docker exec -it rk1106_dev bash -l
```

### 2. Image Import Failed

**Problem**: `docker import` command reports error

**Solutions**:
- Check if tar file downloaded completely (can compare file sizes)
- Ensure sufficient disk space
- Try executing command with `sudo` privileges

### 3. Container Name Conflict

**Problem**: Prompt that name already exists when running container

**Solutions**:
```bash
# Delete existing container
docker rm rk1106_dev

# Or use different name
docker run -it --name rk1106_dev_new aiglasses/rk-rv1106b:ready bash -l
```

### 4. Permission Issues

**Problem**: No permission to perform certain operations in container

**Solution**:
```bash
# Run container with root privileges
docker run -it --name rk1106_dev --user root aiglasses/rk-rv1106b:ready bash -l
```

### 5. Compilation Failed After Mounting System SDK with Bare Image

**Problem**: Cannot find SDK directory or compilation failed after mounting system SDK with bare image

**Solutions**:

**Check Mount Path**:
```bash
# Ensure system SDK directory structure is correct
# Host path should be: /path/to/system_sdk/rv1106b_rv1103b_linux_ipc_v1.0.0_20241016
# Mount to container at: /opt/aiglass_dev_env

# Enter container to check
docker exec -it rk1106_dev_bare bash -l
ls -la /opt/aiglass_dev_env
# Should see build.sh and other files
```

**Ensure Using -l Parameter**:
```bash
# Wrong: missing -l parameter
docker exec -it rk1106_dev_bare bash  # ❌ Wrong

# Correct: with -l parameter
docker exec -it rk1106_dev_bare bash -l  # ✅ Correct
```

**Recreate Container**:
```bash
# If mount path is wrong, delete container and recreate
docker stop rk1106_dev_bare
docker rm rk1106_dev_bare

# Rerun, ensure path is correct
docker run -it \
  -v $(pwd)/rv1106b_rv1103b_linux_ipc_v1.0.0_20241016:/opt/aiglass_dev_env \
  --name rk1106_dev_bare \
  aiglasses/rk-rv1106b-bare:ready bash -l
```

### 6. Windows/WSL2 Path Issues

**Problem**: Path format incorrect when using WSL2 on Windows

**Solution**:
```bash
# Windows path conversion
# Windows: D:\dev\sdk\rv1106b_rv1103b_linux_ipc_v1.0.0_20241016
# WSL2: /mnt/d/dev/sdk/rv1106b_rv1103b_linux_ipc_v1.0.0_20241016

# Run in WSL2
docker run -it \
  -v /mnt/d/dev/sdk/rv1106b_rv1103b_linux_ipc_v1.0.0_20241016:/opt/aiglass_dev_env \
  --name rk1106_dev_bare \
  aiglasses/rk-rv1106b-bare:ready bash -l
```

## 📚 More Resources

### Docker Hub Image Addresses
- **Bare Image**: https://hub.docker.com/r/aiglasses/rk-rv1106b-bare (tag: `v0.6.1`)

### Cloud Storage Download
- **Cloud Storage**: [International (Google Drive)](https://drive.google.com/drive/folders/1mZnHhKv-sV4owZLXMEmUNvj3Vq2AGbXa?usp=drive_link) | [Mainland China (Quark)](https://pan.quark.cn/s/efe46ae65bfd)
  - `aiglass_dev_env.tar.gz` - System SDK package
  - `aiglasses_rv1106b_bare_docker.tar` - Bare image

### Other Resources
- **Project Home**: https://github.com/Iam5stillLearning/OpenSource-Ai-Glasses
- **System Development SDK**: aiglass_dev_env.tar.gz (for firmware development, required by Bare image)
- **Software Development SDK**: Provided separately to application developers (for application development)

## 💡 Best Practices

### Data Persistence

To avoid data loss after container deletion, recommend using volume or bind mount:

#### When Using Complete Image

**Mount Firmware Output Directory**:

```bash
# Use bind mount to mount host directory to container
docker run -it --name rk1106_dev \
  -v /path/on/host:/opt/aiglass_dev_env/output \
  aiglasses/rk-rv1106b:ready bash -l
```

**Mount Application Development Working Directory** (recommended):

```bash
# Mount application development directory for easy code editing on host and compilation in container
docker run -it --name rk1106_dev \
  -v /path/to/your/project:/workspace \
  aiglasses/rk-rv1106b:ready bash -l
```

**Mount Multiple Directories Simultaneously**:

```bash
# Mount multiple directories to support both firmware and application development
docker run -it --name rk1106_dev \
  -v /path/to/your/project:/workspace \
  -v /path/to/firmware/output:/opt/aiglass_dev_env/output \
  aiglasses/rk-rv1106b:ready bash -l
```

#### When Using Bare Image (recommended for advanced users)

**Complete Development Environment Configuration**:

```bash
# Recommended configuration: mount both system SDK and application project directories
docker run -it --name rk1106_dev_bare \
  -v ~/aiglasses_system_sdk/rv1106b_rv1103b_linux_ipc_v1.0.0_20241016:/opt/aiglass_dev_env \
  -v ~/my_app_project:/workspace \
  aiglasses/rk-rv1106b-bare:ready bash -l
```

**Use Absolute Paths** (avoid path issues):

```bash
# Use $(pwd) to get current absolute path
docker run -it --name rk1106_dev_bare \
  -v $(pwd)/system_sdk/rv1106b_rv1103b_linux_ipc_v1.0.0_20241016:/opt/aiglass_dev_env \
  -v $(pwd)/app_project:/workspace \
  aiglasses/rk-rv1106b-bare:ready bash -l
```

**Development Workflow Recommendations** (Bare image, for firmware development):

1. **Start Container**: Run Docker container in background, mount system SDK directory
2. **Open IDE**: Open system SDK directory on host with VS Code/Claude Code/Cursor
3. **Edit Code**: Write and modify firmware code in IDE (enjoy AI assistance, code completion features)
4. **Compile & Test**: Enter container via `docker exec` to execute firmware compilation
5. **View Results**: Continue modifications in IDE based on compilation output
6. **Version Control**: Use IDE's integrated Git tools or host Git GUI

**Recommended IDE Configuration**:
- **VS Code** + Docker plugin + C/C++ plugin
- **Claude Code** - AI-assisted programming, code understanding and generation
- **Cursor** + AI features - Intelligent code completion and refactoring

### Run in Background

If you need the container to run continuously in the background:

**Complete Image**:
```bash
# Start container in background
docker run -d --name rk1106_dev aiglasses/rk-rv1106b:ready tail -f /dev/null

# Enter container when needed (must include -l)
docker exec -it rk1106_dev bash -l
```

**Bare Image**:
```bash
# Start container in background (remember to mount system SDK)
docker run -d \
  -v ~/aiglasses_system_sdk/rv1106b_rv1103b_linux_ipc_v1.0.0_20241016:/opt/aiglass_dev_env \
  --name rk1106_dev_bare \
  aiglasses/rk-rv1106b-bare:ready tail -f /dev/null

# Enter container when needed (must include -l)
docker exec -it rk1106_dev_bare bash -l
```

> **Tip**: Using background run mode, the container keeps running continuously, you can enter anytime via `docker exec`, very suitable for use with host IDEs.

## 🚀 Next Steps

After Docker environment setup, you can:

### 1. Start Application Development

Use Docker environment to develop user-level applications:

```bash
# Enter development environment
docker exec -it rk1106_dev bash -l

# Create project directory
mkdir -p /workspace/myproject
cd /workspace/myproject

# Write code (using vim/nano or mount host directory)
cat > hello.c << 'EOF'
#include <stdio.h>
int main() {
    printf("Hello from AI Glasses!\n");
    return 0;
}
EOF

# Compile using cross-compilation toolchain
arm-rockchip831-linux-uclibcgnueabihf-gcc -o hello hello.c

# Exit container
exit
```

**Deploy to Device and Run**:

```bash
# Copy from container to host
docker cp rk1106_dev:/workspace/myproject/hello ./hello

# Push to device
adb push hello /userdata/

# Run on device
adb shell "cd /userdata && chmod +x hello && ./hello"
```

See [Application Development Guide](APPLICATION_DEVELOPMENT.en.md) for more details.

### 2. Firmware Customization (Optional)

If you need to modify system firmware:

```bash
# Enter development environment
docker exec -it rk1106_dev bash -l

# Compile firmware
./build.sh

# Flash firmware to device
# See [Firmware Flashing Guide](FIRMWARE_FLASHING.en.md)
```

### 3. Learn Development Tools

Familiarize yourself with development tools in Docker environment:
- **Compilers**: gcc, g++, make, cmake
- **Debugging Tools**: gdb
- **Version Control**: git
- **Other Tools**: vim, nano editors

### 4. Reference Documentation

- [Application Development Guide](APPLICATION_DEVELOPMENT.en.md) - Application development getting started
- [Firmware Flashing Guide](FIRMWARE_FLASHING.en.md) - How to flash firmware to device

## 🔄 Image Updates

When new version of Docker image is released:

### Update Complete Image

```bash
# Stop and delete old container
docker stop rk1106_dev
docker rm rk1106_dev

# Delete old image
docker rmi aiglasses/rk-rv1106b:ready

# Pull new image
docker pull aiglasses/rk-rv1106b:ready

# Run new container
docker run -it --name rk1106_dev aiglasses/rk-rv1106b:ready bash -l
```

### Update Bare Image

```bash
# Stop and delete old container
docker stop rk1106_dev_bare
docker rm rk1106_dev_bare

# Delete old image
docker rmi aiglasses/rk-rv1106b-bare:ready

# Pull new image
docker pull aiglasses/rk-rv1106b-bare:ready

# Run new container (remember to mount system SDK directory)
docker run -it \
  -v /path/to/system_sdk/rv1106b_rv1103b_linux_ipc_v1.0.0_20241016:/opt/aiglass_dev_env \
  --name rk1106_dev_bare \
  aiglasses/rk-rv1106b-bare:ready bash -l
```

---

**Last Updated**: 2025-11-12 | **Version**: v1.0.0
