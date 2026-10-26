#ifndef AI_SPP_H
#define AI_SPP_H

#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AI_SPP_SOCKET_PATH "/var/run/ai_spp.sock"
#define AI_SPP_MAX_REMOTE_ADDR_LEN 32

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

#ifdef __cplusplus
}
#endif

#endif /* AI_SPP_H */
