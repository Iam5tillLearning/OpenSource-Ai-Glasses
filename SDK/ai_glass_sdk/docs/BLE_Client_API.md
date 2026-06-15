# BLE 文本客户端 API

## 1. 概述

`ai_ble` 模块为本地应用提供统一的 BLE 文本消息接入能力。  
应用不直接操作 BLE GATT，而是通过 `ai_glass_sdk` 连接 `bt_service` 提供的本地 Unix Socket 网关。

当前 V1 协议固定为 UTF-8 JSON 文本：

```json
{"datatype":"display.text","data":"hello"}
```

规则：
- `datatype` 必填，字符串类型
- `data` 必填，字符串类型
- 整个 UTF-8 JSON 包长度不得超过 `180` 字节
- 同一个 `datatype` 可被多个本地应用同时订阅

## 2. 头文件与链接

- 头文件：`include/ai_ble.h`
- 库文件：`lib/libai_glass_sdk.a` 或 `lib/libai_glass_sdk.so`

交叉编译示例：

```bash
arm-rockchip831-linux-uclibcgnueabihf-gcc \
    -o my_ble_app my_ble_app.c \
    -I/path/to/ai_glass_sdk/include \
    -L/path/to/ai_glass_sdk/lib \
    -lai_glass_sdk \
    -lpthread -lrt
```

## 3. API

### 3.1 类型定义

```c
typedef struct ai_ble_client_ctx ai_ble_client_t;
typedef void (*ai_ble_text_cb)(const char *datatype, const char *data, void *user_data);
```

### 3.2 创建与启动

```c
ai_ble_client_t* ai_ble_client_create(void);
int ai_ble_client_start(ai_ble_client_t *client);
void ai_ble_client_stop(ai_ble_client_t *client);
void ai_ble_client_destroy(ai_ble_client_t *client);
```

说明：
- `ai_ble_client_create()` 创建客户端句柄
- `ai_ble_client_start()` 启动后台接收线程，并尝试连接 `/var/run/ai_ble.sock`
- 如果 `bt_service` 暂时未启动，客户端会在后台自动重连
- `ai_ble_client_start()` 返回成功仅表示线程已启动，不表示此刻一定已经建立 socket 连接

### 3.3 datatype 订阅

```c
int ai_ble_register_datatype(ai_ble_client_t *client,
                             const char *datatype,
                             ai_ble_text_cb cb,
                             void *user_data);

int ai_ble_unregister_datatype(ai_ble_client_t *client,
                               const char *datatype);
```

说明：
- 应用只有注册了某个 `datatype`，才会收到对应消息
- 同一个 `datatype` 可以被多个本地应用同时注册
- 同一个客户端重复注册同一 `datatype` 时，会更新回调和 `user_data`

`datatype` 命名限制：
- 仅允许 ASCII 小写字母、数字、`.`、`_`
- 首字符必须为小写字母
- 最大长度 32 字节
- 推荐示例：`camera.open`、`display.text`、`ai.asr.start`

### 3.4 发送消息

```c
int ai_ble_send(ai_ble_client_t *client, const char *datatype, const char *data);
```

说明：
- SDK 会将 `datatype + data` 封装为 UTF-8 JSON 文本并发送到 `bt_service`
- `bt_service` 再通过 BLE notify 将消息发送给手机端
- 如果 JSON 编码后超过 `180` 字节，发送会失败

## 4. 使用示例

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

## 5. 接收流程

```text
手机
  ↓ BLE write
bt_service
  ↓ 解析 JSON
  ↓ 按 datatype 路由
ai_glass_sdk
  ↓ 回调
本地应用
```

## 6. BLE 往返 demo

SDK 提供 `examples/ble_demo/` 作为完整手机端/眼镜端往返示例：

- 眼镜端 `ble_demo.c` 订阅 `sdk.demo.ping`。
- Android 端 `android/` 扫描 `OSAIG-XXXX` 并发送 `sdk.demo.ping`。
- 眼镜端收到后通过 `ai_ble_send()` 回发 `sdk.demo.pong`。
- Android 端开启 notify 后显示 `sdk.demo.pong`。

示例消息：

```json
{"datatype":"sdk.demo.ping","data":"hello from android"}
{"datatype":"sdk.demo.pong","data":"ack:hello from android"}
```

眼镜端编译：

```bash
cd examples/ble_demo
make
```

Android 端编译：

```bash
cd examples/ble_demo/android
bash build_android.sh
```

该 demo datatype 只用于 SDK 示例，不触发相机、串流、显示或固定播报业务。

## 7. 故障排查

### 7.1 `ai_ble_client_start()` 失败

检查：
- `bt_service` 是否已启动
- `/var/run/ai_ble.sock` 是否存在
- 当前进程是否能访问该 socket

### 7.2 收不到消息

检查：
- 是否已成功注册对应 `datatype`
- 手机发来的 JSON 是否包含 `datatype` 和 `data`
- 手机发来的 UTF-8 JSON 总长度是否小于等于 `180`

### 7.3 发送失败

检查：
- `datatype` 是否符合命名规则
- `data` 是否为合法 UTF-8
- 编码后的整包是否超过 `180` 字节
- 手机侧是否已开启 notify
