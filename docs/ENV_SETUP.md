# 开发环境搭建指南（主机 / WSL2）

[English](ENV_SETUP.en.md) | 中文

## 🎯 开发口径

- 推荐在 **Ubuntu/Debian 主机**，或 **Windows + WSL2（Ubuntu/Debian）** 上直接开发与编译。
- 开发环境唯一获取方式：`https://github.com/makevary/AIGLASS_DEV_ENV`

## 📋 系统要求

- **操作系统**：Debian/Ubuntu（原生或 WSL2）
- **权限要求**：可执行 `sudo`（或 root）
- **网络要求**：首次安装依赖需要访问软件源

## 🚀 快速开始

### 1. 获取开发环境

```bash
git clone https://github.com/makevary/AIGLASS_DEV_ENV.git
cd AIGLASS_DEV_ENV
```

### 2. 修复/安装编译依赖

```bash
./setup_build_env.sh
```

该脚本会自动：
- 启用 `i386` 架构
- 更新 `apt` 索引
- 安装固件编译所需依赖（如 `texinfo`、`gperf`、`multilib`、`flex`、`bison`、`cmake` 等）

### 3. 编译固件

```bash
# 二选一
# 方式 A：你的眼镜带显示能力（默认）
./build.sh

# 方式 B：你的眼镜不带显示能力（关闭显示启动）
./build.sh --without-display
```

选择建议：
- 设备带显示能力：使用 `./build.sh`
- 设备不带显示能力：使用 `./build.sh --without-display`

参数说明：
- `--without-display`：禁用显示启动（`skip S60_app_launcher`）

### 4. 查看产物

```bash
ls -lh output/image/update.img
```

## 🪟 Windows + WSL2 使用说明

1. 在 Windows 启用 WSL2，并安装 Ubuntu/Debian 发行版。
2. 进入 WSL2 终端后，按“快速开始”步骤执行：
   - 克隆 `AIGLASS_DEV_ENV`
   - 运行 `./setup_build_env.sh`
   - 运行 `./build.sh`

## ✅ 验证检查

执行以下命令可快速确认环境可用：

```bash
# 1) 脚本执行成功（无 ERROR）
./setup_build_env.sh

# 2) 工具可用
cmake --version
bison --version
flex --version

# 3) 固件编译
./build.sh
./build.sh --without-display
```

## 🛠️ 常见问题

### 1. 提示没有 `apt-get`

`setup_build_env.sh` 仅支持 Debian/Ubuntu 系（含 WSL2 的 Ubuntu/Debian）。

### 2. 提示权限不足

请使用具备 `sudo` 权限的账号，或直接使用 root 用户执行。

### 3. 编译工具链缺依赖

优先重新执行一次：

```bash
./setup_build_env.sh
```

如仍异常，请记录报错并附上 `build.sh` 日志再排查。

## 📚 相关文档

- [应用开发指南](APPLICATION_DEVELOPMENT.md)
- [固件烧录指南](FIRMWARE_FLASHING.md)
- [English 版本](ENV_SETUP.en.md)
