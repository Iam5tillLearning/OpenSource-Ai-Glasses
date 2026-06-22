# BLE + SPP Camera Transfer Demo

This demo shows how an SDK app can combine BLE and classic Bluetooth SPP:

1. The client connects SPP/RFCOMM first, then connects BLE GATT.
2. The client sends `combo.camera.takephoto` over BLE.
3. The glasses-side demo calls `ai_camera_take_photo()` to create a JPG.
4. The glasses-side demo sends the JPG header and raw bytes over SPP.
5. The client reads exactly the advertised JPG size, shows receive progress,
   and displays the image.

BLE carries only short commands and status. JPG file data is transferred only through SPP.

## Layout

```text
combo_camera_spp_demo/
  glasses/
    combo_camera_spp_demo.c
    Makefile
  clients/
    android/
    windows/
```

## Glasses Side

Build:

```bash
cd examples/bluetooth_demo/combo_camera_spp_demo/glasses
make
```

Before running, make sure `bt_service` and `ai-core` are running and these sockets exist:

```bash
ls -l /var/run/ai_ble.sock /var/run/ai_spp.sock
```

Run:

```bash
../../build/combo_camera_spp_demo
```

BLE datatypes:

- Command: `combo.camera.takephoto`
- Status: `combo.camera.status`

SPP file format:

```text
OSAIG_JPG_V1 request=<request_id> size=<bytes> name=<filename>\n
<exactly size bytes of JPEG data>
```

## Android Client

Build:

```bash
cd examples/bluetooth_demo/combo_camera_spp_demo/clients/android
bash build_android.sh
```

Run:

1. Enable Android Bluetooth and grant the required permissions.
2. Tap `Connect BLE + SPP` and wait until both channels are ready.
3. Tap `Take Photo`.
4. The page shows SPP receive percent and received/total bytes during transfer.
5. The page displays the received JPG.

The glasses-side demo also prints `send progress request=... sent=<n>/<total> percent=<p>`
while writing the JPG, so logs can distinguish a slow transfer from a stalled link.
