# Windows classic BT client

Windows 端作为 RFCOMM/SPP client 接入眼镜：

- 先在 Windows 蓝牙设置中与 `OSAIG-XXXX` 完成经典蓝牙配对。
- 使用标准 SPP UUID `00001101-0000-1000-8000-00805f9b34fb` 建立 RFCOMM 连接。
- 写入 UTF-8 文本后，眼镜端 `bt_service` 会原样回显收到的数据。

Windows 可使用 `Windows.Devices.Bluetooth.Rfcomm` API 实现 RFCOMM client。该目录保留为后续补充 Windows 可构建示例的入口。
