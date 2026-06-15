# BLE 往返 Demo

本目录同时包含 BLE SDK demo 的两端示例：

- 眼镜端：`ble_demo.c`，通过 `ai_glass_sdk` 订阅 `sdk.demo.ping` 并回发 `sdk.demo.pong`。
- Android 端：`android/`，通过系统 BLE API 连接眼镜，发送测试指令并展示 notify 回包。

该 demo 使用独立的示例 datatype，避免和真实业务能力混淆：

```json
{"datatype":"sdk.demo.ping","data":"hello from android"}
{"datatype":"sdk.demo.pong","data":"ack:hello from android"}
```

## 1. 编译眼镜端示例

```bash
cd examples/ble_demo
make
```

产物路径：

```text
examples/build/ble_demo
```

## 2. 运行眼镜端示例

确认眼镜端 `bt_service` 已启动，并且存在本地 Socket：

```bash
ls -l /var/run/ai_ble.sock
```

运行：

```bash
./examples/build/ble_demo
```

示例启动后会订阅 `sdk.demo.ping`。收到手机端消息后会打印日志，并通过 `ai_ble_send()` 回发 `sdk.demo.pong`。

## 3. 编译 Android 示例

Android 示例位于：

```text
examples/ble_demo/android/
```

可直接用 Android Studio 打开该目录，也可以在已安装 Gradle 的环境中执行：

```bash
cd examples/ble_demo/android
bash build_android.sh
```

在本开发仓库内，`build_android.sh` 会优先使用系统 `gradle`，若不存在则复用仓库中 OSAIG Android 工程的 Gradle wrapper。

## 4. 手机端操作

1. 安装 Android demo APK。
2. 打开手机蓝牙；部分 Android 机型还需要开启系统定位服务才能扫描 BLE。
3. 点击 `Scan OSAIG`。
4. demo 会扫描并连接名称符合 `OSAIG-XXXX` 的设备。
5. 连接成功后点击 `Send Ping`。
6. 页面中应显示收到的 `sdk.demo.pong` 回包。

## 5. 故障排查

- 扫描不到设备：确认眼镜 BLE 名称为 `OSAIG-XXXX`，并检查手机蓝牙权限和定位服务。
- 连接后没有回包：确认眼镜端 `ble_demo` 正在运行，且 `bt_service` 日志中能看到 `sdk.demo.ping`。
- 眼镜端发送失败：确认 Android 端已经开启 notify，且 JSON 包未超过 180 字节。
