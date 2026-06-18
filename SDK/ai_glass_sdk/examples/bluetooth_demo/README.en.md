# Bluetooth Demo

This directory contains AI glasses Bluetooth communication demos, organized by Bluetooth type and communication role:

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
```

- `glasses/`: glasses-side demo or runtime notes.
- `clients/`: external clients, including Android phones, Android tablets, Windows PCs, and other Bluetooth peers.

## BLE demo

The BLE demo transfers UTF-8 JSON text through the GATT characteristic exposed by `bt_service`. The glasses-side `glasses/ble_demo.c` subscribes to `sdk.demo.ping` and replies with `sdk.demo.pong`.

```bash
cd examples/bluetooth_demo/ble_demo/glasses
make
```

Android client:

```bash
cd examples/bluetooth_demo/ble_demo/clients/android
bash build_android.sh
```

## Classic Bluetooth demo

The classic Bluetooth demo uses SPP/RFCOMM. The glasses-side `bt_service` enables an SPP server and echoes received client bytes. The Android client connects to a paired `OSAIG-XXXX` device and displays the echoed text.

```bash
cd examples/bluetooth_demo/classic_bt_demo/clients/android
bash build_android.sh
```
