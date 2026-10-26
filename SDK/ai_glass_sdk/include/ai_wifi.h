#ifndef AI_WIFI_H
#define AI_WIFI_H

#ifdef __cplusplus
extern "C" {
#endif

#define AI_WIFI_DEFAULT_IFNAME "wlan0"
#define AI_WIFI_GUARD_SOCKET_PATH "/var/run/guard_wifi.sock"
#define AI_WIFI_IFNAME_MAX 16
#define AI_WIFI_SSID_MAX 64
#define AI_WIFI_BSSID_MAX 32
#define AI_WIFI_IP_MAX 64
#define AI_WIFI_WPA_STATE_MAX 32

typedef enum {
    AI_WIFI_STATE_UNKNOWN = 0,
    AI_WIFI_STATE_DISCONNECTED = 1,
    AI_WIFI_STATE_CONNECTING = 2,
    AI_WIFI_STATE_CONNECTED = 3,
} ai_wifi_state_t;

typedef struct {
    ai_wifi_state_t state;
    char ifname[AI_WIFI_IFNAME_MAX];
    char ssid[AI_WIFI_SSID_MAX];
    char bssid[AI_WIFI_BSSID_MAX];
    char ip_address[AI_WIFI_IP_MAX];
    char wpa_state[AI_WIFI_WPA_STATE_MAX];
    int network_id;
    int frequency_mhz;
    int signal_level_dbm;
} ai_wifi_status_t;

/*
 * Wi-Fi 状态和控制由 Guard 唯一执行。以下 API 通过本地 Unix socket 请求
 * Guard；调用方不得再将它视为 wpa_cli 直控接口。
 */
int ai_wifi_get_status(ai_wifi_status_t *status);
int ai_wifi_get_status_on(const char *ifname, ai_wifi_status_t *status);

int ai_wifi_connect(const char *ssid, const char *password);
int ai_wifi_connect_on(const char *ifname,
                       const char *ssid,
                       const char *password,
                       int timeout_ms);

int ai_wifi_disconnect(void);
int ai_wifi_disconnect_on(const char *ifname, int timeout_ms);

const char *ai_wifi_state_to_string(ai_wifi_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* AI_WIFI_H */
