# Windows 客户端入口

当前目录保留 Windows 综合蓝牙客户端扩展入口，协议与 Android client 相同：

1. 通过 Windows Bluetooth LE API 连接 OSAIG BLE Service `00001910-0000-1000-8000-00805f9b34fb`。
2. 启用 Characteristic `dfd4416e-1810-47f7-8248-eb8be3dc47f9` 的 notify。
3. 通过 Windows RFCOMM/SPP API 连接 OSAIG SDK SPP UUID `00001911-0000-1000-8000-00805f9b34fb`。
4. BLE 写入：

```json
{"datatype":"combo.camera.takephoto","data":"req_1718600000000"}
```

5. 从 SPP 读取：

```text
OSAIG_JPG_V1 request=<request_id> size=<bytes> name=<filename>\n
<exactly size bytes of JPEG data>
```

第一版只提供 Android 可构建客户端；Windows 可按该协议补齐 C#、C++/WinRT 或 Python RFCOMM 示例。
