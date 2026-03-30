# Camera Capture Example Guide

[中文版本](README.md) | English Version

## Introduction

`camera_capture_example` demonstrates how to capture one frame via Camera SDK and save it to a fixed path:

- `/tmp/test_capture`

The sample prints:

- capture status
- image metadata (resolution, format, size, sequence)
- final file location

## Build

```bash
cd ai_glass_sdk/examples/camera_capture_example
make
```

Output binary:

- `../build/camera_capture_example`

## Prerequisites

1. `ai-core` is running with camera mode enabled (`--camera-mode`)
2. Use SDK-matched `ai-core` build to avoid shared-memory layout mismatch

Recommended startup:

```bash
./ai-core --camera-mode
```

If you need directly viewable JPG output, add `--enable-jpeg`:

```bash
./ai-core --camera-mode --enable-jpeg
```

Note: `--disable-aicore-physical-interaction` is not required for capture. Use it only when you want to prevent AI-Core auto physical-button actions.

## Usage

```bash
../build/camera_capture_example
```

## Expected Output

### NV12 output example

```text
[SAMPLE] camera_capture_example started.
[SAMPLE] Target output path: /tmp/test_capture
[SAMPLE] Step 1/3: init camera client...
[SAMPLE] Step 2/3: capture one frame (timeout: 5000ms)...
[SAMPLE] Capture metadata: size=3110400, resolution=1920x1080, format=NV12, seq=1
[SAMPLE] Step 3/3: save bytes to /tmp/test_capture ...
[SAMPLE][OK] Capture file is ready.
[SAMPLE][OK] Read your file from: /tmp/test_capture
[SAMPLE][INFO] File payload format is: NV12 raw bytes
```

### JPEG output example (`--enable-jpeg` required)

```text
[SAMPLE] Capture metadata: size=3xxxxxx, resolution=1920x1080, format=JPEG, seq=1
[SAMPLE][INFO] File payload format is: JPEG bytes
```

## Verify Output File

```bash
ls -l /tmp/test_capture
wc -c /tmp/test_capture
```

If payload format is JPEG, you can rename it and open:

```bash
cp /tmp/test_capture /tmp/test_capture.jpg
```

If payload format is NV12 raw bytes, use `ffplay`:

```bash
ffplay -f rawvideo -pixel_format nv12 -video_size 1920x1080 /tmp/test_capture
```

## Exit Codes

| Code | Description |
|---|---|
| `0` | Success |
| `1` | Failure (init timeout/write failure etc.) |

## Troubleshooting

### 1) Why can’t I open `/tmp/test_capture` as image?

It may be NV12 raw bytes, not JPG.  
If you need JPG, start `ai-core` with `--enable-jpeg`.

### 2) `Invalid data size: ...`

Usually SDK and ai-core shared-memory size/layout do not match.  
Ensure both sides are updated together (current baseline uses 4MB image buffer).

## Related API Doc

- [Camera_Client_API.en.md](../../docs/Camera_Client_API.en.md)
