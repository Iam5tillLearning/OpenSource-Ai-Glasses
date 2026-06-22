# BLE + SPP 拍照回传综合 Demo

本 demo 演示 SDK 侧同时使用 BLE 与经典蓝牙 SPP：

1. 客户端先建立 SPP/RFCOMM 连接，再建立 BLE GATT 连接。
2. 客户端通过 BLE 发送 `combo.camera.takephoto`。
3. 眼镜端调用 `ai_camera_take_photo()` 生成 JPG。
4. 眼镜端通过 SPP 发送 JPG 文件头和原始字节。
5. 客户端按文件头中的 `size` 读取 JPG，显示接收进度，并显示图片。

BLE 只承载短指令和状态；JPG 大数据只走 SPP。

## 目录

```text
combo_camera_spp_demo/
  glasses/
    combo_camera_spp_demo.c
    Makefile
  clients/
    android/
    windows/
```

## 眼镜端

构建：

```bash
cd examples/bluetooth_demo/combo_camera_spp_demo/glasses
make
```

运行前确认 `bt_service`、`ai-core` 已启动，并且存在：

```bash
ls -l /var/run/ai_ble.sock /var/run/ai_spp.sock
```

运行：

```bash
../../build/combo_camera_spp_demo
```

眼镜端订阅：

- BLE 命令：`combo.camera.takephoto`
- BLE 状态：`combo.camera.status`

SPP 文件格式：

```text
OSAIG_JPG_V1 request=<request_id> size=<bytes> name=<filename>\n
<exactly size bytes of JPEG data>
```

## Android client

构建：

```bash
cd examples/bluetooth_demo/combo_camera_spp_demo/clients/android
bash build_android.sh
```

运行：

1. 打开 Android 蓝牙并授予权限。
2. 点击 `Connect BLE + SPP`，等待 BLE 与 SPP 都 ready。
3. 点击 `Take Photo`。
4. 回传期间页面显示 SPP 接收百分比和已收/总大小。
5. 页面收到 JPG 后会直接显示图片。

眼镜端发送 JPG 时也会打印 `send progress request=... sent=<n>/<total> percent=<p>`，用于区分链路仍在传输和真正卡住。

## 协议文档

完整协议见根项目 `docs/api/ai_bluetooth_combo_camera_spp_demo.md`。
