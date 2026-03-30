#include "ai_audio.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TARGET_RECORD_PATH "/tmp/test_record_audio"

static int copy_file(const char *src, const char *dst) {
    FILE *in = NULL;
    FILE *out = NULL;
    char buf[4096];
    size_t n;

    in = fopen(src, "rb");
    if (!in) {
        return -1;
    }

    out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        return -1;
    }

    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            fclose(in);
            fclose(out);
            return -1;
        }
    }

    fclose(in);
    fclose(out);
    return 0;
}

static void print_usage(const char *prog) {
    printf("Usage: %s [-s socket_path]\n", prog);
    printf("\n");
    printf("This sample records audio via SDK command, then copies result to:\n");
    printf("  %s\n", TARGET_RECORD_PATH);
}

int main(int argc, char *argv[]) {
    const char *socket_path = NULL;
    char returned_path[512] = {0};
    int rc = 0;
    int recording = 0;

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

    printf("[SAMPLE] record_audio_example started.\n");
    printf("[SAMPLE] Target output path: %s\n", TARGET_RECORD_PATH);
    printf("[SAMPLE] Step 1/5: connect audio control socket...\n");

    ai_audio_t *client = ai_audio_init(socket_path);
    if (!client) {
        printf("[SAMPLE][ERROR] ai_audio_init failed.\n");
        printf("[SAMPLE][HINT] Ensure ai-core is running and audio control socket is reachable.\n");
        return 1;
    }

    printf("[SAMPLE] Step 2/5: start recording...\n");
    rc = ai_audio_record_start(client);
    if (rc != AI_AUDIO_SUCCESS) {
        printf("[SAMPLE][ERROR] record start failed: %s (%d)\n", ai_audio_get_error_string(rc), rc);
        printf("[SAMPLE][HINT] ai-core should run with --enable-gpio so recording loop is active.\n");
        ai_audio_cleanup(client);
        return 1;
    }

    rc = ai_audio_record_get_status(client, &recording);
    if (rc == AI_AUDIO_SUCCESS) {
        printf("[SAMPLE] Recording status after start: %s\n", recording ? "RECORDING" : "IDLE");
    }

    printf("[SAMPLE] Step 3/5: recording now. Press ENTER to stop...\n");
    (void)getchar();

    printf("[SAMPLE] Step 4/5: stop recording and fetch source path...\n");
    rc = ai_audio_record_stop(client, returned_path, (int)sizeof(returned_path));
    if (rc != AI_AUDIO_SUCCESS) {
        printf("[SAMPLE][ERROR] record stop failed: %s (%d)\n", ai_audio_get_error_string(rc), rc);
        ai_audio_cleanup(client);
        return 1;
    }

    if (returned_path[0] == '\0') {
        strncpy(returned_path, "/tmp/my_recording.pcm", sizeof(returned_path) - 1);
        returned_path[sizeof(returned_path) - 1] = '\0';
    }

    printf("[SAMPLE] Source record path from ai-core: %s\n", returned_path);

    printf("[SAMPLE] Step 5/5: copy recorded file to %s ...\n", TARGET_RECORD_PATH);
    if (copy_file(returned_path, TARGET_RECORD_PATH) != 0) {
        printf("[SAMPLE][ERROR] copy failed: %s\n", strerror(errno));
        ai_audio_cleanup(client);
        return 1;
    }

    printf("[SAMPLE][OK] Record file is ready.\n");
    printf("[SAMPLE][OK] Read your file from: %s\n", TARGET_RECORD_PATH);

    ai_audio_cleanup(client);
    return 0;
}
