/**
 * =============================================================================
 * GPIO事件广播服务 - 客户端示例程序
 * =============================================================================
 *
 * 【功能说明】
 * 本程序演示如何使用GPIO事件广播服务的客户端API，订阅GPIO按键事件。
 *
 * 【使用场景】
 * - 需要在独立进程中响应GPIO按键事件
 * - 多个进程需要同时监听同一个GPIO
 * - 不直接访问GPIO硬件，而是通过事件服务获取通知
 *
 * 【编译方法】
 * gcc -o gpio_client_example gpio_event_client_example.c \
 *     ../service/src/ai_gpio.c \
 *     ../service/src/ai_gpio_manager.c \
 *     ../service/src/ai_ipc.c \
 *     -I../service/src \
 *     -I../../../output/out/media_out/include \
 *     -lpthread -lrt
 *
 * 【运行方法】
 * ./gpio_client_example
 *
 * 【注意事项】
 * - 需要先启动ai-core服务端（启用GPIO功能）
 * - 确保服务端已经初始化GPIO事件广播服务
 *
 * 作者：AI Media Service Team
 * 版本：v1.0
 * 日期：2025-10-09
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "ai_gpio.h"

// 全局变量：控制程序退出
static volatile int g_should_exit = 0;

// 统计变量
static int g_press_count = 0;
static int g_release_count = 0;

/**
 * 信号处理函数（Ctrl+C退出）
 */
void signal_handler(int sig) {
    printf("\n🛑 收到退出信号，准备关闭...\n");
    g_should_exit = 1;
}

/**
 * GPIO事件回调函数
 * 当GPIO状态变化时，此函数会被调用（在独立线程中执行）
 */
void my_gpio_event_callback(gpio_event_t event_type, int gpio_number, void *user_data) {
    uint64_t timestamp = ai_gpio_get_timestamp_us();

    switch (event_type) {
        case GPIO_EVENT_PRESS:
            g_press_count++;
            printf("\n");
            printf("═══════════════════════════════════════════\n");
            printf("  🔴 GPIO%d 按键按下事件\n", gpio_number);
            printf("───────────────────────────────────────────\n");
            printf("  时间戳: %llu us\n", (unsigned long long)timestamp);
            printf("  按下次数: %d\n", g_press_count);
            printf("═══════════════════════════════════════════\n");
            printf("\n");
            break;

        case GPIO_EVENT_RELEASE:
            g_release_count++;
            printf("\n");
            printf("═══════════════════════════════════════════\n");
            printf("  ⚪ GPIO%d 按键释放事件\n", gpio_number);
            printf("───────────────────────────────────────────\n");
            printf("  时间戳: %llu us\n", (unsigned long long)timestamp);
            printf("  释放次数: %d\n", g_release_count);
            printf("═══════════════════════════════════════════\n");
            printf("\n");
            break;

        case GPIO_EVENT_ERROR:
            printf("❌ GPIO%d 错误事件\n", gpio_number);
            break;

        default:
            printf("⚠️  未知事件类型: %d\n", event_type);
            break;
    }

    fflush(stdout);
}

// 全局变量：GPIO编号，默认为1（向后兼容）
static int g_target_gpio = 1;

/**
 * 打印程序使用说明
 */
void print_usage(const char *program_name) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  GPIO事件客户端示例程序\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("【功能】订阅GPIO事件，实时接收按键通知\n");
    printf("\n");
    printf("【使用方法】\n");
    printf("  %s [-g <gpio_number>]\n", program_name);
    printf("\n");
    printf("  选项:\n");
    printf("    -g <gpio_number>  指定要监听的GPIO编号（默认：1）\n");
    printf("\n");
    printf("【前置条件】\n");
    printf("  1. 确保ai-core服务端已启动\n");
    printf("  2. 服务端需要启用GPIO功能（--enable-gpio）\n");
    printf("  3. 若监听非主按键，服务端需指定--gpio-numbers\n");
    printf("\n");
    printf("【退出方式】\n");
    printf("  按 Ctrl+C 退出程序\n");
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");
}

/**
 * GPIO事件客户端示例：事件驱动模式（异步回调）
 */
int run_gpio_event_client(int gpio_number) {
    gpio_event_client_t client = {0};

    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  GPIO事件客户端 - 异步回调模式\n");
    printf("  监听目标: GPIO %d\n", gpio_number);
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");

    // 步骤1: 创建客户端
    printf("📝 [步骤1/3] 创建GPIO事件客户端...\n");
    if (ai_gpio_event_client_create(&client) != 0) {
        printf("❌ 创建客户端失败\n");
        return -1;
    }
    printf("✅ 客户端已创建\n\n");

    // 步骤2: 连接到服务
    printf("📝 [步骤2/3] 连接到GPIO事件广播服务 (GPIO %d)...\n", gpio_number);
    
    // 【修改】连接到指定的GPIO服务
    if (ai_gpio_event_client_connect_gpio(&client, gpio_number) != 0) {
        printf("❌ 连接失败，请确保ai-core已启动并启用GPIO功能\n");
        printf("   提示：服务端是否添加了 --gpio-numbers %d 参数？\n", gpio_number);
        ai_gpio_event_client_destroy(&client);
        return -1;
    }
    printf("✅ 已连接到服务\n\n");

    // 检查服务状态
    if (!ai_gpio_event_client_is_service_alive(&client)) {
        printf("⚠️  警告：服务可能未正常运行\n\n");
    }

    // 步骤3: 订阅事件
    printf("📝 [步骤3/3] 订阅GPIO事件...\n");
    if (ai_gpio_event_client_subscribe(&client, my_gpio_event_callback, NULL) != 0) {
        printf("❌ 订阅失败\n");
        ai_gpio_event_client_disconnect(&client);
        ai_gpio_event_client_destroy(&client);
        return -1;
    }
    printf("✅ 已订阅GPIO事件\n");
    printf("   - 本地通知Socket: %s\n", client.notify_socket_path);
    printf("   - 当前事件序列号: %u\n\n", client.last_sequence);

    printf("═══════════════════════════════════════════════════════════\n");
    printf("  🎧 监听中... 请按下GPIO %d 按键\n", gpio_number);
    printf("  💡 提示：按 Ctrl+C 退出程序\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");

    // 主循环：等待事件（实际处理在回调函数中）
    while (!g_should_exit) {
        sleep(1);

        // 定期显示状态（每10秒）
        static int counter = 0;
        if (++counter % 10 == 0) {
            printf("💓 心跳检查... 按下次数=%d, 释放次数=%d\n",
                   g_press_count, g_release_count);

            // 检查服务是否仍然活跃
            if (!ai_gpio_event_client_is_service_alive(&client)) {
                printf("⚠️  服务已停止，准备退出\n");
                break;
            }
        }
    }

    // 清理资源
    printf("\n📝 清理资源...\n");
    ai_gpio_event_client_unsubscribe(&client);
    printf("   - 已注销通知Socket\n");
    ai_gpio_event_client_disconnect(&client);
    ai_gpio_event_client_destroy(&client);
    printf("✅ 资源已清理\n\n");

    return 0;
}

/**
 * 主函数
 */
int main(int argc, char *argv[]) {
    // 解析命令行参数
    int opt;
    while ((opt = getopt(argc, argv, "g:h")) != -1) {
        switch (opt) {
        case 'g':
            g_target_gpio = atoi(optarg);
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        default:
            print_usage(argv[0]);
            return -1;
        }
    }

    // 注册信号处理函数
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 打印使用说明
    // print_usage(argv[0]); // 启动时不强制打印，保持清爽

    // 运行GPIO事件客户端
    int result = run_gpio_event_client(g_target_gpio);

    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  程序退出\n");
    printf("───────────────────────────────────────────────────────────\n");
    printf("  总按下次数: %d\n", g_press_count);
    printf("  总释放次数: %d\n", g_release_count);
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");

    return result;
}

