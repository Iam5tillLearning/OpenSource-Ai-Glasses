# Classic Bluetooth SPP Client API

## 1. Overview

The `ai_spp` module lets a local application use a classic Bluetooth
SPP/RFCOMM byte stream. The application does not register a BlueZ Profile
directly. It connects to the SPP broker built into `bt_service`, and that
broker hands the RFCOMM file descriptor to the current SDK owner.

SPP is a raw byte stream:

- The SDK and the `bt_service` SPP broker do not wrap data in JSON.
- The SDK and the `bt_service` SPP broker do not implement application framing,
  retransmission, or checksums.
- After accepting a connection, the application reads and writes the fd
  directly.

## 2. Header And Linking

- Header: `include/ai_spp.h`
- Library: `lib/libai_glass_sdk.a` or `lib/libai_glass_sdk.so`
- Service: SPP broker built into `/oem/usr/bin/bt_service`; owner socket `/var/run/ai_spp.sock`

Cross-compilation example:

```bash
arm-rockchip831-linux-uclibcgnueabihf-gcc \
    -o my_spp_app my_spp_app.c \
    -I/path/to/ai_glass_sdk/include \
    -L/path/to/ai_glass_sdk/lib \
    -lai_glass_sdk \
    -lpthread -lrt
```

## 3. API

```c
typedef struct ai_spp_client_ctx ai_spp_client_t;

typedef struct {
    int fd;
    char remote_addr[AI_SPP_MAX_REMOTE_ADDR_LEN];
    int control_fd;
} ai_spp_connection_t;

typedef void (*ai_spp_connection_cb)(const ai_spp_connection_t *connection,
                                     void *user_data);

ai_spp_client_t *ai_spp_client_create(void);
int ai_spp_client_start(ai_spp_client_t *client,
                        ai_spp_connection_cb callback,
                        void *user_data);
int ai_spp_accept(ai_spp_client_t *client,
                  ai_spp_connection_t *connection,
                  int timeout_ms);
ssize_t ai_spp_read(ai_spp_connection_t *connection,
                    void *buffer,
                    size_t size);
ssize_t ai_spp_write(ai_spp_connection_t *connection,
                     const void *buffer,
                     size_t size);
void ai_spp_close(ai_spp_connection_t *connection);
void ai_spp_client_stop(ai_spp_client_t *client);
void ai_spp_client_destroy(ai_spp_client_t *client);
```

Applications should only use `fd` and `remote_addr` directly. `control_fd` is
an SDK-internal field used by `ai_spp_close()` to notify the service that the
active connection has ended.

## 4. Flow

1. Start `bt_service`.
2. Create a client with `ai_spp_client_create()`.
3. Register the current SPP owner with `ai_spp_client_start()`.
4. Use `ai_spp_accept()` or a callback to receive a connection.
5. Connect from Android or Windows with the OSAIG SDK SPP UUID
   `00001911-0000-1000-8000-00805f9b34fb`; the current server SDP record
   publishes RFCOMM channel `10`.
6. Use `ai_spp_read()` / `ai_spp_write()` or native fd APIs.
7. Close the connection with `ai_spp_close()`.
8. Destroy the client with `ai_spp_client_destroy()` before exit.

The full example is under
`examples/bluetooth_demo/classic_bt_demo/glasses/sdk_spp_demo/`.

## 5. Limits

- Only one SDK owner is supported at a time.
- Only one active SPP connection is supported at a time.
- New SPP connections are rejected if no owner is registered.
- New SPP connections are rejected while another connection is active.
