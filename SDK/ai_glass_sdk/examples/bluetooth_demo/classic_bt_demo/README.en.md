# Classic Bluetooth SPP Demo

This demo shows a minimal classic Bluetooth SPP/RFCOMM text echo flow:

- Glasses side: `glasses/sdk_spp_demo/`, backed by the SPP broker built into `bt_service`, which receives the RFCOMM fd and reads/writes it directly.
- Android client: `clients/android/`, which scans for `OSAIG-XXXX`, connects with an insecure RFCOMM socket, sends text, and displays the echo.
- Windows client: `clients/windows/`, reserved for a Windows RFCOMM/SPP client.

The demo uses the OSAIG SDK SPP UUID:

```text
00001911-0000-1000-8000-00805f9b34fb
```

The first glasses-side implementation registers RFCOMM channel `10`; use
`sdptool browse local` to confirm the SDP record contains `RFCOMM Channel: 10`.

The current system Bluetooth stack exposes the standard Serial Port UUID
`00001101-0000-1000-8000-00805f9b34fb` by default, so the SDK SPP broker uses a
project-owned UUID to avoid BlueZ `UUID already registered` conflicts.

## Android client

Start `bt_service` and the glasses-side SDK demo first:

```bash
cd examples/bluetooth_demo/classic_bt_demo/glasses/sdk_spp_demo
make
./../../../build/spp_sdk_demo
```

```bash
cd examples/bluetooth_demo/classic_bt_demo/clients/android
bash build_android.sh
```

The current Android demo uses an insecure RFCOMM socket and does not require pairing in system Bluetooth settings first. Open the demo and tap `Run SPP FD Test`; the app connects to `OSAIG-XXXX` and sends test text.
