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
  combo_camera_spp_demo/
    glasses/
    clients/
      android/
      windows/
  latency_demo/
    clients/
      android/
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

经典蓝牙 demo 使用 SPP/RFCOMM。眼镜端由 `bt_service` 内置 broker 注册 OSAIG SDK SPP UUID `00001911-0000-1000-8000-00805f9b34fb` 和 RFCOMM channel `10`，本地 `glasses/sdk_spp_demo/` 通过 `ai_spp_*` API 接收 RFCOMM fd 并直接读写；Android client 扫描 `OSAIG-XXXX` 后使用 insecure RFCOMM socket 连接并显示回显，当前不要求先完成系统蓝牙配对。

眼镜端：

```bash
cd examples/bluetooth_demo/classic_bt_demo/glasses/sdk_spp_demo
make
```

Android client：

```bash
cd examples/bluetooth_demo/classic_bt_demo/clients/android
bash build_android.sh
```

## BLE + SPP 拍照回传综合 demo

综合 demo 演示 BLE 发送拍照短指令、经典蓝牙 SPP 回传 JPG 大数据。眼镜端 `combo_camera_spp_demo/glasses/` 订阅 BLE `combo.camera.takephoto`，调用 `ai_camera_take_photo()` 生成 JPG 后，通过 SPP 发送 `OSAIG_JPG_V1` 文件头和 JPG 原始字节；Android client 收到后解码并显示图片。

眼镜端：

```bash
cd examples/bluetooth_demo/combo_camera_spp_demo/glasses
make
```

Android client：

```bash
cd examples/bluetooth_demo/combo_camera_spp_demo/clients/android
bash build_android.sh
```

## 蓝牙延迟测试 demo

延迟测试 demo 复用现有 BLE 与 SPP 眼镜端 echo 示例，在 Android 侧输出 RTT 统计，并在 Android、`bt_service`、BLE demo、SPP demo 输出每个 `seq` 的转点 trace。

眼镜端先按测试类型启动现有 demo：

```bash
cd examples/bluetooth_demo/ble_demo/glasses
make
./../../../build/ble_demo
```

或：

```bash
cd examples/bluetooth_demo/classic_bt_demo/glasses/sdk_spp_demo
make
./../../../build/spp_sdk_demo
```

Android client：

```bash
cd examples/bluetooth_demo/latency_demo/clients/android
bash build_android.sh
```

日志入口：

- Android：`adb logcat -s OSAIG_BT_LATENCY`
- BLE demo：`[BLE_DEMO][TRACE]`
- SPP demo：`[SPP_SDK_DEMO][TRACE]`
- `bt_service` BLE 路径：`[AI_BLE][LATENCY]`
