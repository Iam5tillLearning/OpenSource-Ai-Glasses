# Media Resource Control User Guide

[中文版本](README.md) | English Version

## Introduction

`media_resource_control` is an interactive command-line tool for controlling whether `ai-core` holds camera and audio resources through the AI Glass SDK.

Typical use cases:

- Release camera/audio before launching an external application
- Quickly inspect current media resource state during debugging

Note: the current RTSP live-video path uses the `ai-core` H.265 shared-main-stream architecture. Do not use this tool to run `cam_off/cam_on` or `aud_off/aud_on` when starting or stopping live video. Use the device-side `/oem/usr/bin/rkipc_device_enter.sh` and `/oem/usr/bin/rkipc_device_exit_to_aicore.sh` helpers instead; camera/audio remain owned by `ai-core` throughout that flow.

### Core Features
- ✅ Reuses SDK resource arbitration APIs directly
- ✅ Supports both camera and audio resources
- ✅ Provides real-time status query
- ✅ Suitable for manual debugging and scripted verification

## Build

```bash
cd ai_glass_sdk/examples/media_resource_control
make
```

Output: `../build/media_resource_control`

## Basic Usage

```bash
# Use default control socket
./../build/media_resource_control

# Use custom socket path
./../build/media_resource_control /tmp/ai-core_audio_ctrl
```

After startup, it enters interactive mode:

```text
media-resource>
```

## Available Commands

| Command | Description |
|------|------|
| `cam_on` | Resume camera back to `ai-core` |
| `cam_off` | Release camera to external application |
| `aud_on` | Resume audio back to `ai-core` |
| `aud_off` | Release audio to external application |
| `status` | Query current resource state |
| `help` | Show help information |
| `quit` / `exit` | Exit program |

## Typical Usage

### 1. Query Current State

```text
media-resource> status
State: camera=active(ai-core owned), audio=active(ai-core owned)
```

### 2. Release Resources to an External Application

```text
media-resource> cam_off
media-resource> aud_off
media-resource> status
```

Expected state:

```text
camera=suspended, audio=suspended
```

This flow is only for debugging cases where an external application must directly own camera/audio. It is not the current RTSP live-video path.

### 3. Switch Back to `ai-core`

After the external application releases the resources, run:

```text
media-resource> cam_on
media-resource> aud_on
media-resource> status
```

### 4. RTSP Live-Video Entry Points

The current live-video flow does not use this tool for resource switching. Use:

```bash
/oem/usr/bin/rkipc_device_enter.sh
/oem/usr/bin/rkipc_device_exit_to_aicore.sh
```

If you manually stop `rkipc`, use normal exit and avoid `kill -9`:

```bash
killall -TERM rkipc
```

Because `SIGKILL` skips RTSP, socket, and subscription cleanup and can leave incomplete subscription state or log evidence.

## Output Conventions

- Successful operations print `OK: ...`
- Failed operations print `ERROR: ...`
- Current state is printed automatically after each resource operation

## Requirements

- `ai-core` is already running
- Audio control socket is available, default path: `/tmp/ai-core_audio_ctrl`
- Server side implements:
  - `ai_audio_suspend_resources()`
  - `ai_audio_resume_resources()`
  - `ai_audio_get_resource_status()`

## FAQ

### 1. Initialization failed, cannot connect to ai-core audio control socket

Check whether `ai-core` is running:

```bash
ps | grep ai-core
ls -l /tmp/ai-core_audio_ctrl
```

### 2. `cam_on` takes several seconds

On the current RV1106 device, about `3-5s` recovery time for `cam_on` is expected.

### 3. `cam_on` fails after running `rkipc`

First confirm that the external holder has really released resources. If `rkipc` was started manually before, confirm that it exited normally with `SIGTERM` instead of `kill -9`.
