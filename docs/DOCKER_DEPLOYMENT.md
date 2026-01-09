# Docker 部署指南

[English](DOCKER_DEPLOYMENT.en.md) | 中文

本文档提供了基于 Docker 的 AI 智能眼镜开发环境部署指南。

## 🎯 环境说明

Docker 开发环境提供了完整的工具链，支持：
- 🔧 **固件开发**: 编译和定制系统固件（需要系统开发SDK）
- 💻 **应用开发**: 开发用户级应用程序（使用交叉编译工具链）
- 🛠️ **交叉编译**: 为 RV1106B 芯片编译程序
- 🧪 **测试调试**: 完整的开发和调试工具

> [!IMPORTANT]
> **从 v0.6.1 版本开始，项目仅提供 Bare 镜像**。Bare镜像不包含系统SDK，需要将宿主机的系统SDK目录挂载到容器中。这种方式适合使用 VS Code、Claude Code、Cursor 等 IDE 进行开发。

> **术语说明**:
> - **系统SDK**（或系统开发SDK）：用于固件编译和系统定制，即 `rv1106b_rv1103b_linux_ipc_v1.0.0_20241016`
> - **软件开发SDK**：用于应用程序开发的API库和工具（单独提供给开发者）

## 📋 系统要求

- **推荐系统**: Ubuntu 22.04 LTS（Windows环境可以使用WSL2+Ubuntu 22.04）
- **Docker 版本**: 20.10 或更高
- **硬盘空间**: 至少 10GB 可用空间
- **内存**: 建议 8GB 或以上

## 🐳 方式一：从 Docker Hub 拉取镜像（推荐）

### 1. 拉取镜像

如果你可以正常访问 Docker Hub，使用以下命令直接拉取镜像：

```bash
docker pull aiglasses/rk-rv1106b-bare:v0.6.1
```

### 2. 下载系统SDK

从以下链接下载系统SDK压缩包：

🔗 **下载地址**: [国内用户(夸克)](https://pan.quark.cn/s/efe46ae65bfd) | [海外用户(Google Drive)](https://drive.google.com/drive/folders/1mZnHhKv-sV4owZLXMEmUNvj3Vq2AGbXa?usp=drive_link)

文件列表：
- `aiglass_dev_env.tar.gz` - 系统SDK压缩包
- `aiglasses_rv1106b_bare_docker.tar` - Bare镜像（可选，无法访问Docker Hub时使用）

### 3. 运行容器

拉取完成后，运行容器（需挂载系统SDK目录）：

```bash
docker run -it \
  -v /path/to/aiglass_dev_env:/opt/aiglass_dev_env \
  --name rk1106_dev \
  aiglasses/rk-rv1106b-bare:v0.6.1 bash -l
```

**注意**: 必须使用 `bash -l` 参数，否则运行环境会有问题。

## 💾 方式二：从网盘下载镜像文件

如果无法访问 Docker Hub，可以从网盘下载 Docker 镜像 tar 文件。

### 1. 下载镜像和系统SDK

从以下链接下载文件：

🔗 **下载地址**: [国内用户(夸克)](https://pan.quark.cn/s/efe46ae65bfd) | [海外用户(Google Drive)](https://drive.google.com/drive/folders/1mZnHhKv-sV4owZLXMEmUNvj3Vq2AGbXa?usp=drive_link)

文件列表：
- `aiglass_dev_env.tar.gz` - 系统SDK压缩包
- `aiglasses_rv1106b_bare_docker.tar` - Bare镜像

### 2. 加载镜像

下载完成后，使用以下命令加载镜像：

```bash
docker load -i aiglasses_rv1106b_bare_docker.tar
```

**注意**: 请将文件名替换为你实际下载的文件名。

### 3. 运行容器

加载完成后，运行容器（需挂载系统SDK目录）：

```bash
docker run -it \
  -v /path/to/aiglass_dev_env:/opt/aiglass_dev_env \
  --name rk1106_dev \
  aiglasses/rk-rv1106b-bare:v0.6.1 bash -l
```

## 🎨 Bare 镜像使用指南

从 v0.6.1 版本开始，项目仅提供 Bare 镜像。

**镜像名称**: `aiglasses/rk-rv1106b-bare:v0.6.1`

**特点**:
- 🎯 不包含系统开发SDK，镜像体积小（~2GB）
- 🔗 需要挂载宿主机的系统SDK目录
- 💻 系统SDK代码在宿主机，可使用任何喜欢的 IDE/编辑器
- 🔄 宿主机修改固件代码后，容器内立即生效，无需拷贝
- 🚀 容器只负责提供编译环境，固件开发在宿主机完成

**适合这些开发者**:
- ✅ **使用现代 IDE 进行开发**：VS Code、Claude Code、Cursor、IntelliJ IDEA 等
- ✅ **需要 AI 辅助编程**：Claude Code、GitHub Copilot、Cursor 等 AI 编程工具
- ✅ **需要代码智能提示**：自动补全、语法检查、重构等 IDE 功能
- ✅ **团队协作开发**：使用 Git 在宿主机管理代码版本

**典型工作流程**:
1. 在宿主机用 VS Code/Claude Code 打开系统SDK目录
2. 在 IDE 中编辑固件代码（享受代码提示、AI 辅助等功能）
3. 在终端进入 Docker 容器执行固件编译
4. 查看编译结果，继续在 IDE 中修改
5. 使用宿主机的 Git 工具管理版本

**准备工作**:

首先在宿主机上准备系统开发SDK，以Windows用户为例：
1. 下载 aiglass_dev_env.tar.gz 到任意路径，这里以 D:\aiglass_dev_env.tar.gz 为例
2. 命令行进入wsl2
```bash
wsl
```
3. 创建目录 ~/DockerMountPoint
```bash
mkdir ~/DockerMountPoint
```
4. 解压到 ~/DockerMountPoint
```bash
tar -xzf /mnt/d/aiglass_dev_env.tar.gz -C ~/DockerMountPoint
```

**运行方式**:

```bash
# 进入wsl2环境
wsl

# 运行容器并挂载系统SDK目录
docker run -it -v ~/DockerMountPoint/aiglass_dev_env:/opt/aiglass_dev_env --name rk1106_dev aiglasses/rk-rv1106b-bare:v0.6.1 /bin/bash -l

# 提示：如果提示无法连接docker，需要先在docker desktop的设置 Resources->WSL Integration 中勾选 "Enable integration with my default WSL distro"，下面列表里把你的 Ubuntu 也勾上，Apply & Restart
```

**重要说明**:
- 🔴 `-v` 参数将宿主机的系统SDK目录挂载到容器的 `/opt/aiglass_dev_env`
- 🔴 挂载路径必须是完整的系统SDK目录（包含 `aiglass_dev_env`）
- 🔴 容器启动后，在宿主机修改固件代码，容器内立即生效
- 🔴 必须使用 `/bin/bash -l` 或 `bash -l`，`-l` 参数不能省略

**编译固件**:

```bash
# 在容器内
cd /opt/aiglass_dev_env
./build.sh
```

**进入已运行的容器**:

```bash
# 同样必须带 -l 参数
docker exec -it rk1106_dev bash -l
```

## 🔧 开发环境使用

> **重要提示**: 不论使用哪种镜像，进入容器时都必须使用 `-l` 参数（login shell），否则环境变量无法正确加载，将导致编译失败。

### 进入容器

**如果容器正在运行**：

```bash
docker exec -it rk1106_dev bash -l
```

**如果容器已停止**（例如重启电脑后）：

```bash
docker start rk1106_dev
docker exec -it rk1106_dev bash -l
```

**重要提示**:
- 必须带 `-l` 参数（login shell），否则环境变量和工具链配置将无法正确加载
- 不带 `-l` 参数会导致编译环境异常

### 开发应用程序

Docker 环境包含完整的开发工具链，可用于开发用户级应用程序。

#### 交叉编译工具链

环境中已配置针对 RV1106B 芯片的交叉编译工具链：

**工具链位置**: `/opt/new/rv1106b_rv1103b_linux_ipc_v1.0.0_20241016/tools/linux/toolchain/arm-rockchip831-linux-uclibcgnueabihf`

#### 编译应用程序

```bash
# 进入容器
docker exec -it rk1106_dev bash -l

# 进入工作目录
cd /workspace  # 或你的项目目录

# 使用交叉编译工具链编译 C 程序
arm-rockchip831-linux-uclibcgnueabihf-gcc -o myapp myapp.c

# 使用交叉编译工具链编译 C++ 程序
arm-rockchip831-linux-uclibcgnueabihf-g++ -o myapp myapp.cpp

# 编译示例：带优化选项
arm-rockchip831-linux-uclibcgnueabihf-gcc -O2 -o myapp myapp.c

# 编译示例：链接库
arm-rockchip831-linux-uclibcgnueabihf-gcc -o myapp myapp.c -lpthread -lm
```

**提示**: 建议使用数据卷挂载将宿主机代码目录映射到容器中，方便开发。

### 编译固件（可选）

> **注意**: 设备已预装固件。只有在需要修改系统或升级时才需要编译固件。

进入容器后，执行以下命令编译固件：

```bash
./build.sh
```

编译过程可能需要几分钟到几十分钟不等，具体取决于你的硬件配置。

### 获取编译产物

#### 固件文件

编译完成后，固件文件位于：

```
/opt/aiglass_dev_env/output/image
```

你可以通过以下方式将固件复制到宿主机：

```bash
# 在宿主机上执行
docker cp rk1106_dev:/opt/aiglass_dev_env/output/image ./firmware_output
```

#### 应用程序

**步骤1**: 将编译好的应用程序复制到宿主机：

```bash
# 在宿主机上执行
docker cp rk1106_dev:/workspace/myapp ./myapp
```

**步骤2**: 使用 ADB 将程序推送到设备：

```bash
# 将程序推送到设备的指定目录
adb push myapp /userdata/

# 或推送到其他目录，如临时目录
adb push myapp /tmp/

# 或推送到用户目录（推到此目录下的程序可以在任意目录下直接执行，不需要cd到程序所在目录）
adb push myapp /usr/bin/
```

**步骤3**: 在设备上运行程序：

```bash
# 进入设备命令行
adb shell

# 切换到程序所在目录
cd /userdata/

# 添加执行权限
chmod +x myapp

# 运行程序
./myapp
```

**完整示例**：

```bash
# 1. 从容器复制到宿主机
docker cp rk1106_dev:/workspace/myapp ./myapp

# 2. 推送到设备
adb push myapp /userdata/myapp

# 3. 在设备上运行
adb shell "cd /userdata && chmod +x myapp && ./myapp"
```

## 🛠️ 常见问题

### 1. 容器启动后环境异常

**问题**: 进入容器后找不到编译工具或环境变量不正确

**解决方案**: 确保使用 `bash -l` 参数进入容器：

```bash
docker exec -it rk1106_dev bash -l
```

### 2. 镜像加载失败

**问题**: `docker load` 命令报错

**解决方案**:
- 检查 tar 文件是否下载完整（可以对比文件大小）
- 确保有足够的磁盘空间
- 尝试使用 `sudo` 权限执行命令

### 3. 容器名称冲突

**问题**: 运行容器时提示名称已存在

**解决方案**:
```bash
# 删除已存在的容器
docker rm rk1106_dev

# 或者使用不同的名称
docker run -it --name rk1106_dev_new aiglasses/rk-rv1106b:ready bash -l
```

### 4. 权限问题

**问题**: 在容器中没有权限执行某些操作

**解决方案**:
```bash
# 以 root 权限运行容器
docker run -it --name rk1106_dev --user root aiglasses/rk-rv1106b:ready bash -l
```

### 5. Bare镜像挂载系统SDK后编译失败

**问题**: 使用bare镜像挂载系统SDK后，无法找到SDK目录或编译失败

**解决方案**:

**检查挂载路径**:
```bash
# 确保系统SDK目录结构正确
# 宿主机路径应该是：/path/to/system_sdk/rv1106b_rv1103b_linux_ipc_v1.0.0_20241016
# 容器内挂载到：/opt/aiglass_dev_env

# 进入容器检查
docker exec -it rk1106_dev_bare bash -l
ls -la /opt/aiglass_dev_env
# 应该能看到 build.sh 等文件
```

**确保使用 -l 参数**:
```bash
# 错误：缺少 -l 参数
docker exec -it rk1106_dev_bare bash  # ❌ 错误

# 正确：带 -l 参数
docker exec -it rk1106_dev_bare bash -l  # ✅ 正确
```

**重新创建容器**:
```bash
# 如果挂载路径错误，删除容器重新创建
docker stop rk1106_dev_bare
docker rm rk1106_dev_bare

# 重新运行，确保路径正确
docker run -it \
  -v $(pwd)/rv1106b_rv1103b_linux_ipc_v1.0.0_20241016:/opt/aiglass_dev_env \
  --name rk1106_dev_bare \
  aiglasses/rk-rv1106b-bare:ready bash -l
```

### 6. Windows/WSL2 路径问题

**问题**: 在 Windows 使用 WSL2 时，路径格式不正确

**解决方案**:
```bash
# Windows路径转换
# Windows: D:\dev\sdk\rv1106b_rv1103b_linux_ipc_v1.0.0_20241016
# WSL2: /mnt/d/dev/sdk/rv1106b_rv1103b_linux_ipc_v1.0.0_20241016

# 在WSL2中运行
docker run -it \
  -v /mnt/d/dev/sdk/rv1106b_rv1103b_linux_ipc_v1.0.0_20241016:/opt/aiglass_dev_env \
  --name rk1106_dev_bare \
  aiglasses/rk-rv1106b-bare:ready bash -l
```

## 📚 更多资源

### Docker Hub 镜像地址
- **Bare镜像**: https://hub.docker.com/r/aiglasses/rk-rv1106b-bare (tag: `v0.6.1`)

### 网盘下载地址
- **网盘下载**: [国内用户(夸克)](https://pan.quark.cn/s/efe46ae65bfd) | [海外用户(Google Drive)](https://drive.google.com/drive/folders/1mZnHhKv-sV4owZLXMEmUNvj3Vq2AGbXa?usp=drive_link)
  - `aiglass_dev_env.tar.gz` - 系统SDK压缩包
  - `aiglasses_rv1106b_bare_docker.tar` - Bare镜像

### 其他资源
- **项目主页**: https://github.com/Iam5stillLearning/OpenSource-Ai-Glasses
- **系统开发SDK**: aiglass_dev_env.tar.gz（用于固件开发，Bare镜像需要）
- **软件开发SDK**: 单独提供给应用开发者（用于应用程序开发）

## 💡 最佳实践

### 数据持久化

为了避免容器删除后数据丢失，建议使用 volume 或 bind mount：

#### 使用完整镜像时

**挂载固件输出目录**:

```bash
# 使用 bind mount 将宿主机目录挂载到容器
docker run -it --name rk1106_dev \
  -v /path/on/host:/opt/aiglass_dev_env/output \
  aiglasses/rk-rv1106b:ready bash -l
```

**挂载应用开发工作目录**（推荐）:

```bash
# 挂载应用开发目录，方便在宿主机编辑代码，在容器中编译
docker run -it --name rk1106_dev \
  -v /path/to/your/project:/workspace \
  aiglasses/rk-rv1106b:ready bash -l
```

**同时挂载多个目录**:

```bash
# 挂载多个目录以同时支持固件和应用开发
docker run -it --name rk1106_dev \
  -v /path/to/your/project:/workspace \
  -v /path/to/firmware/output:/opt/aiglass_dev_env/output \
  aiglasses/rk-rv1106b:ready bash -l
```

#### 使用Bare镜像时（推荐高级用户）

**完整开发环境配置**:

```bash
# 推荐配置：同时挂载系统SDK和应用项目目录
docker run -it --name rk1106_dev_bare \
  -v ~/aiglasses_system_sdk/rv1106b_rv1103b_linux_ipc_v1.0.0_20241016:/opt/aiglass_dev_env \
  -v ~/my_app_project:/workspace \
  aiglasses/rk-rv1106b-bare:ready bash -l
```

**使用绝对路径**（避免路径问题）:

```bash
# 使用 $(pwd) 获取当前绝对路径
docker run -it --name rk1106_dev_bare \
  -v $(pwd)/system_sdk/rv1106b_rv1103b_linux_ipc_v1.0.0_20241016:/opt/aiglass_dev_env \
  -v $(pwd)/app_project:/workspace \
  aiglasses/rk-rv1106b-bare:ready bash -l
```

**开发工作流建议**（Bare镜像，用于固件开发）:

1. **启动容器**：在后台运行 Docker 容器，挂载系统SDK目录
2. **打开 IDE**：在宿主机用 VS Code/Claude Code/Cursor 打开系统SDK目录
3. **编辑代码**：在 IDE 中编写和修改固件代码（享受 AI 辅助、代码补全等功能）
4. **编译测试**：通过 `docker exec` 进入容器执行固件编译
5. **查看结果**：根据编译输出在 IDE 中继续修改
6. **版本控制**：使用 IDE 集成的 Git 工具或宿主机的 Git 图形界面

**推荐 IDE 配置**:
- **VS Code** + Docker 插件 + C/C++ 插件
- **Claude Code** - AI 辅助编程，代码理解和生成
- **Cursor** + AI 功能 - 智能代码补全和重构

### 后台运行

如果需要容器在后台持续运行：

**完整镜像**:
```bash
# 启动容器并在后台运行
docker run -d --name rk1106_dev aiglasses/rk-rv1106b:ready tail -f /dev/null

# 需要时进入容器（必须带 -l）
docker exec -it rk1106_dev bash -l
```

**Bare镜像**:
```bash
# 启动容器并在后台运行（记得挂载系统SDK）
docker run -d \
  -v ~/aiglasses_system_sdk/rv1106b_rv1103b_linux_ipc_v1.0.0_20241016:/opt/aiglass_dev_env \
  --name rk1106_dev_bare \
  aiglasses/rk-rv1106b-bare:ready tail -f /dev/null

# 需要时进入容器（必须带 -l）
docker exec -it rk1106_dev_bare bash -l
```

> **提示**: 使用后台运行方式，容器会持续保持运行状态，你可以随时通过 `docker exec` 进入，非常适合配合宿主机IDE使用。

## 🚀 下一步

Docker 环境搭建完成后，你可以：

### 1. 开始应用开发

使用 Docker 环境开发用户级应用程序：

```bash
# 进入开发环境
docker exec -it rk1106_dev bash -l

# 创建项目目录
mkdir -p /workspace/myproject
cd /workspace/myproject

# 编写代码（使用 vim/nano 或挂载宿主机目录）
cat > hello.c << 'EOF'
#include <stdio.h>
int main() {
    printf("Hello from AI Glasses!\n");
    return 0;
}
EOF

# 使用交叉编译工具链编译
arm-rockchip831-linux-uclibcgnueabihf-gcc -o hello hello.c

# 退出容器
exit
```

**部署到设备并运行**：

```bash
# 从容器复制到宿主机
docker cp rk1106_dev:/workspace/myproject/hello ./hello

# 推送到设备
adb push hello /userdata/

# 在设备上运行
adb shell "cd /userdata && chmod +x hello && ./hello"
```

参考 [应用开发指南](APPLICATION_DEVELOPMENT.md) 了解更多详情。

### 2. 固件定制（可选）

如果需要修改系统固件：

```bash
# 进入开发环境
docker exec -it rk1106_dev bash -l

# 编译固件
./build.sh

# 烧录固件到设备
# 参考 [固件烧录指南](FIRMWARE_FLASHING.md)
```

### 3. 学习开发工具

熟悉 Docker 环境中的开发工具：
- **编译器**: gcc, g++, make, cmake
- **调试工具**: gdb
- **版本控制**: git
- **其他工具**: vim, nano 等编辑器

### 4. 相关文档

- [应用开发指南](APPLICATION_DEVELOPMENT.md) - 应用程序开发入门
- [固件烧录指南](FIRMWARE_FLASHING.md) - 如何将固件烧录到设备

## 🔄 镜像更新

当有新版本的 Docker 镜像发布时：

### 更新完整镜像

```bash
# 停止并删除旧容器
docker stop rk1106_dev
docker rm rk1106_dev

# 删除旧镜像
docker rmi aiglasses/rk-rv1106b:ready

# 拉取新镜像
docker pull aiglasses/rk-rv1106b:ready

# 运行新容器
docker run -it --name rk1106_dev aiglasses/rk-rv1106b:ready bash -l
```

### 更新Bare镜像

```bash
# 停止并删除旧容器
docker stop rk1106_dev_bare
docker rm rk1106_dev_bare

# 删除旧镜像
docker rmi aiglasses/rk-rv1106b-bare:ready

# 拉取新镜像
docker pull aiglasses/rk-rv1106b-bare:ready

# 运行新容器（记得挂载系统SDK目录）
docker run -it \
  -v /path/to/system_sdk/rv1106b_rv1103b_linux_ipc_v1.0.0_20241016:/opt/aiglass_dev_env \
  --name rk1106_dev_bare \
  aiglasses/rk-rv1106b-bare:ready bash -l
```

---

**最后更新**: 2025-11-11 | **版本**: v1.0.0
