# BLE Text Client API

## 1. Overview

The `ai_ble` module provides local applications with a unified BLE text-message access layer.
Applications do not operate BLE GATT directly. Instead, they connect to the local Unix Socket gateway exposed by `bt_service` through `ai_glass_sdk`.

The current V1 protocol uses UTF-8 JSON text:

```json
{"datatype":"display.text","data":"hello"}
```

Rules:
- `datatype` is required and must be a string
- `data` is required and must be a string
- The full UTF-8 JSON packet must not exceed `180` bytes
- Multiple local applications can subscribe to the same `datatype`

## 2. Header and Linking

- Header: `include/ai_ble.h`
- Library: `lib/libai_glass_sdk.a` or `lib/libai_glass_sdk.so`

Cross-compilation example:

```bash
arm-rockchip831-linux-uclibcgnueabihf-gcc \
    -o my_ble_app my_ble_app.c \
    -I/path/to/ai_glass_sdk/include \
    -L/path/to/ai_glass_sdk/lib \
    -lai_glass_sdk \
    -lpthread -lrt
```

## 3. API

### 3.1 Types

```c
typedef struct ai_ble_client_ctx ai_ble_client_t;
typedef void (*ai_ble_text_cb)(const char *datatype, const char *data, void *user_data);
```

### 3.2 Create and Start

```c
ai_ble_client_t* ai_ble_client_create(void);
int ai_ble_client_start(ai_ble_client_t *client);
void ai_ble_client_stop(ai_ble_client_t *client);
void ai_ble_client_destroy(ai_ble_client_t *client);
```

Notes:
- `ai_ble_client_create()` creates a client handle.
- `ai_ble_client_start()` starts the background receive thread and tries to connect to `/var/run/ai_ble.sock`.
- If `bt_service` is not available yet, the client reconnects in the background.
- A successful `ai_ble_client_start()` only means the receive thread has started. It does not guarantee the socket is connected at that exact moment.

### 3.3 `datatype` Subscription

```c
int ai_ble_register_datatype(ai_ble_client_t *client,
                             const char *datatype,
                             ai_ble_text_cb cb,
                             void *user_data);

int ai_ble_unregister_datatype(ai_ble_client_t *client,
                               const char *datatype);
```

Notes:
- An application receives messages only for the `datatype` values it has registered.
- Multiple local applications can register the same `datatype`.
- Registering the same `datatype` again on the same client updates the callback and `user_data`.

`datatype` naming limits:
- Only lowercase ASCII letters, digits, `.`, and `_` are allowed
- The first character must be a lowercase letter
- Maximum length is 32 bytes
- Recommended examples: `camera.open`, `display.text`, `ai.asr.start`

### 3.4 Send Messages

```c
int ai_ble_send(ai_ble_client_t *client, const char *datatype, const char *data);
```

Notes:
- The SDK encodes `datatype + data` as UTF-8 JSON text and sends it to `bt_service`.
- `bt_service` then sends the message to the mobile side through BLE notify.
- Sending fails if the encoded JSON packet exceeds `180` bytes.

## 4. Example

```c
#include "ai_ble.h"
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

static volatile int running = 1;

static void signal_handler(int sig) {
    (void)sig;
    running = 0;
}

static void on_ble_text(const char *datatype, const char *data, void *user_data) {
    (void)user_data;
    printf("recv %s => %s\n", datatype, data);
}

int main(void) {
    ai_ble_client_t *client = ai_ble_client_create();
    if (!client)
        return 1;

    signal(SIGINT, signal_handler);

    if (ai_ble_client_start(client) != 0) {
        ai_ble_client_destroy(client);
        return 1;
    }

    if (ai_ble_register_datatype(client, "display.text", on_ble_text, NULL) != 0) {
        ai_ble_client_destroy(client);
        return 1;
    }

    sleep(1);
    if (ai_ble_send(client, "display.text", "hello from local app") != 0) {
        printf("send failed\n");
    }

    while (running) {
        sleep(1);
    }

    ai_ble_client_destroy(client);
    return 0;
}
```

## 5. Receive Flow

```text
Mobile phone
  ↓ BLE write
bt_service
  ↓ Parse JSON
  ↓ Route by datatype
ai_glass_sdk
  ↓ Callback
Local application
```

## 6. BLE Roundtrip Demo

The SDK provides `examples/ble_demo/` as a complete mobile/glasses roundtrip reference:

- The glasses-side `ble_demo.c` subscribes to `sdk.demo.ping`.
- The Android demo under `android/` scans `OSAIG-XXXX` and sends `sdk.demo.ping`.
- The glasses side receives the message and replies with `sdk.demo.pong` through `ai_ble_send()`.
- The Android side enables notify and displays `sdk.demo.pong`.

Example messages:

```json
{"datatype":"sdk.demo.ping","data":"hello from android"}
{"datatype":"sdk.demo.pong","data":"ack:hello from android"}
```

Build the glasses-side demo:

```bash
cd examples/ble_demo
make
```

Build the Android demo:

```bash
cd examples/ble_demo/android
bash build_android.sh
```

These demo datatypes are only for SDK examples. They do not trigger camera, stream, display, or fixed toast behavior.

## 7. Troubleshooting

### 7.1 `ai_ble_client_start()` Fails

Check:
- Whether `bt_service` is running
- Whether `/var/run/ai_ble.sock` exists
- Whether the current process can access the socket

### 7.2 No Messages Received

Check:
- Whether the target `datatype` has been registered
- Whether the JSON sent by the mobile side contains `datatype` and `data`
- Whether the full UTF-8 JSON packet is no longer than `180` bytes

### 7.3 Send Failure

Check:
- Whether `datatype` follows the naming rules
- Whether `data` is valid UTF-8
- Whether the encoded packet exceeds `180` bytes
- Whether notify is enabled on the mobile side
