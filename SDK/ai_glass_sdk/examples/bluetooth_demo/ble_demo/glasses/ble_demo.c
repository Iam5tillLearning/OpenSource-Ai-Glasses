#include "ai_ble.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define BLE_DEMO_PING_DATATYPE "sdk.demo.ping"
#define BLE_DEMO_PONG_DATATYPE "sdk.demo.pong"

static volatile sig_atomic_t g_running = 1;

static long long local_time_ms(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000LL + (long long)tv.tv_usec / 1000LL;
}

static int parse_latency_data(const char *data, int *seq, long long *client_send_ms)
{
    const char *p;
    char *end;
    long parsed_seq;
    long long parsed_client_send_ms;

    if (!data)
        return 0;

    p = data;
    if (strncmp(p, "ack:", 4) == 0)
        p += 4;
    if (strncmp(p, "lat|", 4) != 0)
        return 0;

    p += 4;
    parsed_seq = strtol(p, &end, 10);
    if (end == p || *end != '|')
        return 0;

    p = end + 1;
    parsed_client_send_ms = strtoll(p, &end, 10);
    if (end == p)
        return 0;

    if (seq)
        *seq = (int)parsed_seq;
    if (client_send_ms)
        *client_send_ms = parsed_client_send_ms;
    return 1;
}

static void print_latency_trace(const char *stage, const char *data, long long local_ms,
                                long long elapsed_ms, int result)
{
    int seq = -1;
    long long client_send_ms = -1;

    if (!parse_latency_data(data, &seq, &client_send_ms))
        return;

    printf("[BLE_DEMO][TRACE] stage=%s local_ms=%lld seq=%d client_send_ms=%lld",
           stage, local_ms, seq, client_send_ms);
    if (elapsed_ms >= 0)
        printf(" elapsed_ms=%lld", elapsed_ms);
    if (result != 0)
        printf(" result=%d", result);
    printf("\n");
}

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
    int send_ret;
    long long rx_ms;
    long long send_start_ms;
    long long send_end_ms;

    if (!client || !datatype || !data) {
        return;
    }

    rx_ms = local_time_ms();
    print_latency_trace("ble_demo_rx_callback", data, rx_ms, -1, 0);
    printf("[BLE_DEMO] recv datatype=%s data=%s\n", datatype, data);
    fflush(stdout);

    written = snprintf(reply, sizeof(reply), "ack:%s", data);
    if (written < 0 || (size_t)written >= sizeof(reply)) {
        printf("[BLE_DEMO] reply data too large, skip response\n");
        fflush(stdout);
        return;
    }

    send_start_ms = local_time_ms();
    print_latency_trace("ble_demo_send_start", reply, send_start_ms, send_start_ms - rx_ms, 0);
    send_ret = ai_ble_send(client, BLE_DEMO_PONG_DATATYPE, reply);
    send_end_ms = local_time_ms();
    print_latency_trace("ble_demo_send_return", reply, send_end_ms,
                        send_end_ms - send_start_ms, send_ret);

    if (send_ret == 0) {
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
