# RTSP Video Streaming Guide

[中文](RTSP_VIDEO_STREAMING.md) | English

This document explains how to use the glasses RTSP stream for mobile live preview, remote inspection, and cloud-side video AI recognition.

## Scope

The v0.7.0 reference firmware can start an `rkipc` streaming runtime that exposes the camera as RTSP. In the current architecture, `ai-core` still owns camera/audio and produces the H.265 video stream plus the audio mirror stream; `rkipc` only subscribes to those streams and serves RTSP. It does not take camera/audio ownership.

Reference URLs:

| Stream | URL | Reference format |
| --- | --- | --- |
| Main stream | `rtsp://<device_lan_ip>:554/live/0` | H.265, 1920 x 1080 |
| Sub stream | `rtsp://<device_lan_ip>:554/live/1` | H.265, 704 x 576 |

The exact codec, resolution, bitrate, and frame rate are firmware/config dependent. Always verify the stream with `ffprobe` before connecting a production AI pipeline.

## Prerequisites

- The glasses are connected to a 2.4 GHz Wi-Fi network.
- The client, gateway, or cloud ingestion host can reach the glasses LAN IP.
- The device has entered `rkipc` streaming mode.
- Your AI backend can decode H.265 RTSP, or you have a gateway that transcodes or extracts frames.

## Start Streaming

On v0.7.0 reference firmware, enter streaming mode from the OSAIG mobile client, device function flow, or the device-side helper if available:

```bash
adb shell /oem/usr/bin/rkipc_device_enter.sh
```

For USB-only development, forward the RTSP port:

```bash
adb forward tcp:8554 tcp:554
ffprobe -v error -rtsp_transport tcp \
  -show_entries stream=index,codec_type,codec_name,width,height \
  -of compact=p=0:nk=1 \
  rtsp://127.0.0.1:8554/live/0
```

For LAN access, replace the host with the glasses LAN IP:

```bash
ffprobe -v error -rtsp_transport tcp \
  -show_entries stream=index,codec_type,codec_name,width,height \
  -of compact=p=0:nk=1 \
  rtsp://<device_lan_ip>:554/live/0
```

To preview:

```bash
ffplay -rtsp_transport tcp rtsp://<device_lan_ip>:554/live/0
```

## Cloud Video AI Integration

Recommended topology:

```text
Glasses RTSP stream -> LAN/VPN/gateway -> frame extraction or transcoding -> cloud AI inference
```

Do not expose the device RTSP port directly to the public Internet. If the AI service is outside the LAN, use a controlled gateway, VPN, tunnel, or server-side puller with your own authentication and access control.

Typical frame extraction command for an AI pipeline:

```bash
ffmpeg -rtsp_transport tcp \
  -i rtsp://<device_lan_ip>:554/live/0 \
  -an -vf "fps=5" \
  frames/%06d.jpg
```

If your own hardware has a different camera mounting direction, add the required orientation transform in this step before inference.

If your backend cannot decode H.265, transcode at the gateway:

```bash
ffmpeg -rtsp_transport tcp \
  -i rtsp://<device_lan_ip>:554/live/0 \
  -an -c:v libx264 -preset veryfast -tune zerolatency \
  -f rtsp rtsp://<gateway_or_server>/osaig/live
```

## Stop Streaming

When RTSP is no longer needed, exit `rkipc` cleanly. After `rkipc` exits, the SDK subscriptions are disconnected and `ai-core` stops the H.265 producer after the no-subscriber grace period. Camera/audio remain owned by `ai-core` throughout the flow, so there is no extra `cam_on` / `aud_on` step:

```bash
adb shell /oem/usr/bin/rkipc_device_exit_to_aicore.sh
```

Avoid `kill -9` for `rkipc`, because it skips RTSP, socket, and subscription cleanup and can leave incomplete subscription state or log evidence.

## Troubleshooting

### No RTSP connection

Check that the device is in streaming mode and the port is reachable:

```bash
adb shell "ps | grep rkipc"
adb shell "netstat -lnt | grep ':554'"
```

### Stream works over USB but not from LAN

Confirm the device LAN IP, Wi-Fi network, client routing, and firewall rules. The client must be able to reach `rtsp://<device_lan_ip>:554/live/0`.

### AI results use the wrong direction

The reference stream is expected to use the normal camera direction. If you use custom hardware and the AI result is visually misaligned, adjust the input image direction in your cloud pipeline according to your actual camera mounting.

### Latency is too high

Use TCP RTSP first for reliability, then tune the player or gateway buffer. For AI recognition, extracting fewer frames per second is often better than increasing buffering.
