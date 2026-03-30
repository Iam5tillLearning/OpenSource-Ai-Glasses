# 应用开发指南

[English](APPLICATION_DEVELOPMENT.en.md) | 中文

本文档介绍如何在**主机环境**为 AI 智能眼镜开发用户级应用程序。

## 📋 开发环境准备

在开始开发之前，请确保：

- ✅ 开发环境通过 `https://github.com/makevary/AIGLASS_DEV_ENV` 获取
- ✅ 在 `AIGLASS_DEV_ENV` 根目录已执行 `./setup_build_env.sh`
- ✅ 设备已通过 USB 连接到开发主机
- ✅ ADB 工具已安装并可正常连接设备

## ✅ 环境验证

```bash
# 进入开发环境根目录
cd /path/to/AIGLASS_DEV_ENV

# 验证 ADB
adb devices

# 验证交叉编译工具链
arm-rockchip831-linux-uclibcgnueabihf-gcc --version
```

## 🛠️ 交叉编译工具链

- **架构**: ARM Cortex-A7
- **C 编译器**: `arm-rockchip831-linux-uclibcgnueabihf-gcc`
- **C++ 编译器**: `arm-rockchip831-linux-uclibcgnueabihf-g++`

常用命令：

```bash
arm-rockchip831-linux-uclibcgnueabihf-gcc --version
arm-rockchip831-linux-uclibcgnueabihf-g++ --version
arm-rockchip831-linux-uclibcgnueabihf-ld --version
```

## 🚀 快速入门：Hello World

### 1. 创建代码

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

### 2. 编译程序

```bash
arm-rockchip831-linux-uclibcgnueabihf-gcc -O2 -o hello hello.c
file hello
```

### 3. 部署到设备

```bash
adb push hello /userdata/apps/hello
adb shell "chmod +x /userdata/apps/hello && /userdata/apps/hello"
```

预期输出：

```text
Hello from AI Smart Glasses!
```

## 📝 开发工作流（主机）

1. 在主机编辑代码（VS Code/CLion/其他 IDE）
2. 在主机执行交叉编译
3. 通过 ADB 推送到设备
4. 在设备上运行并查看日志
5. 迭代修改

## 💡 编译建议

```bash
# 开启常用告警
arm-rockchip831-linux-uclibcgnueabihf-gcc -Wall -Wextra -O2 -o myapp myapp.c

# 带调试符号
arm-rockchip831-linux-uclibcgnueabihf-gcc -g -O0 -o myapp_dbg myapp.c

# 链接常见库
arm-rockchip831-linux-uclibcgnueabihf-gcc -O2 -o myapp myapp.c -lpthread -lm
```

## 🛠️ 常见问题

### 1. 找不到交叉编译器

先回到开发环境根目录，重新执行：

```bash
./setup_build_env.sh
```

### 2. ADB 连接失败

```bash
adb kill-server
adb start-server
adb devices
```

## 📚 相关文档

- [开发环境搭建指南](ENV_SETUP.md)
- [固件烧录指南](FIRMWARE_FLASHING.md)
