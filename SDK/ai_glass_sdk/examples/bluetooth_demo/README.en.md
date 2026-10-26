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
  combo_camera_spp_demo/
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

The classic Bluetooth demo uses SPP/RFCOMM. The broker built into `bt_service` registers the OSAIG SDK SPP UUID `00001911-0000-1000-8000-00805f9b34fb` on RFCOMM channel `10`, and `glasses/sdk_spp_demo/` receives the RFCOMM fd through the `ai_spp_*` API and reads/writes it directly. The Android client scans for `OSAIG-XXXX`, connects with an insecure RFCOMM socket, and displays the echoed text; the current demo does not require prior system Bluetooth pairing.

Glasses side:

```bash
cd examples/bluetooth_demo/classic_bt_demo/glasses/sdk_spp_demo
make
```

Android client:

```bash
cd examples/bluetooth_demo/classic_bt_demo/clients/android
bash build_android.sh
```

## BLE + SPP camera transfer demo

The combo demo sends a short take-photo command over BLE and transfers the resulting JPG over classic Bluetooth SPP. The glasses-side `combo_camera_spp_demo/glasses/` subscribes to BLE `combo.camera.takephoto`, calls `ai_camera_take_photo()`, then sends an `OSAIG_JPG_V1` file header and raw JPG bytes over SPP. The Android client decodes and displays the received image.

Glasses side:

```bash
cd examples/bluetooth_demo/combo_camera_spp_demo/glasses
make
```

Android client:

```bash
cd examples/bluetooth_demo/combo_camera_spp_demo/clients/android
bash build_android.sh
```
