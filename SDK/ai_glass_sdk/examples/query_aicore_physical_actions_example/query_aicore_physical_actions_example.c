#include "ai_audio.h"

#include <stdio.h>
#include <string.h>

static void print_usage(const char *prog) {
    printf("Usage: %s [-s socket_path]\n", prog);
    printf("\n");
    printf("Query current disable_aicore_physical_actions state only.\n");
    printf("This sample does not change AI-Core runtime state.\n");
}

int main(int argc, char *argv[]) {
    const char *socket_path = NULL;
    int disabled = 0;
    int rc = 0;

    for (int i = 1; i < argc; ++i) {
        if ((strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--socket") == 0) && i + 1 < argc) {
            socket_path = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            printf("[SAMPLE] Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    printf("[SAMPLE] query_aicore_physical_actions_example started.\n");
    printf("[SAMPLE] Step 1/2: connect audio control socket...\n");

    ai_audio_t *client = ai_audio_init(socket_path);
    if (!client) {
        printf("[SAMPLE][ERROR] ai_audio_init failed.\n");
        printf("[SAMPLE][HINT] Ensure ai-core is running and audio control socket is reachable.\n");
        return 1;
    }

    printf("[SAMPLE] Step 2/2: query current disable_aicore_physical_actions state...\n");
    rc = ai_audio_get_disable_aicore_physical_actions(client, &disabled);
    if (rc != AI_AUDIO_SUCCESS) {
        printf("[SAMPLE][ERROR] query mode failed: %s (%d)\n", ai_audio_get_error_string(rc), rc);
        ai_audio_cleanup(client);
        return 1;
    }

    printf("[SAMPLE][OK] Current disable_aicore_physical_actions: %s\n",
           disabled ? "ENABLED" : "DISABLED");
    printf("[SAMPLE][OK] Query-only sample, no runtime state was changed.\n");

    ai_audio_cleanup(client);
    return 0;
}
