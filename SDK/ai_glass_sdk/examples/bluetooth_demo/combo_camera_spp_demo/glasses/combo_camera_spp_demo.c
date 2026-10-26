#include "ai_ble.h"
#include "ai_camera.h"
#include "ai_spp.h"

#include <ctype.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define COMBO_BLE_CMD_DATATYPE "combo.camera.takephoto"
#define COMBO_BLE_STATUS_DATATYPE "combo.camera.status"
#define COMBO_CAMERA_TIMEOUT_MS 15000
#define COMBO_REQUEST_ID_MAX 64
#define COMBO_DETAIL_MAX 96
#define COMBO_SPP_HEADER_MAX 256
#define COMBO_FILE_CHUNK_SIZE 256
#define COMBO_SPP_WRITE_TIMEOUT_MS 10000
#define COMBO_SPP_CHUNK_INTERVAL_US 5000
#define COMBO_SPP_PROGRESS_PERCENT_STEP 5
#define COMBO_SPP_PROGRESS_INTERVAL_MS 1000

typedef struct {
    pthread_mutex_t lock;
    ai_ble_client_t *ble_client;
    unsigned int generated_seq;
    int pending;
    int busy;
    int wait_spp_reported;
    char pending_request[COMBO_REQUEST_ID_MAX];
} combo_state_t;

static volatile sig_atomic_t g_running = 1;

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

static void sanitize_token(const char *input, char *output, size_t output_size)
{
    size_t used = 0;
    size_t i;

    if (!output || output_size == 0)
        return;

    output[0] = '\0';
    if (!input)
        return;

    for (i = 0; input[i] != '\0' && used + 1 < output_size; i++) {
        unsigned char ch = (unsigned char)input[i];
        if (isalnum(ch) || ch == '_' || ch == '-' || ch == '.') {
            output[used++] = (char)ch;
        } else {
            output[used++] = '_';
        }
    }
    output[used] = '\0';
}

static const char *path_basename(const char *path)
{
    const char *slash;

    if (!path || path[0] == '\0')
        return "capture.jpg";

    slash = strrchr(path, '/');
    if (!slash || slash[1] == '\0')
        return path;
    return slash + 1;
}

static void send_ble_status(combo_state_t *state,
                            const char *status,
                            const char *request_id,
                            const char *detail)
{
    char data[AI_BLE_MAX_DATA_LEN + 1];
    int written;

    if (!state || !state->ble_client || !status || !request_id)
        return;

    if (detail && detail[0]) {
        written = snprintf(data, sizeof(data), "%s|%s|%s",
                           status, request_id, detail);
    } else {
        written = snprintf(data, sizeof(data), "%s|%s", status, request_id);
    }

    if (written < 0 || (size_t)written >= sizeof(data)) {
        printf("[COMBO_CAMERA_SPP] BLE status too long status=%s request=%s\n",
               status, request_id);
        fflush(stdout);
        return;
    }

    if (ai_ble_send(state->ble_client, COMBO_BLE_STATUS_DATATYPE, data) != 0) {
        printf("[COMBO_CAMERA_SPP] BLE status send failed data=%s\n", data);
    } else {
        printf("[COMBO_CAMERA_SPP] BLE status %s\n", data);
    }
    fflush(stdout);
}

static void build_request_id(combo_state_t *state,
                             const char *data,
                             char *request_id,
                             size_t request_id_size)
{
    if (data && data[0]) {
        sanitize_token(data, request_id, request_id_size);
        if (request_id[0] != '\0')
            return;
    }

    snprintf(request_id, request_id_size, "req_%u", ++state->generated_seq);
}

static void on_ble_takephoto(const char *datatype, const char *data, void *user_data)
{
    combo_state_t *state = (combo_state_t *)user_data;
    char request_id[COMBO_REQUEST_ID_MAX];
    int accepted = 0;

    if (!state || !datatype)
        return;

    pthread_mutex_lock(&state->lock);
    build_request_id(state, data, request_id, sizeof(request_id));
    if (!state->pending && !state->busy) {
        snprintf(state->pending_request, sizeof(state->pending_request),
                 "%s", request_id);
        state->pending = 1;
        state->wait_spp_reported = 0;
        accepted = 1;
    }
    pthread_mutex_unlock(&state->lock);

    printf("[COMBO_CAMERA_SPP] BLE command datatype=%s data=%s request=%s accepted=%d\n",
           datatype, data ? data : "", request_id, accepted);
    fflush(stdout);

    if (accepted) {
        send_ble_status(state, "queued", request_id, NULL);
    } else {
        send_ble_status(state, "busy", request_id, NULL);
    }
}

static int peek_wait_spp_request(combo_state_t *state,
                                 char *request_id,
                                 size_t request_id_size)
{
    int should_report = 0;

    pthread_mutex_lock(&state->lock);
    if (state->pending && !state->wait_spp_reported) {
        snprintf(request_id, request_id_size, "%s", state->pending_request);
        state->wait_spp_reported = 1;
        should_report = 1;
    }
    pthread_mutex_unlock(&state->lock);

    return should_report;
}

static int take_pending_request(combo_state_t *state,
                                char *request_id,
                                size_t request_id_size)
{
    int has_request = 0;

    pthread_mutex_lock(&state->lock);
    if (state->pending && !state->busy) {
        snprintf(request_id, request_id_size, "%s", state->pending_request);
        state->pending_request[0] = '\0';
        state->pending = 0;
        state->busy = 1;
        has_request = 1;
    }
    pthread_mutex_unlock(&state->lock);

    return has_request;
}

static void finish_request(combo_state_t *state)
{
    pthread_mutex_lock(&state->lock);
    state->busy = 0;
    pthread_mutex_unlock(&state->lock);
}

static int wait_spp_writable(ai_spp_connection_t *connection)
{
    struct pollfd pfd;
    int ret;

    if (!connection || connection->fd < 0) {
        errno = EBADF;
        return -1;
    }

    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = connection->fd;
    pfd.events = POLLOUT | POLLERR | POLLHUP | POLLNVAL;

    do {
        ret = poll(&pfd, 1, COMBO_SPP_WRITE_TIMEOUT_MS);
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
    if ((pfd.revents & POLLOUT) == 0) {
        errno = EIO;
        return -1;
    }

    return 0;
}

static int write_full(ai_spp_connection_t *connection,
                      const void *buffer,
                      size_t size)
{
    const unsigned char *p = (const unsigned char *)buffer;
    size_t sent = 0;

    while (sent < size) {
        ssize_t n = ai_spp_write(connection, p + sent, size - sent);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (wait_spp_writable(connection) == 0)
                    continue;
            }
            return -1;
        }
        if (n == 0)
            return -1;
        sent += (size_t)n;
    }

    return 0;
}

static int send_file_over_spp(combo_state_t *state,
                              ai_spp_connection_t *connection,
                              const char *request_id,
                              const char *photo_path)
{
    struct stat st;
    FILE *fp;
    char header[COMBO_SPP_HEADER_MAX];
    char filename[COMBO_REQUEST_ID_MAX];
    unsigned char buffer[COMBO_FILE_CHUNK_SIZE];
    size_t total_sent = 0;
    size_t file_size;
    int next_progress_percent = COMBO_SPP_PROGRESS_PERCENT_STEP;
    long long last_progress_ms = 0;
    int written;

    if (stat(photo_path, &st) != 0 || st.st_size <= 0) {
        printf("[COMBO_CAMERA_SPP] invalid photo path=%s errno=%d\n",
               photo_path, errno);
        send_ble_status(state, "err", request_id, "invalid_photo_file");
        return -1;
    }
    file_size = (size_t)st.st_size;

    sanitize_token(path_basename(photo_path), filename, sizeof(filename));
    if (filename[0] == '\0')
        snprintf(filename, sizeof(filename), "capture.jpg");

    written = snprintf(header, sizeof(header),
                       "OSAIG_JPG_V1 request=%s size=%lld name=%s\n",
                       request_id, (long long)st.st_size, filename);
    if (written < 0 || (size_t)written >= sizeof(header)) {
        send_ble_status(state, "err", request_id, "header_too_long");
        return -1;
    }

    fp = fopen(photo_path, "rb");
    if (!fp) {
        printf("[COMBO_CAMERA_SPP] fopen failed path=%s errno=%d\n",
               photo_path, errno);
        send_ble_status(state, "err", request_id, "open_photo_failed");
        return -1;
    }

    if (write_full(connection, header, strlen(header)) != 0) {
        printf("[COMBO_CAMERA_SPP] SPP header write failed request=%s errno=%d\n",
               request_id, errno);
        fflush(stdout);
        fclose(fp);
        send_ble_status(state, "err", request_id, "spp_header_failed");
        return -1;
    }

    printf("[COMBO_CAMERA_SPP] sending request=%s path=%s bytes=%zu\n",
           request_id, photo_path, file_size);
    fflush(stdout);

    while (!feof(fp)) {
        size_t nread = fread(buffer, 1, sizeof(buffer), fp);
        if (nread > 0) {
            if (write_full(connection, buffer, nread) != 0) {
                printf("[COMBO_CAMERA_SPP] SPP file write failed request=%s offset=%zu chunk=%zu errno=%d\n",
                       request_id, total_sent, nread, errno);
                fflush(stdout);
                fclose(fp);
                send_ble_status(state, "err", request_id, "spp_file_failed");
                return -1;
            }
            total_sent += nread;
            if (file_size > 0) {
                int percent = (int)((total_sent * 100U) / file_size);
                long long now_ms = monotonic_ms();
                if (percent >= next_progress_percent ||
                        now_ms - last_progress_ms >= COMBO_SPP_PROGRESS_INTERVAL_MS ||
                        total_sent == file_size) {
                    printf("[COMBO_CAMERA_SPP] send progress request=%s sent=%zu/%zu percent=%d\n",
                           request_id, total_sent, file_size, percent);
                    fflush(stdout);
                    while (next_progress_percent <= percent)
                        next_progress_percent += COMBO_SPP_PROGRESS_PERCENT_STEP;
                    last_progress_ms = now_ms;
                }
            }
            usleep(COMBO_SPP_CHUNK_INTERVAL_US);
        }
        if (ferror(fp)) {
            fclose(fp);
            send_ble_status(state, "err", request_id, "read_photo_failed");
            return -1;
        }
    }

    fclose(fp);

    if (total_sent != file_size) {
        send_ble_status(state, "err", request_id, "short_read");
        return -1;
    }

    snprintf(header, sizeof(header), "%s|%lld", filename, (long long)st.st_size);
    send_ble_status(state, "done", request_id, header);
    printf("[COMBO_CAMERA_SPP] sent request=%s path=%s bytes=%zu\n",
           request_id, photo_path, total_sent);
    fflush(stdout);
    return 0;
}

static int process_request(combo_state_t *state,
                           ai_spp_connection_t *connection,
                           const char *request_id)
{
    char photo_path[AI_CAMERA_ACTION_PATH_MAX];
    int ret;

    send_ble_status(state, "capturing", request_id, NULL);

    memset(photo_path, 0, sizeof(photo_path));
    ret = ai_camera_take_photo(photo_path, sizeof(photo_path),
                               COMBO_CAMERA_TIMEOUT_MS);
    if (ret != AI_MEDIA_SUCCESS) {
        char detail[COMBO_DETAIL_MAX];
        snprintf(detail, sizeof(detail), "take_photo_failed:%d", ret);
        printf("[COMBO_CAMERA_SPP] take_photo failed request=%s ret=%d\n",
               request_id, ret);
        fflush(stdout);
        send_ble_status(state, "err", request_id, detail);
        return 0;
    }

    printf("[COMBO_CAMERA_SPP] photo ready request=%s path=%s\n",
           request_id, photo_path);
    fflush(stdout);

    send_ble_status(state, "sending", request_id, path_basename(photo_path));
    return send_file_over_spp(state, connection, request_id, photo_path);
}

static int connection_has_closed(ai_spp_connection_t *connection)
{
    struct pollfd pfd;
    int ret;

    if (!connection || connection->fd < 0)
        return 1;

    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = connection->fd;
    pfd.events = POLLERR | POLLHUP | POLLNVAL;
    ret = poll(&pfd, 1, 0);
    if (ret <= 0)
        return 0;

    return (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0;
}

int main(void)
{
    combo_state_t state;
    ai_spp_client_t *spp_client;
    ai_spp_connection_t connection;
    int has_connection = 0;

    memset(&state, 0, sizeof(state));
    pthread_mutex_init(&state.lock, NULL);
    memset(&connection, 0, sizeof(connection));
    connection.fd = -1;
    connection.control_fd = -1;

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGPIPE, SIG_IGN);

    state.ble_client = ai_ble_client_create();
    if (!state.ble_client) {
        printf("[COMBO_CAMERA_SPP] failed to create BLE client\n");
        return 1;
    }

    if (ai_ble_client_start(state.ble_client) != 0) {
        printf("[COMBO_CAMERA_SPP] failed to start BLE client\n");
        ai_ble_client_destroy(state.ble_client);
        return 1;
    }

    if (ai_ble_register_datatype(state.ble_client, COMBO_BLE_CMD_DATATYPE,
                                 on_ble_takephoto, &state) != 0) {
        printf("[COMBO_CAMERA_SPP] failed to register BLE datatype=%s\n",
               COMBO_BLE_CMD_DATATYPE);
        ai_ble_client_destroy(state.ble_client);
        return 1;
    }

    spp_client = ai_spp_client_create();
    if (!spp_client) {
        printf("[COMBO_CAMERA_SPP] failed to create SPP client\n");
        ai_ble_client_destroy(state.ble_client);
        return 1;
    }

    if (ai_spp_client_start(spp_client, NULL, NULL) != 0) {
        printf("[COMBO_CAMERA_SPP] failed to start SPP client, is bt_service running?\n");
        ai_spp_client_destroy(spp_client);
        ai_ble_client_destroy(state.ble_client);
        return 1;
    }

    printf("[COMBO_CAMERA_SPP] running\n");
    printf("[COMBO_CAMERA_SPP] BLE command datatype=%s\n", COMBO_BLE_CMD_DATATYPE);
    printf("[COMBO_CAMERA_SPP] BLE status datatype=%s\n", COMBO_BLE_STATUS_DATATYPE);
    printf("[COMBO_CAMERA_SPP] waiting for SPP connection and BLE take-photo command\n");
    fflush(stdout);

    while (g_running) {
        char request_id[COMBO_REQUEST_ID_MAX];

        if (!has_connection) {
            if (peek_wait_spp_request(&state, request_id, sizeof(request_id))) {
                send_ble_status(&state, "wait_spp", request_id, NULL);
            }

            if (ai_spp_accept(spp_client, &connection, 250) == 0) {
                has_connection = 1;
                printf("[COMBO_CAMERA_SPP] SPP connected remote=%s fd=%d\n",
                       connection.remote_addr, connection.fd);
                fflush(stdout);
            }
            continue;
        }

        if (connection_has_closed(&connection)) {
            printf("[COMBO_CAMERA_SPP] SPP disconnected\n");
            fflush(stdout);
            ai_spp_close(&connection);
            has_connection = 0;
            continue;
        }

        if (take_pending_request(&state, request_id, sizeof(request_id))) {
            int ret = process_request(&state, &connection, request_id);
            finish_request(&state);
            if (ret != 0) {
                printf("[COMBO_CAMERA_SPP] closing SPP after send failure\n");
                fflush(stdout);
                ai_spp_close(&connection);
                has_connection = 0;
            }
            continue;
        }

        usleep(100000);
    }

    if (has_connection)
        ai_spp_close(&connection);
    ai_spp_client_destroy(spp_client);
    ai_ble_unregister_datatype(state.ble_client, COMBO_BLE_CMD_DATATYPE);
    ai_ble_client_destroy(state.ble_client);
    pthread_mutex_destroy(&state.lock);

    printf("[COMBO_CAMERA_SPP] stopped\n");
    return 0;
}
