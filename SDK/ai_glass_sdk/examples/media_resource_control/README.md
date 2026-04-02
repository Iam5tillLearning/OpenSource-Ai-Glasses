# Media Resource Control 使用说明

## 简介

`media_resource_control` 是一个交互式命令行工具，用于通过 AI Glass SDK 控制 `ai-core` 对相机和音频资源的持有状态。

它适合这些场景：

- 在 launcher 或外部应用启动前，把相机/音频释放给外部应用
- 在 `rkipc` 退出后，把相机/音频交还给 `ai-core`
- 调试资源切换时，快速确认当前资源状态

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

### 2. 切换到 `rkipc`

```text
media-resource> cam_off
media-resource> aud_off
media-resource> status
```

预期状态：

```text
状态: camera=suspended(已释放), audio=suspended(已释放)
```

### 3. 从 `rkipc` 回切到 `ai-core`

先保证 `rkipc` 已正常退出，再执行：

```text
media-resource> cam_on
media-resource> aud_on
media-resource> status
```

### 4. 推荐与 `rkipc` 配合方式

退出 `rkipc` 时，推荐使用正常退出：

```bash
killall -TERM rkipc
```

不推荐：

```bash
kill -9 <rkipc_pid>
```

因为硬杀会跳过 `rkipc` 的用户态清理流程，可能导致后续 `cam_on` 恢复不稳定。

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

如果前序运行过 `rkipc`，优先确认它是否使用了正常退出，而不是 `kill -9`。
