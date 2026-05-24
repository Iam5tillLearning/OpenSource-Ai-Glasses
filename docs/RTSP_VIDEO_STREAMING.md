# RTSP 视频流指南

[English](RTSP_VIDEO_STREAMING.en.md) | 中文

本文档说明如何使用眼镜的 RTSP 视频流做手机端实时预览、远程巡检和云端视频 AI 识别。

## 适用范围

v0.7.0 参考固件可以启动 `rkipc` 串流运行态，并通过 RTSP 暴露摄像头画面。当前架构中，`ai-core` 仍然持有 camera/audio 并生产 H.265 视频流与音频镜像流；`rkipc` 只作为 RTSP 出口订阅这些流，不接管 camera/audio。

参考地址：

| 流 | 地址 | 参考格式 |
| --- | --- | --- |
| 主码流 | `rtsp://<device_lan_ip>:554/live/0` | H.265，1920 x 1080 |
| 子码流 | `rtsp://<device_lan_ip>:554/live/1` | H.265，704 x 576 |

实际编码、分辨率、码率和帧率可能随固件与配置变化。接入正式 AI 链路前，应先用 `ffprobe` 确认真实流信息。

## 前置条件

- 眼镜已连接 2.4 GHz Wi-Fi。
- 客户端、网关或云端接入主机能访问眼镜局域网 IP。
- 设备已进入 `rkipc` 串流模式。
- AI 后端能解码 H.265 RTSP，或者中间网关能转码/抽帧。

## 启动串流

在 v0.7.0 参考固件上，可以通过 OSAIG 手机端、设备函数链路，或设备侧 helper 脚本进入串流模式：

```bash
adb shell /oem/usr/bin/rkipc_device_enter.sh
```

USB 调试时，可以先把设备 RTSP 端口转发到主机：

```bash
adb forward tcp:8554 tcp:554
ffprobe -v error -rtsp_transport tcp \
  -show_entries stream=index,codec_type,codec_name,width,height \
  -of compact=p=0:nk=1 \
  rtsp://127.0.0.1:8554/live/0
```

局域网访问时，将主机替换为眼镜局域网 IP：

```bash
ffprobe -v error -rtsp_transport tcp \
  -show_entries stream=index,codec_type,codec_name,width,height \
  -of compact=p=0:nk=1 \
  rtsp://<device_lan_ip>:554/live/0
```

预览画面：

```bash
ffplay -rtsp_transport tcp rtsp://<device_lan_ip>:554/live/0
```

## 云端视频 AI 接入

推荐拓扑：

```text
眼镜 RTSP 流 -> 局域网/VPN/网关 -> 抽帧或转码 -> 云端 AI 推理
```

不要把设备 RTSP 端口直接裸露到公网。如果 AI 服务不在同一局域网内，应使用受控网关、VPN、隧道或服务端拉流器，并自行补齐认证、鉴权和访问控制。

典型抽帧命令：

```bash
ffmpeg -rtsp_transport tcp \
  -i rtsp://<device_lan_ip>:554/live/0 \
  -an -vf "fps=5" \
  frames/%06d.jpg
```

如果自研硬件的摄像头安装方向不同，可以在这一步按实际硬件方向增加必要的方向变换，再送入推理。

如果后端无法解码 H.265，可在网关侧转码：

```bash
ffmpeg -rtsp_transport tcp \
  -i rtsp://<device_lan_ip>:554/live/0 \
  -an -c:v libx264 -preset veryfast -tune zerolatency \
  -f rtsp rtsp://<gateway_or_server>/osaig/live
```

## 停止串流

不再需要 RTSP 时，应正常退出 `rkipc`。退出后 SDK 订阅断开，`ai-core` 会在无订阅者宽限期后停止 H.265 producer；camera/audio 在整个过程中始终由 `ai-core` 持有，不需要再执行 `cam_on` / `aud_on`：

```bash
adb shell /oem/usr/bin/rkipc_device_exit_to_aicore.sh
```

不要用 `kill -9` 结束 `rkipc`，因为这会跳过 RTSP、socket 和订阅断开的用户态清理流程，可能导致后续订阅状态或日志证据不完整。

## 常见问题

### 无法连接 RTSP

检查设备是否进入串流模式，以及 554 端口是否监听：

```bash
adb shell "ps | grep rkipc"
adb shell "netstat -lnt | grep ':554'"
```

### USB 转发能看，局域网不能看

确认设备局域网 IP、Wi-Fi 网络、客户端路由与防火墙规则。客户端必须能访问 `rtsp://<device_lan_ip>:554/live/0`。

### AI 识别结果方向不对

参考视频流预期按正常摄像头方向输出。如果使用自研硬件后识别结果与画面方向不一致，请按实际摄像头安装方向在云端推理前调整输入图像方向。

### 延迟过高

建议先使用 RTSP over TCP 保证稳定性，再调整播放器或网关缓冲。对于 AI 识别，降低抽帧帧率通常比扩大缓冲更有效。
