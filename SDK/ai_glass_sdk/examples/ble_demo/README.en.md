# BLE Roundtrip Demo

This directory contains both sides of the BLE SDK demo:

- Glasses side: `ble_demo.c`, which subscribes to `sdk.demo.ping` through `ai_glass_sdk` and replies with `sdk.demo.pong`.
- Android side: `android/`, which connects to the glasses through Android BLE APIs, sends a test command, and displays the notify response.

The demo uses dedicated example datatypes so it does not conflict with production features:

```json
{"datatype":"sdk.demo.ping","data":"hello from android"}
{"datatype":"sdk.demo.pong","data":"ack:hello from android"}
```

## 1. Build the Glasses-Side Demo

```bash
cd examples/ble_demo
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
./examples/build/ble_demo
```

The demo subscribes to `sdk.demo.ping`. When it receives a mobile message, it prints a log and sends `sdk.demo.pong` back through `ai_ble_send()`.

## 3. Build the Android Demo

The Android demo is located at:

```text
examples/ble_demo/android/
```

Open this directory with Android Studio, or run this command in an environment with Gradle installed:

```bash
cd examples/ble_demo/android
bash build_android.sh
```

Inside this development repository, `build_android.sh` uses system `gradle` first. If it is unavailable, it reuses the Gradle wrapper from the OSAIG Android project.

## 4. Mobile Operation

1. Install the Android demo APK.
2. Enable Bluetooth. Some Android phones also require Location Services for BLE scanning.
3. Tap `Scan OSAIG`.
4. The demo scans and connects to devices named `OSAIG-XXXX`.
5. After connection, tap `Send Ping`.
6. The page should show the received `sdk.demo.pong` response.

## 5. Troubleshooting

- No scan result: verify that the glasses BLE name is `OSAIG-XXXX`, and check Bluetooth permissions and Location Services.
- No response after connection: make sure the glasses-side `ble_demo` is running and `bt_service` logs show `sdk.demo.ping`.
- Send failure on the glasses side: make sure Android notify is enabled and the JSON packet is no longer than 180 bytes.
