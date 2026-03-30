#include "ai_camera.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TARGET_CAPTURE_PATH "/tmp/test_capture"

int main(void) {
    ai_core_client_t *client = NULL;
    ai_core_data_t data;
    int rc = 0;

    memset(&data, 0, sizeof(data));

    printf("[SAMPLE] camera_capture_example started.\n");
    printf("[SAMPLE] Target output path: %s\n", TARGET_CAPTURE_PATH);
    printf("[SAMPLE] Step 1/3: init camera client...\n");

    client = ai_core_init();
    if (!client) {
        printf("[SAMPLE][ERROR] ai_core_init failed.\n");
        printf("[SAMPLE][HINT] Ensure ai-core camera service is running.\n");
        return 1;
    }

    printf("[SAMPLE] Step 2/3: capture one frame (timeout: 5000ms)...\n");
    rc = ai_core_capture(client, &data, 5000);
    if (rc != AI_MEDIA_SUCCESS) {
        printf("[SAMPLE][ERROR] capture failed: %s (%d)\n", ai_core_get_error_string(rc), rc);
        ai_core_cleanup(client);
        return 1;
    }

    printf("[SAMPLE] Capture metadata: size=%zu, resolution=%dx%d, format=%s, seq=%d\n",
           data.size,
           data.width,
           data.height,
           data.format == AI_MEDIA_FORMAT_JPEG ? "JPEG" : "NV12",
           data.sequence);

    printf("[SAMPLE] Step 3/3: save bytes to %s ...\n", TARGET_CAPTURE_PATH);
    FILE *fp = fopen(TARGET_CAPTURE_PATH, "wb");
    if (!fp) {
        printf("[SAMPLE][ERROR] open output file failed: %s\n", strerror(errno));
        ai_core_free_data(&data);
        ai_core_cleanup(client);
        return 1;
    }

    size_t written = fwrite(data.data, 1, data.size, fp);
    fclose(fp);

    if (written != data.size) {
        printf("[SAMPLE][ERROR] short write: %zu/%zu bytes\n", written, data.size);
        ai_core_free_data(&data);
        ai_core_cleanup(client);
        return 1;
    }

    printf("[SAMPLE][OK] Capture file is ready.\n");
    printf("[SAMPLE][OK] Read your file from: %s\n", TARGET_CAPTURE_PATH);
    printf("[SAMPLE][INFO] File payload format is: %s\n",
           data.format == AI_MEDIA_FORMAT_JPEG ? "JPEG bytes" : "NV12 raw bytes");

    ai_core_free_data(&data);
    ai_core_cleanup(client);
    return 0;
}
