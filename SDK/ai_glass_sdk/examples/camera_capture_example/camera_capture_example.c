/**
 * AI Media Client Usage Example
 *
 * 演示如何使用AI媒体共享内存客户端API
 *
 * 编译命令:
 * gcc -o example_media_client example_media_client.c ai_camera.c -lrt
 *
 * 使用方法:
 * ./example_media_client [save_path]
 */

#include "ai_camera.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

static volatile int g_running = 1;

static void signal_handler(int sig) {
    printf("\n🛑 [EXAMPLE] Received signal %d, exiting...\n", sig);
    g_running = 0;
}

int main(int argc, char *argv[]) {
    ai_core_client_t *client = NULL;
    ai_core_data_t data;
    int result;
    int capture_count = 0;
    const char *save_path = (argc > 1) ? argv[1] : "/tmp";

    printf("🚀 [EXAMPLE] AI Media Client Example Starting...\n");
    printf("📁 [EXAMPLE] Media data will be saved to: %s\n", save_path);
    printf("💡 [EXAMPLE] Press Ctrl+C to exit\n\n");

    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 初始化AI媒体客户端
    client = ai_core_init();
    if (!client) {
        printf("❌ [EXAMPLE] Failed to initialize media client\n");
        return -1;
    }

    // 主循环 - 每3秒捕获一次媒体数据
    while (g_running) {
        printf("📸 [EXAMPLE] Capturing media data #%d...\n", capture_count + 1);

        // 捕获媒体数据 (5秒超时)
        result = ai_core_capture(client, &data, 5000);

        if (result == AI_MEDIA_SUCCESS) {
            printf("✅ [EXAMPLE] Capture successful:\n");
            printf("   Size: %zu bytes\n", data.size);
            printf("   Resolution: %dx%d\n", data.width, data.height);
            printf("   Format: %s\n", (data.format == AI_MEDIA_FORMAT_JPEG) ? "JPEG" : "NV12");
            printf("   Sequence: %d\n", data.sequence);

            // 保存媒体数据到文件
            char filename[512];
            const char *ext = (data.format == AI_MEDIA_FORMAT_JPEG) ? "jpg" : "nv12";
            snprintf(filename, sizeof(filename), "%s/capture_%03d.%s", save_path, capture_count + 1, ext);

            FILE *fp = fopen(filename, "wb");
            if (fp) {
                size_t written = fwrite(data.data, 1, data.size, fp);
                fclose(fp);

                if (written == data.size) {
                    printf("💾 [EXAMPLE] Media data saved to: %s\n", filename);
                } else {
                    printf("⚠️ [EXAMPLE] Warning: Only wrote %zu/%zu bytes to %s\n",
                           written, data.size, filename);
                }
            } else {
                printf("❌ [EXAMPLE] Failed to save data to %s: %s\n", filename, strerror(errno));
            }

            // 释放媒体数据内存
            ai_core_free_data(&data);
            capture_count++;

        } else {
            printf("❌ [EXAMPLE] Capture failed: %s\n", ai_core_get_error_string(result));

            // 如果是初始化错误（通常表示服务端不可用），退出程序
            if (result == AI_MEDIA_ERROR_INIT) {
                printf("🛑 [EXAMPLE] Service unavailable, exiting...\n");
                break;
            }
        }

        printf("\n");

        // 等待3秒后进行下次捕获
        for (int i = 0; i < 5 && g_running; i++) {
            usleep(100000); // 100ms
        }
    }

    // 清理资源
    ai_core_cleanup(client);

    printf("📊 [EXAMPLE] Total captures: %d\n", capture_count);
    printf("✅ [EXAMPLE] AI Media Client Example Finished\n");

    return 0;
}
