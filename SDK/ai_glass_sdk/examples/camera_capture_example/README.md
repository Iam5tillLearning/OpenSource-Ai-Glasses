# Camera Capture Example 使用说明

## 简介

`camera_capture_example` 演示如何通过 Camera SDK 抓取一帧图像，并写入固定路径：

- `/tmp/test_capture`

示例会输出：

- 抓拍状态
- 图像元数据（分辨率、格式、大小、序号）
- 最终文件位置

## 编译

```bash
cd ai_glass_sdk/examples/camera_capture_example
make
```

编译产物：

- `../build/camera_capture_example`

## 前置条件

1. `ai-core` 已启动并启用摄像头模式（`--camera-mode`）
2. 推荐使用与 SDK 匹配的最新 `ai-core`，避免共享内存结构不一致

推荐启动命令：

```bash
./ai-core --camera-mode
```

如果你希望得到可直接打开的 JPG，请启动时加上 `--enable-jpeg`：

```bash
./ai-core --camera-mode --enable-jpeg
```

说明：`--disable-aicore-physical-interaction` 不是拍照必要条件，仅在你希望避免物理按键自动动作时可选开启。

## 使用方法

```bash
../build/camera_capture_example
```

## 预期输出示例

### NV12 输出示例

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

### JPEG 输出示例（需 `--enable-jpeg`）

```text
[SAMPLE] Capture metadata: size=3xxxxxx, resolution=1920x1080, format=JPEG, seq=1
[SAMPLE][INFO] File payload format is: JPEG bytes
```

## 文件检查

```bash
ls -l /tmp/test_capture
wc -c /tmp/test_capture
```

如果是 JPEG 数据，可直接改后缀后打开：

```bash
cp /tmp/test_capture /tmp/test_capture.jpg
```

如果是 NV12 原始数据，可用 `ffplay` 查看：

```bash
ffplay -f rawvideo -pixel_format nv12 -video_size 1920x1080 /tmp/test_capture
```

## 返回值

| 返回值 | 说明 |
|---|---|
| `0` | 成功 |
| `1` | 失败（初始化失败、抓拍超时、写文件失败等） |

## 常见问题

### 1) 为什么我打不开 `/tmp/test_capture`？

因为该文件可能是 NV12 原始数据，不是 JPG。  
若需要 JPG，请让服务端使用 `--enable-jpeg`。

### 2) `Invalid data size: ...`

通常是 SDK 与 ai-core 的共享内存大小/结构不一致。  
请确保两端版本匹配（当前口径为 4MB 图像缓冲区）。

## 相关 API 文档

- [Camera_Client_API.md](../../docs/Camera_Client_API.md)
