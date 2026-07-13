# 蓝牙延迟测试 Demo

本 demo 用于测量 BLE 与经典蓝牙 SPP 的小包往返延迟 RTT。

- BLE：复用 `ble_demo/glasses/ble_demo.c`，Android 发送 `sdk.demo.ping`，眼镜端回发 `sdk.demo.pong`。
- 经典蓝牙 SPP：复用 `classic_bt_demo/glasses/sdk_spp_demo/spp_sdk_demo.c`，Android 发送一行文本，眼镜端原样 echo。
- 转点 trace：Android、`bt_service`、BLE demo 和 SPP demo 均输出每个 `seq` 的关键转点时间，便于定位 RTT 来源。

统计口径见根项目文档 `docs/api/ai_bluetooth_latency_demo.md`。

## 眼镜端

BLE RTT 测试前启动现有 BLE demo：

```bash
cd examples/bluetooth_demo/ble_demo/glasses
make
./../../../build/ble_demo
```

SPP RTT 测试前启动现有 SPP SDK demo：

```bash
cd examples/bluetooth_demo/classic_bt_demo/glasses/sdk_spp_demo
make
./../../../build/spp_sdk_demo
```

## Android client

```bash
cd examples/bluetooth_demo/latency_demo/clients/android
bash build_android.sh
```

多台设备同时在场时，可通过 `target_address` 指定目标眼镜蓝牙地址：

```bash
adb shell 'am start -n com.osaig.sdk.bluetooth.latency.demo/.MainActivity --es target_address 22:22:43:96:41:9E'
```

页面操作：

1. 点击 `Connect BLE`，等待 BLE ready。
2. 点击 `Run BLE RTT`。
3. 点击 `Run SPP RTT`。

Android logcat 可用 `OSAIG_BT_LATENCY` 过滤测试证据。

眼镜端日志可关注：

- BLE demo：`[BLE_DEMO][TRACE]`
- SPP demo：`[SPP_SDK_DEMO][TRACE]`
- `bt_service`：`[AI_BLE][LATENCY]`
