#include "ai_spp.h"

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define SPP_SDK_DEMO_TX "OSAIG_SPP_SDK_TX hello from sdk demo\n"

static volatile sig_atomic_t g_running = 1;

static long long local_time_ms(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000LL + (long long)tv.tv_usec / 1000LL;
}

static int parse_latency_line(const unsigned char *data, ssize_t len,
                              int *seq, long long *client_send_ms)
{
    char text[256];
    size_t copy_len;
    char *seq_ptr;
    char *time_ptr;

    if (!data || len <= 0)
        return 0;

    copy_len = (size_t)len;
    if (copy_len >= sizeof(text))
        copy_len = sizeof(text) - 1;
    memcpy(text, data, copy_len);
    text[copy_len] = '\0';

    if (strstr(text, "OSAIG_LATENCY ") == NULL)
        return 0;

    seq_ptr = strstr(text, "seq=");
    time_ptr = strstr(text, "t=");
    if (!seq_ptr || !time_ptr)
        return 0;

    if (seq)
        *seq = atoi(seq_ptr + 4);
    if (client_send_ms)
        *client_send_ms = strtoll(time_ptr + 2, NULL, 10);
    return 1;
}

static void print_latency_trace(const char *stage, const unsigned char *data, ssize_t len,
                                long long local_ms, long long elapsed_ms, int result)
{
    int seq = -1;
    long long client_send_ms = -1;

    if (!parse_latency_line(data, len, &seq, &client_send_ms))
        return;

    printf("[SPP_SDK_DEMO][TRACE] stage=%s local_ms=%lld seq=%d client_send_ms=%lld",
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

static void log_ascii(const unsigned char *data, ssize_t len)
{
    ssize_t i;

    for (i = 0; i < len; i++) {
        unsigned char ch = data[i];
        putchar((ch >= 32 && ch <= 126) ? ch : '.');
    }
}

static void handle_connection(ai_spp_connection_t *connection)
{
    int i;

    printf("[SPP_SDK_DEMO] connected remote=%s fd=%d\n",
           connection->remote_addr, connection->fd);
    fflush(stdout);

    if (ai_spp_write(connection, SPP_SDK_DEMO_TX,
                     strlen(SPP_SDK_DEMO_TX)) < 0) {
        printf("[SPP_SDK_DEMO] initial write failed errno=%d\n", errno);
        fflush(stdout);
    }

    for (i = 0; i < 60 && g_running; i++) {
        struct pollfd pfd;
        int ret;

        memset(&pfd, 0, sizeof(pfd));
        pfd.fd = connection->fd;
        pfd.events = POLLIN | POLLERR | POLLHUP;

        ret = poll(&pfd, 1, 1000);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            printf("[SPP_SDK_DEMO] poll failed errno=%d\n", errno);
            break;
        }
        if (ret == 0)
            continue;

        if (pfd.revents & POLLIN) {
            unsigned char buffer[512];
            long long read_ms;
            long long echo_start_ms;
            long long echo_end_ms;
            int echo_ret;
            ssize_t len = ai_spp_read(connection, buffer, sizeof(buffer));
            if (len <= 0) {
                printf("[SPP_SDK_DEMO] read end len=%zd errno=%d\n",
                       len, len < 0 ? errno : 0);
                break;
            }

            read_ms = local_time_ms();
            print_latency_trace("spp_demo_read", buffer, len, read_ms, -1, 0);
            printf("[SPP_SDK_DEMO] RX len=%zd ascii=\"", len);
            log_ascii(buffer, len);
            printf("\"\n");
            fflush(stdout);

            echo_start_ms = local_time_ms();
            print_latency_trace("spp_demo_echo_start", buffer, len,
                                echo_start_ms, echo_start_ms - read_ms, 0);
            echo_ret = ai_spp_write(connection, buffer, (size_t)len);
            echo_end_ms = local_time_ms();
            print_latency_trace("spp_demo_echo_return", buffer, len,
                                echo_end_ms, echo_end_ms - echo_start_ms, echo_ret);
            if (echo_ret < 0) {
                printf("[SPP_SDK_DEMO] echo failed errno=%d\n", errno);
                break;
            }
        }

        if (pfd.revents & (POLLERR | POLLHUP)) {
            printf("[SPP_SDK_DEMO] remote disconnected revents=0x%x\n",
                   pfd.revents);
            break;
        }
    }

    ai_spp_close(connection);
    printf("[SPP_SDK_DEMO] connection closed\n");
    fflush(stdout);
}

int main(void)
{
    ai_spp_client_t *client;

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    client = ai_spp_client_create();
    if (!client) {
        printf("[SPP_SDK_DEMO] create client failed\n");
        return 1;
    }

    if (ai_spp_client_start(client, NULL, NULL) != 0) {
        printf("[SPP_SDK_DEMO] start client failed, is bt_service SPP broker running?\n");
        ai_spp_client_destroy(client);
        return 1;
    }

    printf("[SPP_SDK_DEMO] waiting for SPP connection\n");
    fflush(stdout);

    while (g_running) {
        ai_spp_connection_t connection;
        if (ai_spp_accept(client, &connection, 1000) == 0) {
            handle_connection(&connection);
            printf("[SPP_SDK_DEMO] waiting for next SPP connection\n");
            fflush(stdout);
        }
    }

    ai_spp_client_destroy(client);
    printf("[SPP_SDK_DEMO] stopped\n");
    return 0;
}
