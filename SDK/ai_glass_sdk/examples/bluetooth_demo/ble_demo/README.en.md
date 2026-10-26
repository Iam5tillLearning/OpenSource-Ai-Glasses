# BLE Roundtrip Demo

This directory contains the glasses side and external clients for the BLE SDK demo:

- Glasses side: `glasses/ble_demo.c`, which subscribes to `sdk.demo.ping` through `ai_glass_sdk` and replies with `sdk.demo.pong`.
- Android client: `clients/android/`, which connects to the glasses through Android BLE APIs, sends a test command, and displays the notify response.
- Windows client: `clients/windows/`, reserved for a Windows BLE GATT client.

The demo uses dedicated example datatypes so it does not conflict with production features:

```json
{"datatype":"sdk.demo.ping","data":"hello from android"}
{"datatype":"sdk.demo.pong","data":"ack:hello from android"}
```

## 1. Build the Glasses-Side Demo

```bash
cd examples/bluetooth_demo/ble_demo/glasses
make
```

Output:

```text
examples/build/ble_demo
```

## 2. Run the Glasses-Side Demo

Make sure `bt_service` is running on the glasses and the local socket exists:

```bash
ls -l /var/run/ai_ble.sock
```

Run:

```bash
./../../../build/ble_demo
```

The demo subscribes to `sdk.demo.ping`. When it receives a mobile message, it prints a log and sends `sdk.demo.pong` back through `ai_ble_send()`.

## 3. Build the Android Demo

The Android demo is located at:

```text
examples/bluetooth_demo/ble_demo/clients/android/
```

Open this directory with Android Studio, or run this command in an environment with Gradle installed:

```bash
cd examples/bluetooth_demo/ble_demo/clients/android
bash build_android.sh
```

Inside this development repository, `build_android.sh` uses system `gradle` first. If it is unavailable, it reuses the Gradle wrapper from the OSAIG Android project.

## 4. Android Client Operation

1. Install the Android demo APK.
2. Enable Bluetooth. Some Android devices also require Location Services for BLE scanning.
3. Tap `Scan OSAIG`.
4. The demo scans and connects to devices named `OSAIG-XXXX`.
5. After connection, tap `Send Ping`.
6. The page should show the received `sdk.demo.pong` response.

## 5. Windows Client Extension

Windows should connect as a BLE GATT client to Service UUID `00001910-0000-1000-8000-00805f9b34fb` and Characteristic UUID `dfd4416e-1810-47f7-8248-eb8be3dc47f9`. Write UTF-8 JSON to the characteristic and receive responses through notify. See the root project `docs/api/ai_ble_protocol.md` for the message fields and packet limit.

## 6. Troubleshooting

- No scan result: verify that the glasses BLE name is `OSAIG-XXXX`, and check client Bluetooth permissions and Location Services.
- No response after connection: make sure the glasses-side `ble_demo` is running and `bt_service` logs show `sdk.demo.ping`.
- Send failure on the glasses side: make sure Android notify is enabled and the JSON packet is no longer than 180 bytes.
