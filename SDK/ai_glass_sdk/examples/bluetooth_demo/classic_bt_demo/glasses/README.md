# 眼镜端 classic BT SPP demo

眼镜端不需要单独启动 SDK demo 进程。经典蓝牙 SPP server 由 `bt_service` 提供：

1. `bt_service` 默认 profile 需要包含 `PROFILE_SPP`。
2. Android 或 Windows 客户端先与眼镜 `OSAIG-XXXX` 完成经典蓝牙配对。
3. 客户端使用标准 SPP UUID `00001101-0000-1000-8000-00805f9b34fb` 建立 RFCOMM 连接。
4. `bt_service` 收到 `RK_BT_STATE_SPP_RECV_DATA` 后把收到的字节原样写回当前 SPP fd。

构建眼镜端服务：

```bash
cd InternalProjects/bt_service/bt/rk_btapp
make
```

构建完成后，模块 Makefile 会同步 stripped `bt_service` 到 `project/oem/bin/bt_service`。真机验证时应按设备真实启动链路替换并启动 `bt_service`，不要用 `/tmp` 临时前台运行来规避服务替换。
