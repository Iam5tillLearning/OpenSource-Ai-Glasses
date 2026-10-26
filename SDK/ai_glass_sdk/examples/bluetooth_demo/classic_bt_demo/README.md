# 经典蓝牙 SPP Demo

本 demo 演示经典蓝牙 SPP/RFCOMM 文本回显：

- 眼镜端：`glasses/sdk_spp_demo/`，通过 `bt_service` 内置 broker 获取 RFCOMM fd，并由应用直接读写。
- Android 客户端：`clients/android/`，扫描 `OSAIG-XXXX` 后使用 insecure RFCOMM socket 连接，发送文本并显示回显。
- Windows 客户端：`clients/windows/`，保留 Windows RFCOMM/SPP client 接入口径。

SPP UUID 使用 OSAIG SDK SPP UUID：

```text
00001911-0000-1000-8000-00805f9b34fb
```

眼镜端第一版固定注册 RFCOMM channel `10`，可用 `sdptool browse local` 确认 SDP 记录中包含 `RFCOMM Channel: 10`。

当前系统蓝牙栈会默认暴露标准 Serial Port UUID `00001101-0000-1000-8000-00805f9b34fb`，SDK SPP broker 不复用该 UUID，避免 BlueZ `UUID already registered` 冲突。

## Android client

眼镜端先启动 `bt_service` 和 SDK demo：

```bash
cd examples/bluetooth_demo/classic_bt_demo/glasses/sdk_spp_demo
make
./../../../build/spp_sdk_demo
```

```bash
cd examples/bluetooth_demo/classic_bt_demo/clients/android
bash build_android.sh
```

当前 Android demo 使用 insecure RFCOMM socket，不要求先在系统蓝牙设置中与眼镜完成配对。demo 启动后点击 `Run SPP FD Test`，应用会连接 `OSAIG-XXXX` 并发送测试文本。
