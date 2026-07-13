#include "ai_spp.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define BENCH_DEFAULT_PAYLOAD_SIZE 204800U
#define BENCH_MAX_PAYLOAD_SIZE (1024U * 1024U)
#define BENCH_LINE_MAX 512
#define BENCH_HEADER_MAX 256
#define BENCH_REQUEST_ID_MAX 64
#define BENCH_HOST_MAX 64
#define BENCH_RW_TIMEOUT_MS 15000
#define BENCH_CONNECT_TIMEOUT_MS 8000
#define BENCH_SPP_WRITE_CHUNK_SIZE 256U
#define BENCH_SPP_CHUNK_INTERVAL_US 1000U

typedef enum {
    BENCH_CHANNEL_SPP = 0,
    BENCH_CHANNEL_WIFI = 1
} bench_channel_t;

typedef struct {
    bench_channel_t channel;
    char request_id[BENCH_REQUEST_ID_MAX];
    char host[BENCH_HOST_MAX];
    unsigned short port;
    size_t size;
} bench_command_t;

typedef struct {
    int ok;
    char request_id[BENCH_REQUEST_ID_MAX];
    char status[16];
    size_t bytes;
    unsigned int crc32;
} bench_ack_t;

typedef struct {
    unsigned char *data;
    size_t size;
    unsigned int crc32;
} payload_cache_t;

typedef struct {
    long long command_ms;
    long long payload_ready_ms;
    long long connect_ms;
    long long data_start_ms;
    long long data_end_ms;
    long long ack_end_ms;
} bench_metrics_t;

static volatile sig_atomic_t g_running = 1;
static payload_cache_t g_payload_cache;

static void handle_signal(int sig)
{
    (void)sig;
    g_running = 0;
}

static long long monotonic_ms(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        return 0;
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

static unsigned int crc32_update(unsigned int crc,
                                 const unsigned char *data,
                                 size_t size)
{
    size_t i;

    crc = ~crc;
    for (i = 0; i < size; i++) {
        unsigned int value = crc ^ data[i];
        int bit;
        for (bit = 0; bit < 8; bit++) {
            if (value & 1U)
                value = (value >> 1) ^ 0xEDB88320U;
            else
                value >>= 1;
        }
        crc = value;
    }
    return ~crc;
}

static unsigned int crc32_bytes(const unsigned char *data, size_t size)
{
    return crc32_update(0U, data, size);
}

static void fill_payload(unsigned char *buffer, size_t size)
{
    size_t i;

    if (!buffer || size == 0)
        return;

    for (i = 0; i < size; i++)
        buffer[i] = (unsigned char)(((i * 131U) + 17U) & 0xFFU);

    if (size >= 10) {
        buffer[0] = 0xFF;
        buffer[1] = 0xD8;
        buffer[2] = 0xFF;
        buffer[3] = 0xE0;
        buffer[4] = 0x00;
        buffer[5] = 0x10;
        buffer[6] = 'J';
        buffer[7] = 'F';
        buffer[8] = 'I';
        buffer[9] = 'F';
    }
    if (size >= 2) {
        buffer[size - 2] = 0xFF;
        buffer[size - 1] = 0xD9;
    }
}

static int ensure_payload_cache(size_t size)
{
    unsigned char *data;
    long long start_ms;
    long long end_ms;

    if (size == 0 || size > BENCH_MAX_PAYLOAD_SIZE) {
        errno = EINVAL;
        return -1;
    }

    if (g_payload_cache.data && g_payload_cache.size == size)
        return 0;

    data = (unsigned char *)malloc(size);
    if (!data)
        return -1;

    start_ms = monotonic_ms();
    fill_payload(data, size);
    end_ms = monotonic_ms();

    free(g_payload_cache.data);
    g_payload_cache.data = data;
    g_payload_cache.size = size;
    g_payload_cache.crc32 = crc32_bytes(data, size);

    printf("[TRANSPORT_BENCH] payload_cache_refreshed size=%zu crc32=%08x fill_ms=%lld\n",
           size, g_payload_cache.crc32, end_ms - start_ms);
    fflush(stdout);
    return 0;
}

static int wait_fd_ready(int fd, short events, int timeout_ms)
{
    struct pollfd pfd;
    int ret;

    if (fd < 0) {
        errno = EBADF;
        return -1;
    }

    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = fd;
    pfd.events = events | POLLERR | POLLHUP | POLLNVAL;

    do {
        ret = poll(&pfd, 1, timeout_ms);
    } while (ret < 0 && errno == EINTR);

    if (ret == 0) {
        errno = ETIMEDOUT;
        return -1;
    }
    if (ret < 0)
        return -1;
    if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
        errno = EPIPE;
        return -1;
    }
    if ((pfd.revents & events) == 0) {
        errno = EIO;
        return -1;
    }
    return 0;
}

static ssize_t spp_read_line(ai_spp_connection_t *connection,
                             char *buffer,
                             size_t capacity)
{
    size_t used = 0;

    if (!connection || !buffer || capacity < 2) {
        errno = EINVAL;
        return -1;
    }

    while (used + 1 < capacity) {
        char ch;
        ssize_t n;

        if (wait_fd_ready(connection->fd, POLLIN, BENCH_RW_TIMEOUT_MS) != 0)
            return -1;

        n = ai_spp_read(connection, &ch, 1);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (n == 0) {
            errno = ECONNRESET;
            return -1;
        }
        if (ch == '\r')
            continue;

        buffer[used++] = ch;
        if (ch == '\n') {
            buffer[used] = '\0';
            return (ssize_t)used;
        }
    }

    errno = EMSGSIZE;
    return -1;
}

static ssize_t tcp_read_line(int fd, char *buffer, size_t capacity)
{
    size_t used = 0;

    if (fd < 0 || !buffer || capacity < 2) {
        errno = EINVAL;
        return -1;
    }

    while (used + 1 < capacity) {
        char ch;
        ssize_t n;

        if (wait_fd_ready(fd, POLLIN, BENCH_RW_TIMEOUT_MS) != 0)
            return -1;

        n = recv(fd, &ch, 1, 0);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (n == 0) {
            errno = ECONNRESET;
            return -1;
        }
        if (ch == '\r')
            continue;

        buffer[used++] = ch;
        if (ch == '\n') {
            buffer[used] = '\0';
            return (ssize_t)used;
        }
    }

    errno = EMSGSIZE;
    return -1;
}

static int spp_write_all(ai_spp_connection_t *connection,
                         const void *buffer,
                         size_t size)
{
    const unsigned char *ptr = (const unsigned char *)buffer;
    size_t sent = 0;

    while (sent < size) {
        ssize_t n;
        size_t chunk_size = size - sent;

        if (wait_fd_ready(connection->fd, POLLOUT, BENCH_RW_TIMEOUT_MS) != 0)
            return -1;

        if (chunk_size > BENCH_SPP_WRITE_CHUNK_SIZE)
            chunk_size = BENCH_SPP_WRITE_CHUNK_SIZE;

        n = ai_spp_write(connection, ptr + sent, chunk_size);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (n == 0) {
            errno = EIO;
            return -1;
        }
        sent += (size_t)n;
        if (sent < size && BENCH_SPP_CHUNK_INTERVAL_US > 0)
            usleep(BENCH_SPP_CHUNK_INTERVAL_US);
    }
    return 0;
}

static int tcp_write_all(int fd, const void *buffer, size_t size)
{
    const unsigned char *ptr = (const unsigned char *)buffer;
    size_t sent = 0;

    while (sent < size) {
        ssize_t n;

        if (wait_fd_ready(fd, POLLOUT, BENCH_RW_TIMEOUT_MS) != 0)
            return -1;

        n = send(fd, ptr + sent, size - sent, 0);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (n == 0) {
            errno = EIO;
            return -1;
        }
        sent += (size_t)n;
    }
    return 0;
}

static int sanitize_request_id(const char *src, char *dst, size_t dst_size)
{
    size_t used = 0;
    size_t i;

    if (!src || !dst || dst_size == 0)
        return -1;

    for (i = 0; src[i] != '\0' && src[i] != '\n' && used + 1 < dst_size; i++) {
        unsigned char ch = (unsigned char)src[i];
        if (isalnum(ch) || ch == '_' || ch == '-' || ch == '.')
            dst[used++] = (char)ch;
    }

    dst[used] = '\0';
    return used > 0 ? 0 : -1;
}

static int parse_size_value(const char *value, size_t *out_size)
{
    unsigned long parsed;
    char *end = NULL;

    if (!value || !out_size)
        return -1;

    errno = 0;
    parsed = strtoul(value, &end, 10);
    if (errno != 0 || !end || *end != '\0')
        return -1;
    if (parsed == 0UL || parsed > BENCH_MAX_PAYLOAD_SIZE)
        return -1;

    *out_size = (size_t)parsed;
    return 0;
}

static int parse_port_value(const char *value, unsigned short *out_port)
{
    unsigned long parsed;
    char *end = NULL;

    if (!value || !out_port)
        return -1;

    errno = 0;
    parsed = strtoul(value, &end, 10);
    if (errno != 0 || !end || *end != '\0')
        return -1;
    if (parsed == 0UL || parsed > 65535UL)
        return -1;

    *out_port = (unsigned short)parsed;
    return 0;
}

static int parse_command_line(const char *line, bench_command_t *command)
{
    char copy[BENCH_LINE_MAX];
    char *saveptr = NULL;
    char *token;
    int saw_request = 0;

    if (!line || !command)
        return -1;

    memset(command, 0, sizeof(*command));
    command->size = BENCH_DEFAULT_PAYLOAD_SIZE;

    snprintf(copy, sizeof(copy), "%s", line);
    token = strtok_r(copy, " \t\r\n", &saveptr);
    if (!token)
        return -1;

    if (strcmp(token, "SPP_SEND") == 0) {
        command->channel = BENCH_CHANNEL_SPP;
    } else if (strcmp(token, "WIFI_SEND") == 0) {
        command->channel = BENCH_CHANNEL_WIFI;
    } else {
        return -1;
    }

    while ((token = strtok_r(NULL, " \t\r\n", &saveptr)) != NULL) {
        if (strncmp(token, "request=", 8) == 0) {
            if (sanitize_request_id(token + 8,
                                    command->request_id,
                                    sizeof(command->request_id)) != 0) {
                return -1;
            }
            saw_request = 1;
        } else if (strncmp(token, "size=", 5) == 0) {
            if (parse_size_value(token + 5, &command->size) != 0)
                return -1;
        } else if (strncmp(token, "host=", 5) == 0) {
            snprintf(command->host, sizeof(command->host), "%s", token + 5);
        } else if (strncmp(token, "port=", 5) == 0) {
            if (parse_port_value(token + 5, &command->port) != 0)
                return -1;
        }
    }

    if (!saw_request)
        return -1;

    if (command->channel == BENCH_CHANNEL_WIFI) {
        struct in_addr addr;

        if (command->host[0] == '\0' || command->port == 0)
            return -1;
        if (inet_pton(AF_INET, command->host, &addr) != 1)
            return -1;
    }

    return 0;
}

static int parse_ack_line(const char *line, bench_ack_t *ack)
{
    char copy[BENCH_LINE_MAX];
    char *saveptr = NULL;
    char *token;
    int saw_request = 0;
    int saw_status = 0;
    int saw_bytes = 0;
    int saw_crc32 = 0;

    if (!line || !ack)
        return -1;

    memset(ack, 0, sizeof(*ack));
    snprintf(copy, sizeof(copy), "%s", line);

    token = strtok_r(copy, " \t\r\n", &saveptr);
    if (!token || strcmp(token, "OSAIG_BENCH_ACK") != 0)
        return -1;

    while ((token = strtok_r(NULL, " \t\r\n", &saveptr)) != NULL) {
        if (strncmp(token, "request=", 8) == 0) {
            if (sanitize_request_id(token + 8,
                                    ack->request_id,
                                    sizeof(ack->request_id)) != 0) {
                return -1;
            }
            saw_request = 1;
        } else if (strncmp(token, "status=", 7) == 0) {
            snprintf(ack->status, sizeof(ack->status), "%s", token + 7);
            saw_status = 1;
        } else if (strncmp(token, "bytes=", 6) == 0) {
            if (parse_size_value(token + 6, &ack->bytes) != 0)
                return -1;
            saw_bytes = 1;
        } else if (strncmp(token, "crc32=", 6) == 0) {
            char *end = NULL;
            errno = 0;
            ack->crc32 = (unsigned int)strtoul(token + 6, &end, 16);
            if (errno != 0 || !end || *end != '\0')
                return -1;
            saw_crc32 = 1;
        }
    }

    ack->ok = saw_request && saw_status && saw_bytes && saw_crc32 &&
              strcmp(ack->status, "ok") == 0;
    return ack->ok ? 0 : -1;
}

static void log_command(const bench_command_t *command)
{
    printf("[TRANSPORT_BENCH] command channel=%s request=%s size=%zu",
           command->channel == BENCH_CHANNEL_SPP ? "spp" : "wifi",
           command->request_id, command->size);
    if (command->channel == BENCH_CHANNEL_WIFI)
        printf(" host=%s port=%u", command->host, command->port);
    printf("\n");
    fflush(stdout);
}

static int send_done_line(ai_spp_connection_t *connection,
                          const bench_command_t *command,
                          const bench_metrics_t *metrics,
                          const char *status,
                          const char *reason)
{
    char line[BENCH_LINE_MAX];
    int written;
    size_t bytes = command->size;
    unsigned int crc32 = g_payload_cache.size == command->size
                         ? g_payload_cache.crc32
                         : 0U;
    long long total_ms = metrics->ack_end_ms - metrics->command_ms;
    long long prep_ms = metrics->payload_ready_ms - metrics->command_ms;
    long long data_ms = metrics->data_end_ms - metrics->data_start_ms;
    long long ack_ms = metrics->ack_end_ms - metrics->data_end_ms;

    if (!status)
        status = "error";
    if (!reason)
        reason = "na";

    written = snprintf(
        line, sizeof(line),
        "OSAIG_BENCH_DONE request=%s channel=%s status=%s total_ms=%lld prep_ms=%lld "
        "connect_ms=%lld data_ms=%lld ack_ms=%lld bytes=%zu crc32=%08x reason=%s\n",
        command->request_id,
        command->channel == BENCH_CHANNEL_SPP ? "spp" : "wifi",
        status,
        total_ms,
        prep_ms,
        metrics->connect_ms,
        data_ms,
        ack_ms,
        bytes,
        crc32,
        reason);
    if (written < 0 || (size_t)written >= sizeof(line)) {
        errno = EMSGSIZE;
        return -1;
    }

    return spp_write_all(connection, line, (size_t)written);
}

static int verify_ack(const bench_command_t *command, const bench_ack_t *ack)
{
    if (!ack->ok)
        return -1;
    if (strcmp(command->request_id, ack->request_id) != 0)
        return -1;
    if (ack->bytes != command->size)
        return -1;
    if (ack->crc32 != g_payload_cache.crc32)
        return -1;
    return 0;
}

static int create_tcp_client(const char *host, unsigned short port, long long *connect_ms)
{
    struct sockaddr_in addr;
    int fd = -1;
    int flags;
    int one = 1;
    int ret;
    socklen_t err_len;
    int so_error = 0;
    struct pollfd pfd;
    long long start_ms = monotonic_ms();
    long long end_ms;

    if (connect_ms)
        *connect_ms = 0;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;

    (void)setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        goto fail;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0)
        goto fail;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        errno = EINVAL;
        goto fail;
    }

    ret = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (ret == 0)
        goto connected;
    if (ret < 0 && errno != EINPROGRESS)
        goto fail;

    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = fd;
    pfd.events = POLLOUT | POLLERR | POLLHUP | POLLNVAL;
    do {
        ret = poll(&pfd, 1, BENCH_CONNECT_TIMEOUT_MS);
    } while (ret < 0 && errno == EINTR);
    if (ret == 0) {
        errno = ETIMEDOUT;
        goto fail;
    }
    if (ret < 0)
        goto fail;
    if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
        errno = EPIPE;
        goto fail;
    }

    err_len = sizeof(so_error);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &err_len) != 0)
        goto fail;
    if (so_error != 0) {
        errno = so_error;
        goto fail;
    }

connected:
    if (fcntl(fd, F_SETFL, flags) != 0)
        goto fail;
    end_ms = monotonic_ms();
    if (connect_ms)
        *connect_ms = end_ms - start_ms;
    return fd;

fail:
    if (fd >= 0)
        close(fd);
    return -1;
}

static int send_header_and_payload_spp(ai_spp_connection_t *connection,
                                       const bench_command_t *command)
{
    char header[BENCH_HEADER_MAX];
    int written = snprintf(header, sizeof(header),
                           "OSAIG_BENCH_V1 channel=spp request=%s size=%zu crc32=%08x\n",
                           command->request_id,
                           command->size,
                           g_payload_cache.crc32);
    if (written < 0 || (size_t)written >= sizeof(header)) {
        errno = EMSGSIZE;
        return -1;
    }
    if (spp_write_all(connection, header, (size_t)written) != 0)
        return -1;
    if (spp_write_all(connection, g_payload_cache.data, command->size) != 0)
        return -1;
    return 0;
}

static int send_header_and_payload_tcp(int fd, const bench_command_t *command)
{
    char header[BENCH_HEADER_MAX];
    int written = snprintf(header, sizeof(header),
                           "OSAIG_BENCH_V1 channel=wifi request=%s size=%zu crc32=%08x\n",
                           command->request_id,
                           command->size,
                           g_payload_cache.crc32);
    if (written < 0 || (size_t)written >= sizeof(header)) {
        errno = EMSGSIZE;
        return -1;
    }
    if (tcp_write_all(fd, header, (size_t)written) != 0)
        return -1;
    if (tcp_write_all(fd, g_payload_cache.data, command->size) != 0)
        return -1;
    return 0;
}

static int handle_spp_transfer(ai_spp_connection_t *connection,
                               const bench_command_t *command,
                               bench_metrics_t *metrics)
{
    char ack_line[BENCH_LINE_MAX];
    bench_ack_t ack;

    metrics->data_start_ms = monotonic_ms();
    if (send_header_and_payload_spp(connection, command) != 0)
        return -1;
    metrics->data_end_ms = monotonic_ms();

    if (spp_read_line(connection, ack_line, sizeof(ack_line)) < 0)
        return -1;
    metrics->ack_end_ms = monotonic_ms();

    if (parse_ack_line(ack_line, &ack) != 0)
        return -1;
    if (verify_ack(command, &ack) != 0) {
        errno = EPROTO;
        return -1;
    }
    return 0;
}

static int handle_wifi_transfer(ai_spp_connection_t *connection,
                                const bench_command_t *command,
                                bench_metrics_t *metrics)
{
    char ack_line[BENCH_LINE_MAX];
    bench_ack_t ack;
    int tcp_fd;

    (void)connection;

    tcp_fd = create_tcp_client(command->host, command->port, &metrics->connect_ms);
    if (tcp_fd < 0)
        return -1;

    metrics->data_start_ms = monotonic_ms();
    if (send_header_and_payload_tcp(tcp_fd, command) != 0) {
        close(tcp_fd);
        return -1;
    }
    metrics->data_end_ms = monotonic_ms();

    if (tcp_read_line(tcp_fd, ack_line, sizeof(ack_line)) < 0) {
        close(tcp_fd);
        return -1;
    }
    metrics->ack_end_ms = monotonic_ms();
    close(tcp_fd);

    if (parse_ack_line(ack_line, &ack) != 0)
        return -1;
    if (verify_ack(command, &ack) != 0) {
        errno = EPROTO;
        return -1;
    }
    return 0;
}

static int process_command(ai_spp_connection_t *connection, const char *line)
{
    bench_command_t command;
    bench_metrics_t metrics;
    const char *reason = "ok";
    int ret = -1;
    int saved_errno = 0;

    if (parse_command_line(line, &command) != 0) {
        printf("[TRANSPORT_BENCH] invalid_command line=%s", line ? line : "(null)\n");
        fflush(stdout);
        return -1;
    }

    memset(&metrics, 0, sizeof(metrics));
    metrics.command_ms = monotonic_ms();
    log_command(&command);

    if (ensure_payload_cache(command.size) != 0) {
        reason = "payload_alloc";
        goto done;
    }
    metrics.payload_ready_ms = monotonic_ms();

    if (command.channel == BENCH_CHANNEL_SPP)
        ret = handle_spp_transfer(connection, &command, &metrics);
    else
        ret = handle_wifi_transfer(connection, &command, &metrics);

    if (ret == 0) {
        reason = "ok";
        goto done;
    }

    saved_errno = errno;
    if (saved_errno == ETIMEDOUT)
        reason = "timeout";
    else if (saved_errno == EPROTO)
        reason = "bad_ack";
    else if (saved_errno == EPIPE || saved_errno == ECONNRESET)
        reason = "peer_closed";
    else
        reason = "io_error";

done:
    if (metrics.payload_ready_ms == 0)
        metrics.payload_ready_ms = monotonic_ms();
    if (metrics.data_start_ms == 0)
        metrics.data_start_ms = metrics.payload_ready_ms;
    if (metrics.data_end_ms == 0)
        metrics.data_end_ms = monotonic_ms();
    if (metrics.ack_end_ms == 0)
        metrics.ack_end_ms = monotonic_ms();

    if (send_done_line(connection,
                       &command,
                       &metrics,
                       ret == 0 ? "ok" : "error",
                       reason) != 0) {
        printf("[TRANSPORT_BENCH] send_done_failed request=%s errno=%d\n",
               command.request_id, errno);
    }

    printf("[TRANSPORT_BENCH] result channel=%s request=%s status=%s total_ms=%lld prep_ms=%lld "
           "connect_ms=%lld data_ms=%lld ack_ms=%lld bytes=%zu crc32=%08x reason=%s\n",
           command.channel == BENCH_CHANNEL_SPP ? "spp" : "wifi",
           command.request_id,
           ret == 0 ? "ok" : "error",
           metrics.ack_end_ms - metrics.command_ms,
           metrics.payload_ready_ms - metrics.command_ms,
           metrics.connect_ms,
           metrics.data_end_ms - metrics.data_start_ms,
           metrics.ack_end_ms - metrics.data_end_ms,
           command.size,
           g_payload_cache.size == command.size ? g_payload_cache.crc32 : 0U,
           reason);
    if (command.channel == BENCH_CHANNEL_SPP &&
        (metrics.ack_end_ms - metrics.command_ms) > 100) {
        printf("[TRANSPORT_BENCH][WARN] spp_total_ms_exceeded threshold_ms=100 actual_ms=%lld "
               "request=%s connect_ms=%lld data_ms=%lld ack_ms=%lld\n",
               metrics.ack_end_ms - metrics.command_ms,
               command.request_id,
               metrics.connect_ms,
               metrics.data_end_ms - metrics.data_start_ms,
               metrics.ack_end_ms - metrics.data_end_ms);
    }
    fflush(stdout);

    errno = saved_errno;
    return ret;
}

static void handle_connection(ai_spp_connection_t *connection)
{
    printf("[TRANSPORT_BENCH] connected remote=%s fd=%d\n",
           connection->remote_addr, connection->fd);
    fflush(stdout);

    while (g_running) {
        char line[BENCH_LINE_MAX];
        ssize_t len = spp_read_line(connection, line, sizeof(line));
        if (len < 0) {
            if (errno != ETIMEDOUT) {
                printf("[TRANSPORT_BENCH] connection_read_end remote=%s errno=%d\n",
                       connection->remote_addr, errno);
                fflush(stdout);
                break;
            }
            continue;
        }

        printf("[TRANSPORT_BENCH] control_rx len=%zd text=%s", len, line);
        fflush(stdout);
        (void)process_command(connection, line);
    }

    ai_spp_close(connection);
    printf("[TRANSPORT_BENCH] disconnected\n");
    fflush(stdout);
}

int main(void)
{
    ai_spp_client_t *client;

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    client = ai_spp_client_create();
    if (!client) {
        printf("[TRANSPORT_BENCH] create_client_failed\n");
        return 1;
    }

    if (ensure_payload_cache(BENCH_DEFAULT_PAYLOAD_SIZE) != 0) {
        printf("[TRANSPORT_BENCH] initial_payload_prepare_failed errno=%d\n", errno);
        ai_spp_client_destroy(client);
        return 1;
    }

    if (ai_spp_client_start(client, NULL, NULL) != 0) {
        printf("[TRANSPORT_BENCH] start_client_failed socket=%s\n", AI_SPP_SOCKET_PATH);
        ai_spp_client_destroy(client);
        return 1;
    }

    printf("[TRANSPORT_BENCH] waiting_for_spp_connection default_size=%u crc32=%08x\n",
           BENCH_DEFAULT_PAYLOAD_SIZE, g_payload_cache.crc32);
    fflush(stdout);

    while (g_running) {
        ai_spp_connection_t connection;

        if (ai_spp_accept(client, &connection, 1000) == 0) {
            handle_connection(&connection);
            printf("[TRANSPORT_BENCH] waiting_for_next_connection\n");
            fflush(stdout);
        }
    }

    ai_spp_client_destroy(client);
    free(g_payload_cache.data);
    memset(&g_payload_cache, 0, sizeof(g_payload_cache));
    printf("[TRANSPORT_BENCH] stopped\n");
    return 0;
}
