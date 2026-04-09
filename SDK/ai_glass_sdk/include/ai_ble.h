#ifndef AI_BLE_H
#define AI_BLE_H

#ifdef __cplusplus
extern "C" {
#endif

#define AI_BLE_SOCKET_PATH "/var/run/ai_ble.sock"
#define AI_BLE_MAX_PACKET_SIZE 180
#define AI_BLE_MAX_DATATYPE_LEN 32
#define AI_BLE_MAX_DATA_LEN 180

typedef struct ai_ble_client_ctx ai_ble_client_t;

typedef void (*ai_ble_text_cb)(const char *datatype, const char *data, void *user_data);

ai_ble_client_t* ai_ble_client_create(void);
int ai_ble_client_start(ai_ble_client_t *client);
int ai_ble_register_datatype(ai_ble_client_t *client, const char *datatype, ai_ble_text_cb cb, void *user_data);
int ai_ble_unregister_datatype(ai_ble_client_t *client, const char *datatype);
int ai_ble_send(ai_ble_client_t *client, const char *datatype, const char *data);
void ai_ble_client_stop(ai_ble_client_t *client);
void ai_ble_client_destroy(ai_ble_client_t *client);

#ifdef __cplusplus
}
#endif

#endif /* AI_BLE_H */
