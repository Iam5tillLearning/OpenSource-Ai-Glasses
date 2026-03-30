# Development Environment Setup Guide (Host / WSL2)

English | [中文](ENV_SETUP.md)

## 🎯 Current Workflow

- Recommended platforms: **Ubuntu/Debian host**, or **Ubuntu/Debian on Windows WSL2**.
- Single source for development environment: `https://github.com/makevary/AIGLASS_DEV_ENV`

## 📋 Requirements

- **OS**: Debian/Ubuntu (native or WSL2)
- **Permissions**: `sudo` (or root)
- **Network**: required for first-time dependency installation

## 🚀 Quick Start

### 1. Get the development environment

```bash
git clone https://github.com/makevary/AIGLASS_DEV_ENV.git
cd AIGLASS_DEV_ENV
```

### 2. Fix/install build dependencies

```bash
./setup_build_env.sh
```

This script will automatically:
- enable `i386` architecture
- update `apt` index
- install firmware build dependencies (`texinfo`, `gperf`, `multilib`, `flex`, `bison`, `cmake`, etc.)

### 3. Build firmware

```bash
# Choose one
# Option A: glasses with display capability (default)
./build.sh

# Option B: glasses without display capability (disable display startup)
./build.sh --without-display
```

Selection guide:
- Device with display capability: use `./build.sh`
- Device without display capability: use `./build.sh --without-display`

Parameter note:
- `--without-display`: disables display startup (`skip S60_app_launcher`)

### 4. Check output image

```bash
ls -lh output/image/update.img
```

## 🪟 Windows + WSL2 Notes

1. Enable WSL2 in Windows and install an Ubuntu/Debian distro.
2. In WSL2 terminal, follow the same quick-start steps:
   - clone `AIGLASS_DEV_ENV`
   - run `./setup_build_env.sh`
   - run `./build.sh`

## ✅ Verification

```bash
# 1) dependency fix succeeds (no ERROR)
./setup_build_env.sh

# 2) core tools are available
cmake --version
bison --version
flex --version

# 3) firmware build
./build.sh
./build.sh --without-display
```

## 🛠️ Troubleshooting

### 1. `apt-get` not found

`setup_build_env.sh` only supports Debian/Ubuntu systems (including Debian/Ubuntu in WSL2).

### 2. Permission denied

Use an account with `sudo` privilege, or run as root.

### 3. Missing build dependencies

Run again:

```bash
./setup_build_env.sh
```

If it still fails, capture `build.sh` output and troubleshoot with logs.

## 📚 Related Docs

- [Application Development Guide](APPLICATION_DEVELOPMENT.en.md)
- [Firmware Flashing Guide](FIRMWARE_FLASHING.en.md)
- [中文版本](ENV_SETUP.md)
