#include "ai_ble.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BLE_DEMO_PING_DATATYPE "sdk.demo.ping"
#define BLE_DEMO_PONG_DATATYPE "sdk.demo.pong"

static volatile sig_atomic_t g_running = 1;

static void handle_signal(int sig)
{
    (void)sig;
    g_running = 0;
}

static void on_ble_ping(const char *datatype, const char *data, void *user_data)
{
    ai_ble_client_t *client = (ai_ble_client_t *)user_data;
    char reply[AI_BLE_MAX_DATA_LEN + 1];
    int written;

    if (!client || !datatype || !data) {
        return;
    }

    printf("[BLE_DEMO] recv datatype=%s data=%s\n", datatype, data);
    fflush(stdout);

    written = snprintf(reply, sizeof(reply), "ack:%s", data);
    if (written < 0 || (size_t)written >= sizeof(reply)) {
        printf("[BLE_DEMO] reply data too large, skip response\n");
        fflush(stdout);
        return;
    }

    if (ai_ble_send(client, BLE_DEMO_PONG_DATATYPE, reply) == 0) {
        printf("[BLE_DEMO] sent datatype=%s data=%s\n", BLE_DEMO_PONG_DATATYPE, reply);
    } else {
        printf("[BLE_DEMO] send failed datatype=%s data=%s\n", BLE_DEMO_PONG_DATATYPE, reply);
    }
    fflush(stdout);
}

int main(void)
{
    ai_ble_client_t *client;

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    client = ai_ble_client_create();
    if (!client) {
        printf("[BLE_DEMO] failed to create BLE client\n");
        return 1;
    }

    if (ai_ble_client_start(client) != 0) {
        printf("[BLE_DEMO] failed to start BLE client\n");
        ai_ble_client_destroy(client);
        return 1;
    }

    if (ai_ble_register_datatype(client, BLE_DEMO_PING_DATATYPE, on_ble_ping, client) != 0) {
        printf("[BLE_DEMO] failed to register datatype=%s\n", BLE_DEMO_PING_DATATYPE);
        ai_ble_client_destroy(client);
        return 1;
    }

    printf("[BLE_DEMO] running\n");
    printf("[BLE_DEMO] subscribed datatype=%s\n", BLE_DEMO_PING_DATATYPE);
    printf("[BLE_DEMO] reply datatype=%s\n", BLE_DEMO_PONG_DATATYPE);
    printf("[BLE_DEMO] press Ctrl+C to exit\n");
    fflush(stdout);

    while (g_running) {
        sleep(1);
    }

    ai_ble_unregister_datatype(client, BLE_DEMO_PING_DATATYPE);
    ai_ble_client_destroy(client);
    printf("[BLE_DEMO] stopped\n");
    return 0;
}
