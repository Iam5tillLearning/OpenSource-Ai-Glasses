# 经典蓝牙 SPP 客户端 API

## 1. 概述

`ai_spp` 模块为本地应用提供经典蓝牙 SPP/RFCOMM 大数据通道接入能力。
应用不直接注册 BlueZ Profile，而是通过 `ai_glass_sdk` 连接
`bt_service` 内置 SPP broker，由该 broker 把 RFCOMM fd 交给当前 SDK owner。

SPP 数据面是 raw byte stream：

- SDK 和 `bt_service` 内置 SPP broker 不封装 JSON。
- SDK 和 `bt_service` 内置 SPP broker 不做业务分片、重传或校验。
- 应用拿到 fd 后直接 `read/write/poll`。

## 2. 头文件与链接

- 头文件：`include/ai_spp.h`
- 库文件：`lib/libai_glass_sdk.a` 或 `lib/libai_glass_sdk.so`
- 服务端：`/oem/usr/bin/bt_service` 内置 SPP broker，owner socket 为 `/var/run/ai_spp.sock`

交叉编译示例：

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

应用只应直接使用 `fd` 和 `remote_addr`。`control_fd` 是 SDK 内部字段，
用于 `ai_spp_close()` 通知服务释放当前 active 状态。

## 4. 使用流程

1. 启动 `bt_service`。
2. 应用调用 `ai_spp_client_create()`。
3. 应用调用 `ai_spp_client_start()` 注册当前 SPP owner。
4. 阻塞式应用调用 `ai_spp_accept()` 等待连接；回调式应用传入 callback。
5. 外部 Android 或 Windows 设备用 OSAIG SDK SPP UUID `00001911-0000-1000-8000-00805f9b34fb` 连接眼镜；当前服务端 SDP 固定发布 RFCOMM channel `10`。
6. 应用通过 `ai_spp_read()` / `ai_spp_write()` 或原生 fd API 读写。
7. 应用调用 `ai_spp_close()` 关闭当前连接。
8. 应用退出前调用 `ai_spp_client_destroy()`。

## 5. 最小示例

```c
#include "ai_spp.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    ai_spp_client_t *client = ai_spp_client_create();
    ai_spp_connection_t connection;
    char buffer[512];
    ssize_t n;

    if (!client)
        return 1;

    if (ai_spp_client_start(client, NULL, NULL) != 0) {
        ai_spp_client_destroy(client);
        return 1;
    }

    if (ai_spp_accept(client, &connection, -1) != 0) {
        ai_spp_client_destroy(client);
        return 1;
    }

    ai_spp_write(&connection, "hello from glasses\n", 19);
    n = ai_spp_read(&connection, buffer, sizeof(buffer));
    if (n > 0)
        printf("recv %zd bytes\n", n);

    ai_spp_close(&connection);
    ai_spp_client_destroy(client);
    return 0;
}
```

完整示例见 `examples/bluetooth_demo/classic_bt_demo/glasses/sdk_spp_demo/`。

## 6. 限制

- 同一时刻只允许一个 SDK owner。
- 同一时刻只允许一个活跃 SPP 连接。
- 无 owner 时，新的外部 SPP 连接会被拒绝。
- 已有活跃连接时，新的外部 SPP 连接会被拒绝。
