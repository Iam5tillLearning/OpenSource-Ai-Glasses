# Media Resource Control User Guide

[中文版本](README.md) | English Version

## Introduction

`media_resource_control` is an interactive command-line tool for controlling whether `ai-core` holds camera and audio resources through the AI Glass SDK.

Typical use cases:

- Release camera/audio before launching an external application
- Return camera/audio back to `ai-core` after `rkipc` exits
- Quickly inspect current media resource state during debugging

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

### 2. Switch to `rkipc`

```text
media-resource> cam_off
media-resource> aud_off
media-resource> status
```

Expected state:

```text
camera=suspended, audio=suspended
```

### 3. Switch Back to `ai-core`

Make sure `rkipc` has exited cleanly first, then run:

```text
media-resource> cam_on
media-resource> aud_on
media-resource> status
```

### 4. Recommended `rkipc` Exit Method

Use normal exit when stopping `rkipc`:

```bash
killall -TERM rkipc
```

Avoid:

```bash
kill -9 <rkipc_pid>
```

Because `SIGKILL` skips user-space cleanup and may make later `cam_on` recovery unstable.

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

First confirm that `rkipc` exited normally with `SIGTERM` instead of `kill -9`.
