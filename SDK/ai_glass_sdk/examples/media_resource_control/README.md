# Media Resource Control 使用说明

## 简介

`media_resource_control` 是一个交互式命令行工具，用于通过 AI Glass SDK 控制 `ai-core` 对相机和音频资源的持有状态。

它适合这些场景：

- 在 launcher 或外部应用启动前，把相机/音频释放给外部应用
- 调试资源切换时，快速确认当前资源状态

说明：当前 RTSP 实时视频主路径已经切换为 `ai-core` H.265 共享主码流架构。启动或关闭实时视频流时，不再使用本工具执行 `cam_off/cam_on`、`aud_off/aud_on`；应使用设备侧 `/oem/usr/bin/rkipc_device_enter.sh` 和 `/oem/usr/bin/rkipc_device_exit_to_aicore.sh`，全过程 camera/audio 仍由 `ai-core` 持有。

### 核心特性
- ✅ 直接复用 SDK 资源仲裁 API
- ✅ 同时支持相机和音频两类资源
- ✅ 提供实时状态查询
- ✅ 适合手工调试和脚本验证

## 编译工具

```bash
cd ai_glass_sdk/examples/media_resource_control
make
```

编译产物：`../build/media_resource_control`

## 基本用法

```bash
# 使用默认控制 socket
./../build/media_resource_control

# 使用自定义 socket 路径
./../build/media_resource_control /tmp/ai-core_audio_ctrl
```

启动后进入交互模式：

```text
media-resource>
```

## 可用指令

| 指令 | 说明 |
|------|------|
| `cam_on` | 回收相机给 `ai-core` |
| `cam_off` | 释放相机给外部应用 |
| `aud_on` | 回收音频给 `ai-core` |
| `aud_off` | 释放音频给外部应用 |
| `status` | 查询当前资源状态 |
| `help` | 显示帮助信息 |
| `quit` / `exit` | 退出程序 |

## 典型用法

### 1. 查看当前状态

```text
media-resource> status
状态: camera=active(ai-core持有), audio=active(ai-core持有)
```

### 2. 释放资源给外部应用

```text
media-resource> cam_off
media-resource> aud_off
media-resource> status
```

预期状态：

```text
状态: camera=suspended(已释放), audio=suspended(已释放)
```

该流程仅适用于确实需要外部应用直接持有 camera/audio 的调试场景，不适用于当前 RTSP 实时视频主路径。

### 3. 回切到 `ai-core`

外部应用释放完毕后执行：

```text
media-resource> cam_on
media-resource> aud_on
media-resource> status
```

### 4. RTSP 实时视频流入口

当前实时视频流不通过本工具切换资源。请使用：

```bash
/oem/usr/bin/rkipc_device_enter.sh
/oem/usr/bin/rkipc_device_exit_to_aicore.sh
```

若手工停止 `rkipc`，推荐使用正常退出，不推荐 `kill -9`：

```bash
killall -TERM rkipc
```

因为硬杀会跳过 `rkipc` 的 RTSP、socket 和订阅断开清理流程，可能导致后续订阅状态或日志证据不完整。

## 返回与输出说明

- 成功操作会输出 `OK: ...`
- 失败操作会输出 `ERROR: ...`
- 每次资源切换后都会自动打印当前状态

## 依赖条件

- `ai-core` 已启动
- 音频控制 socket 可用，默认路径为 `/tmp/ai-core_audio_ctrl`
- 服务端已实现资源仲裁接口：
  - `ai_audio_suspend_resources()`
  - `ai_audio_resume_resources()`
  - `ai_audio_get_resource_status()`

## 常见问题

### 1. 初始化失败，无法连接 ai-core 音频控制通道

先检查 `ai-core` 是否在运行：

```bash
ps | grep ai-core
ls -l /tmp/ai-core_audio_ctrl
```

### 2. `cam_on` 恢复较慢

在当前 RV1106 设备上，`cam_on` 恢复约 `3-5s` 属于正常现象。

### 3. `cam_on` 失败

先确认外部 holder 是否已经真实释放资源；如果前序手工运行过 `rkipc`，优先确认它是否使用了正常退出，而不是 `kill -9`。
