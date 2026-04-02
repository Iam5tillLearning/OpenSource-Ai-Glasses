/**
 * @file media_resource_control.c
 * @brief 媒体资源切换控制台示例（相机/音频）
 *
 * 核心指令：
 *   cam_on   - 回收相机给 ai-core
 *   cam_off  - 释放相机给外部应用（如 rkipc）
 *   aud_on   - 回收音频给 ai-core
 *   aud_off  - 释放音频给外部应用（如 rkipc）
 *
 * 辅助指令：
 *   status   - 查询当前资源状态
 *   help     - 查看帮助
 *   quit     - 退出程序
 */

#include "ai_audio.h"

#include <stdio.h>
#include <string.h>

static void trim_line(char *line) {
    if (!line) {
        return;
    }

    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        line[len - 1] = '\0';
        len--;
    }
}

static void print_help(void) {
    printf("\n");
    printf("=============================================\n");
    printf(" Media Resource Control\n");
    printf("=============================================\n");
    printf("核心指令:\n");
    printf("  cam_on   - 回收相机给 ai-core\n");
    printf("  cam_off  - 释放相机给外部应用\n");
    printf("  aud_on   - 回收音频给 ai-core\n");
    printf("  aud_off  - 释放音频给外部应用\n");
    printf("\n");
    printf("辅助指令:\n");
    printf("  status   - 查询资源状态\n");
    printf("  help     - 显示帮助\n");
    printf("  quit     - 退出\n");
    printf("=============================================\n");
}

static void print_status(ai_audio_t *client) {
    ai_audio_resource_status_t status;
    int ret = ai_audio_get_resource_status(client, &status);
    if (ret != AI_AUDIO_SUCCESS) {
        printf("ERROR: 查询状态失败: %s (%d)\n", ai_audio_get_error_string(ret), ret);
        return;
    }

    printf("状态: camera=%s, audio=%s\n",
           status.camera_suspended ? "suspended(已释放)" : "active(ai-core持有)",
           status.audio_suspended ? "suspended(已释放)" : "active(ai-core持有)");
}

static int handle_command(ai_audio_t *client, const char *cmd) {
    int ret = AI_AUDIO_SUCCESS;

    if (strcmp(cmd, "cam_on") == 0) {
        ret = ai_audio_resume_resources(client, AI_AUDIO_RESOURCE_CAMERA);
        if (ret == AI_AUDIO_SUCCESS) {
            printf("OK: cam_on 成功\n");
        } else {
            printf("ERROR: cam_on 失败: %s (%d)\n", ai_audio_get_error_string(ret), ret);
        }
    } else if (strcmp(cmd, "cam_off") == 0) {
        ret = ai_audio_suspend_resources(client, AI_AUDIO_RESOURCE_CAMERA);
        if (ret == AI_AUDIO_SUCCESS) {
            printf("OK: cam_off 成功\n");
        } else {
            printf("ERROR: cam_off 失败: %s (%d)\n", ai_audio_get_error_string(ret), ret);
        }
    } else if (strcmp(cmd, "aud_on") == 0) {
        ret = ai_audio_resume_resources(client, AI_AUDIO_RESOURCE_AUDIO);
        if (ret == AI_AUDIO_SUCCESS) {
            printf("OK: aud_on 成功\n");
        } else {
            printf("ERROR: aud_on 失败: %s (%d)\n", ai_audio_get_error_string(ret), ret);
        }
    } else if (strcmp(cmd, "aud_off") == 0) {
        ret = ai_audio_suspend_resources(client, AI_AUDIO_RESOURCE_AUDIO);
        if (ret == AI_AUDIO_SUCCESS) {
            printf("OK: aud_off 成功\n");
        } else {
            printf("ERROR: aud_off 失败: %s (%d)\n", ai_audio_get_error_string(ret), ret);
        }
    } else if (strcmp(cmd, "status") == 0) {
        print_status(client);
        return 0;
    } else if (strcmp(cmd, "help") == 0) {
        print_help();
        return 0;
    } else {
        printf("WARN: 未知指令: %s\n", cmd);
        printf("可用指令: cam_on cam_off aud_on aud_off status help quit\n");
        return -1;
    }

    print_status(client);
    return (ret == AI_AUDIO_SUCCESS) ? 0 : -1;
}

int main(int argc, char *argv[]) {
    const char *socket_path = NULL;
    if (argc > 1) {
        socket_path = argv[1];
    }

    ai_audio_t *client = ai_audio_init(socket_path);
    if (!client) {
        printf("ERROR: 初始化失败，无法连接 ai-core 音频控制通道\n");
        return -1;
    }

    print_help();
    print_status(client);

    char line[128];
    while (1) {
        printf("\nmedia-resource> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }

        trim_line(line);
        if (line[0] == '\0') {
            continue;
        }

        if (strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0) {
            printf("退出媒体资源切换控制台。\n");
            break;
        }

        handle_command(client, line);
    }

    ai_audio_cleanup(client);
    return 0;
}
