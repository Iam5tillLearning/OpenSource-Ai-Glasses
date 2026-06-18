# Classic Bluetooth SPP Demo

This demo shows a minimal classic Bluetooth SPP/RFCOMM text echo flow:

- Glasses side: `glasses/`, backed by `bt_service`, which enables an SPP server and echoes received client bytes.
- Android client: `clients/android/`, which connects to a paired `OSAIG-XXXX` device, sends text, and displays the echo.
- Windows client: `clients/windows/`, reserved for a Windows RFCOMM/SPP client.

The demo uses the standard Serial Port Profile UUID:

```text
00001101-0000-1000-8000-00805f9b34fb
```

## Android client

```bash
cd examples/bluetooth_demo/classic_bt_demo/clients/android
bash build_android.sh
```

Pair the Android device with the glasses in system Bluetooth settings first. Then open the demo, tap `List Paired OSAIG`, `Connect`, and `Send Echo`.
