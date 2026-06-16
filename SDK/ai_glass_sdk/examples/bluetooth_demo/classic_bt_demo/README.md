# 经典蓝牙 SPP Demo

本 demo 演示经典蓝牙 SPP/RFCOMM 文本回显：

- 眼镜端：`glasses/`，由 `bt_service` 启用 SPP server 并原样回显收到的客户端数据。
- Android 客户端：`clients/android/`，连接已配对的 `OSAIG-XXXX` 设备，发送文本并显示回显。
- Windows 客户端：`clients/windows/`，保留 Windows RFCOMM/SPP client 接入口径。

SPP UUID 使用标准 Serial Port Profile UUID：

```text
00001101-0000-1000-8000-00805f9b34fb
```

## Android client

```bash
cd examples/bluetooth_demo/classic_bt_demo/clients/android
bash build_android.sh
```

使用前先在 Android 系统蓝牙设置中与眼镜完成配对。demo 启动后点击 `List Paired OSAIG`，再点击 `Connect` 和 `Send Echo`。
