# 蓝牙 Demo

本目录统一放置 AI 眼镜蓝牙通信示例，按蓝牙类型和通信角色划分：

```text
bluetooth_demo/
  ble_demo/
    glasses/
    clients/
      android/
      windows/
  classic_bt_demo/
    glasses/
    clients/
      android/
      windows/
```

- `glasses/`：眼镜端示例或眼镜端运行说明。
- `clients/`：外部客户端示例，包含 Android 手机、Android 平板、Windows 电脑等。

## BLE demo

BLE demo 通过 `bt_service` 暴露的 GATT characteristic 传输 UTF-8 JSON 文本。眼镜端 `glasses/ble_demo.c` 订阅 `sdk.demo.ping`，收到后回发 `sdk.demo.pong`。

```bash
cd examples/bluetooth_demo/ble_demo/glasses
make
```

Android client：

```bash
cd examples/bluetooth_demo/ble_demo/clients/android
bash build_android.sh
```

## 经典蓝牙 demo

经典蓝牙 demo 使用 SPP/RFCOMM。眼镜端由 `bt_service` 启用 SPP server，收到客户端字节后原样回写；Android client 连接已配对的 `OSAIG-XXXX` 设备并显示回显。

```bash
cd examples/bluetooth_demo/classic_bt_demo/clients/android
bash build_android.sh
```
