# Windows BLE Client

Windows 端作为 BLE GATT client 接入眼镜：

- Service UUID：`00001910-0000-1000-8000-00805f9b34fb`
- Characteristic UUID：`dfd4416e-1810-47f7-8248-eb8be3dc47f9`
- 写入方向：向 characteristic 写入 UTF-8 JSON。
- 回包方向：订阅 characteristic notify。

示例请求：

```json
{"datatype":"sdk.demo.ping","data":"hello from windows"}
```

预期回包：

```json
{"datatype":"sdk.demo.pong","data":"ack:hello from windows"}
```

BLE 正式协议见根项目 `docs/api/ai_ble_protocol.md`。
