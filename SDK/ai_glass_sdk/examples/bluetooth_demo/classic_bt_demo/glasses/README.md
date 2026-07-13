# 眼镜端 classic BT SPP demo

正式 classic BT SPP 数据面由 `bt_service` 内置 SPP broker 提供：

1. `bt_service` 不启用 `PROFILE_SPP`，避免 RK receive path 抢读。
2. `bt_service` 内置 broker 向 BlueZ 注册 OSAIG SDK SPP UUID `00001911-0000-1000-8000-00805f9b34fb`。
3. 本目录 `sdk_spp_demo/` 使用 `ai_spp_*` API 注册 owner。
4. Android 或 Windows 客户端连接 OSAIG SDK SPP UUID 后，SDK demo 直接收到 RFCOMM fd 并读写。

构建蓝牙基础服务：

```bash
cd bt_service/bt/rk_btapp
make
```

构建 SDK 和眼镜端 demo：

```bash
cd SDK/ai_glass_sdk
make
```

单独构建眼镜端 demo：

```bash
cd SDK/ai_glass_sdk/examples/bluetooth_demo/classic_bt_demo/glasses/sdk_spp_demo
make
```

构建产物：

```text
SDK/ai_glass_sdk/examples/bluetooth_demo/build/spp_sdk_demo
```

真机运行顺序：先启动 `bt_service`，再启动 `spp_sdk_demo`；设备侧不应存在独立 `ai_spp_service` 进程。
