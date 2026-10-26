#include "ai_spp.h"
#include "ai_wifi.h"

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SPP_WIFI_RX_MAX 1024
#define SPP_WIFI_JSON_MAX 256

static volatile sig_atomic_t g_running = 1;

static void handle_signal(int sig)
{
    (void)sig;
    g_running = 0;
}

static const char *skip_ws(const char *cursor)
{
    while (cursor && (*cursor == ' ' || *cursor == '\n' ||
                      *cursor == '\r' || *cursor == '\t')) {
        cursor++;
    }
    return cursor;
}

static int parse_json_string(const char **cursor, char *out, size_t out_size)
{
    const char *p;
    size_t used = 0;

    if (!cursor || !*cursor || !out || out_size == 0)
        return -1;

    p = skip_ws(*cursor);
    if (*p != '"')
        return -1;
    p++;

    while (*p && *p != '"') {
        if (used + 1 >= out_size)
            return -1;
        if (*p == '\\') {
            p++;
            if (*p == '\0')
                return -1;
        }
        out[used++] = *p;
        p++;
    }

    if (*p != '"')
        return -1;

    out[used] = '\0';
    *cursor = p + 1;
    return 0;
}

static int extract_json_string(const char *json,
                               const char *key,
                               char *value,
                               size_t value_size)
{
    const char *match;
    const char *cursor;
    char pattern[64];
    int written;

    if (!json || !key || !value || value_size == 0)
        return -1;

    written = snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    if (written < 0 || (size_t)written >= sizeof(pattern))
        return -1;

    match = strstr(json, pattern);
    if (!match)
        return -1;

    cursor = skip_ws(match + strlen(pattern));
    if (*cursor != ':')
        return -1;
    cursor++;

    return parse_json_string(&cursor, value, value_size);
}

static int append_literal(char *buffer,
                          size_t buffer_size,
                          size_t *used,
                          const char *literal)
{
    size_t len;

    if (!buffer || !used || !literal)
        return -1;

    len = strlen(literal);
    if (*used + len >= buffer_size)
        return -1;

    memcpy(buffer + *used, literal, len);
    *used += len;
    buffer[*used] = '\0';
    return 0;
}

static int append_escaped_json_string(char *buffer,
                                      size_t buffer_size,
                                      size_t *used,
                                      const char *text)
{
    size_t i;

    if (append_literal(buffer, buffer_size, used, "\"") != 0)
        return -1;

    for (i = 0; text && text[i] != '\0'; i++) {
        unsigned char ch = (unsigned char)text[i];

        if (ch == '"' || ch == '\\') {
            if (*used + 2 >= buffer_size)
                return -1;
            buffer[(*used)++] = '\\';
            buffer[(*used)++] = (char)ch;
            buffer[*used] = '\0';
            continue;
        }

        if (ch < 0x20 || *used + 1 >= buffer_size)
            return -1;

        buffer[(*used)++] = (char)ch;
        buffer[*used] = '\0';
    }

    return append_literal(buffer, buffer_size, used, "\"");
}

static int append_json_sep(char *buffer,
                           size_t buffer_size,
                           size_t *used,
                           int *field_count)
{
    if (!field_count)
        return -1;
    if (*field_count > 0 &&
        append_literal(buffer, buffer_size, used, ",") != 0) {
        return -1;
    }
    (*field_count)++;
    return 0;
}

static int append_json_key_string(char *buffer,
                                  size_t buffer_size,
                                  size_t *used,
                                  int *field_count,
                                  const char *key,
                                  const char *value)
{
    if (append_json_sep(buffer, buffer_size, used, field_count) != 0 ||
        append_escaped_json_string(buffer, buffer_size, used, key) != 0 ||
        append_literal(buffer, buffer_size, used, ":") != 0 ||
        append_escaped_json_string(buffer, buffer_size, used, value) != 0) {
        return -1;
    }
    return 0;
}

static int append_json_key_bool(char *buffer,
                                size_t buffer_size,
                                size_t *used,
                                int *field_count,
                                const char *key,
                                int value)
{
    if (append_json_sep(buffer, buffer_size, used, field_count) != 0 ||
        append_escaped_json_string(buffer, buffer_size, used, key) != 0 ||
        append_literal(buffer, buffer_size, used, ":") != 0 ||
        append_literal(buffer, buffer_size, used,
                       value ? "true" : "false") != 0) {
        return -1;
    }
    return 0;
}

static int append_json_key_int(char *buffer,
                               size_t buffer_size,
                               size_t *used,
                               int *field_count,
                               const char *key,
                               int value)
{
    char text[24];

    snprintf(text, sizeof(text), "%d", value);
    if (append_json_sep(buffer, buffer_size, used, field_count) != 0 ||
        append_escaped_json_string(buffer, buffer_size, used, key) != 0 ||
        append_literal(buffer, buffer_size, used, ":") != 0 ||
        append_literal(buffer, buffer_size, used, text) != 0) {
        return -1;
    }
    return 0;
}

static int build_wifi_result_json(const char *action,
                                  int ok,
                                  const ai_wifi_status_t *status,
                                  const char *message,
                                  char *output,
                                  size_t output_size)
{
    size_t used = 0;
    int field_count = 0;

    if (!action || !output || output_size == 0)
        return -1;

    output[0] = '\0';
    if (append_literal(output, output_size, &used, "{") != 0 ||
        append_json_key_bool(output, output_size, &used, &field_count, "ok", ok) != 0 ||
        append_json_key_string(output, output_size, &used, &field_count, "action", action) != 0 ||
        append_json_key_string(output,
                               output_size,
                               &used,
                               &field_count,
                               "state",
                               status ? ai_wifi_state_to_string(status->state) : "unknown") != 0) {
        return -1;
    }

    if (status) {
        if (status->ssid[0] != '\0' &&
            append_json_key_string(output,
                                   output_size,
                                   &used,
                                   &field_count,
                                   "ssid",
                                   status->ssid) != 0) {
            return -1;
        }
        if (status->ip_address[0] != '\0' &&
            append_json_key_string(output,
                                   output_size,
                                   &used,
                                   &field_count,
                                   "ip",
                                   status->ip_address) != 0) {
            return -1;
        }
        if (status->wpa_state[0] != '\0' &&
            append_json_key_string(output,
                                   output_size,
                                   &used,
                                   &field_count,
                                   "wpa_state",
                                   status->wpa_state) != 0) {
            return -1;
        }
        if (status->frequency_mhz > 0 &&
            append_json_key_int(output,
                                output_size,
                                &used,
                                &field_count,
                                "frequency_mhz",
                                status->frequency_mhz) != 0) {
            return -1;
        }
        if ((status->signal_level_dbm != 0 || status->state == AI_WIFI_STATE_CONNECTED) &&
            append_json_key_int(output,
                                output_size,
                                &used,
                                &field_count,
                                "signal_dbm",
                                status->signal_level_dbm) != 0) {
            return -1;
        }
    }

    if (message && message[0] != '\0' &&
        append_json_key_string(output,
                               output_size,
                               &used,
                               &field_count,
                               "message",
                               message) != 0) {
        return -1;
    }

    if (append_literal(output, output_size, &used, "}") != 0)
        return -1;
    return 0;
}

static int send_json_line(ai_spp_connection_t *connection, const char *json)
{
    char line[SPP_WIFI_JSON_MAX + 2];
    int written;

    if (!connection || !json)
        return -1;

    written = snprintf(line, sizeof(line), "%s\n", json);
    if (written < 0 || (size_t)written >= sizeof(line))
        return -1;
    return ai_spp_write(connection, line, (size_t)written) < 0 ? -1 : 0;
}

static void send_wifi_result(ai_spp_connection_t *connection,
                             const char *action,
                             int ok,
                             const ai_wifi_status_t *status,
                             const char *message)
{
    char json[SPP_WIFI_JSON_MAX];

    if (build_wifi_result_json(action, ok, status, message, json, sizeof(json)) != 0) {
        snprintf(json,
                 sizeof(json),
                 "{\"ok\":false,\"action\":\"%s\",\"state\":\"unknown\",\"message\":\"internal_error\"}",
                 action ? action : "unknown");
    }

    if (send_json_line(connection, json) != 0) {
        printf("[SPP_WIFI_DEMO] send failed action=%s\n",
               action ? action : "unknown");
    }
}

static void process_wifi_command(ai_spp_connection_t *connection, const char *line)
{
    char action[32];
    char ssid[AI_WIFI_SSID_MAX];
    char password[AI_WIFI_SSID_MAX];
    ai_wifi_status_t status;
    int status_ready = 0;

    if (!connection || !line)
        return;

    if (extract_json_string(line, "action", action, sizeof(action)) != 0) {
        send_wifi_result(connection, "unknown", 0, NULL, "invalid_request");
        return;
    }

    if (strcmp(action, "status") == 0) {
        if (ai_wifi_get_status(&status) == 0) {
            printf("[SPP_WIFI_DEMO] status state=%s ssid=%s\n",
                   ai_wifi_state_to_string(status.state),
                   status.ssid[0] ? status.ssid : "-");
            send_wifi_result(connection, action, 1, &status, NULL);
        } else {
            printf("[SPP_WIFI_DEMO] status failed\n");
            send_wifi_result(connection, action, 0, NULL, "status_failed");
        }
        return;
    }

    if (strcmp(action, "disconnect") == 0) {
        if (ai_wifi_disconnect() == 0 && ai_wifi_get_status(&status) == 0) {
            printf("[SPP_WIFI_DEMO] disconnect success\n");
            send_wifi_result(connection, action, 1, &status, NULL);
        } else {
            if (ai_wifi_get_status(&status) == 0) {
                status_ready = 1;
            }
            printf("[SPP_WIFI_DEMO] disconnect failed\n");
            send_wifi_result(connection,
                             action,
                             0,
                             status_ready ? &status : NULL,
                             "disconnect_failed");
        }
        return;
    }

    if (strcmp(action, "connect") == 0) {
        password[0] = '\0';
        if (extract_json_string(line, "ssid", ssid, sizeof(ssid)) != 0 ||
            ssid[0] == '\0') {
            if (ai_wifi_get_status(&status) == 0) {
                status_ready = 1;
            }
            printf("[SPP_WIFI_DEMO] invalid connect request\n");
            send_wifi_result(connection,
                             action,
                             0,
                             status_ready ? &status : NULL,
                             "invalid_request");
            return;
        }
        if (extract_json_string(line, "password", password, sizeof(password)) != 0) {
            password[0] = '\0';
        }

        printf("[SPP_WIFI_DEMO] connect request ssid=%s\n", ssid);
        if (ai_wifi_connect(ssid, password) == 0 && ai_wifi_get_status(&status) == 0) {
            printf("[SPP_WIFI_DEMO] connect success ssid=%s\n", ssid);
            send_wifi_result(connection, action, 1, &status, NULL);
        } else {
            if (ai_wifi_get_status(&status) == 0) {
                status_ready = 1;
            }
            printf("[SPP_WIFI_DEMO] connect failed ssid=%s\n", ssid);
            send_wifi_result(connection,
                             action,
                             0,
                             status_ready ? &status : NULL,
                             "connect_failed");
        }
        return;
    }

    printf("[SPP_WIFI_DEMO] unsupported action=%s\n", action);
    send_wifi_result(connection, action, 0, NULL, "unsupported_action");
}

static void handle_connection(ai_spp_connection_t *connection)
{
    char pending[SPP_WIFI_RX_MAX];
    size_t pending_len = 0;
    ai_wifi_status_t status;

    if (!connection)
        return;

    printf("[SPP_WIFI_DEMO] connected remote=%s fd=%d\n",
           connection->remote_addr, connection->fd);

    if (ai_wifi_get_status(&status) == 0) {
        send_wifi_result(connection, "status", 1, &status, NULL);
    }

    while (g_running) {
        struct pollfd pfd;
        ssize_t len;
        char *line_end;
        char *cursor = pending;

        memset(&pfd, 0, sizeof(pfd));
        pfd.fd = connection->fd;
        pfd.events = POLLIN | POLLERR | POLLHUP;
        do {
            len = poll(&pfd, 1, -1);
        } while (len < 0 && errno == EINTR && g_running);

        if (len <= 0 || !(pfd.revents & POLLIN)) {
            printf("[SPP_WIFI_DEMO] poll end ret=%zd revents=0x%x errno=%d\n",
                   len, pfd.revents, len < 0 ? errno : 0);
            break;
        }

        do {
            len = ai_spp_read(connection,
                              pending + pending_len,
                              sizeof(pending) - pending_len - 1);
        } while (len < 0 && errno == EINTR);

        if (len < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            continue;
        if (len <= 0) {
            printf("[SPP_WIFI_DEMO] read end len=%zd errno=%d\n",
                   len, len < 0 ? errno : 0);
            break;
        }

        pending_len += (size_t)len;
        pending[pending_len] = '\0';

        while ((line_end = strchr(cursor, '\n')) != NULL) {
            *line_end = '\0';
            if (cursor[0] != '\0') {
                process_wifi_command(connection, cursor);
            }
            cursor = line_end + 1;
        }

        if (cursor != pending) {
            size_t remaining = strlen(cursor);
            memmove(pending, cursor, remaining + 1);
            pending_len = remaining;
        } else if (pending_len + 1 >= sizeof(pending)) {
            printf("[SPP_WIFI_DEMO] pending buffer overflow, resetting\n");
            pending[0] = '\0';
            pending_len = 0;
            send_wifi_result(connection, "unknown", 0, NULL, "request_too_large");
        }
    }

    ai_spp_close(connection);
    printf("[SPP_WIFI_DEMO] connection closed\n");
}

int main(void)
{
    ai_spp_client_t *client;

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    client = ai_spp_client_create();
    if (!client) {
        printf("[SPP_WIFI_DEMO] create client failed\n");
        return 1;
    }

    if (ai_spp_client_start(client, NULL, NULL) != 0) {
        printf("[SPP_WIFI_DEMO] start client failed, is bt_service SPP broker running?\n");
        ai_spp_client_destroy(client);
        return 1;
    }

    printf("[SPP_WIFI_DEMO] waiting for SPP connection\n");
    while (g_running) {
        ai_spp_connection_t connection;
        if (ai_spp_accept(client, &connection, 1000) == 0) {
            handle_connection(&connection);
        }
    }

    ai_spp_client_destroy(client);
    printf("[SPP_WIFI_DEMO] stopped\n");
    return 0;
}
