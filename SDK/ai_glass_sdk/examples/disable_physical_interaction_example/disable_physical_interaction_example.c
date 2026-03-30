#include "ai_audio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *prog) {
    printf("Usage: %s [-s socket_path]\n", prog);
    printf("\n");
    printf("This sample enables AI-Core physical interaction disable mode\n");
    printf("(AI-Core will not auto-trigger record/capture/barge-in by physical button,\n");
    printf("while GPIO events are still available for SDK consumers).\n");
}

int main(int argc, char *argv[]) {
    const char *socket_path = NULL;
    int mode_before = 0;
    int mode_after = 0;
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

    printf("[SAMPLE] disable_physical_interaction_example started.\n");
    printf("[SAMPLE] Step 1/4: connect audio control socket...\n");

    ai_audio_t *client = ai_audio_init(socket_path);
    if (!client) {
        printf("[SAMPLE][ERROR] ai_audio_init failed.\n");
        printf("[SAMPLE][HINT] Ensure ai-core is running and audio control socket is reachable.\n");
        return 1;
    }

    printf("[SAMPLE] Step 2/4: query current sdk control mode...\n");
    rc = ai_audio_get_sdk_control_mode(client, &mode_before);
    if (rc != AI_AUDIO_SUCCESS) {
        printf("[SAMPLE][ERROR] query mode failed: %s (%d)\n", ai_audio_get_error_string(rc), rc);
        ai_audio_cleanup(client);
        return 1;
    }
    printf("[SAMPLE] Current mode: %s\n", mode_before ? "ENABLED" : "DISABLED");

    printf("[SAMPLE] Step 3/4: set sdk control mode to ENABLED...\n");
    rc = ai_audio_set_sdk_control_mode(client, 1);
    if (rc != AI_AUDIO_SUCCESS) {
        printf("[SAMPLE][ERROR] set mode failed: %s (%d)\n", ai_audio_get_error_string(rc), rc);
        ai_audio_cleanup(client);
        return 1;
    }

    printf("[SAMPLE] Step 4/4: re-check mode...\n");
    rc = ai_audio_get_sdk_control_mode(client, &mode_after);
    if (rc != AI_AUDIO_SUCCESS) {
        printf("[SAMPLE][ERROR] re-check mode failed: %s (%d)\n", ai_audio_get_error_string(rc), rc);
        ai_audio_cleanup(client);
        return 1;
    }

    if (mode_after != 1) {
        printf("[SAMPLE][ERROR] mode verification failed, expected ENABLED but got DISABLED.\n");
        ai_audio_cleanup(client);
        return 1;
    }

    printf("[SAMPLE][OK] Physical interaction disable mode is now ENABLED.\n");
    printf("[SAMPLE][OK] AI-Core physical auto actions are disabled, GPIO events remain available.\n");

    ai_audio_cleanup(client);
    return 0;
}
