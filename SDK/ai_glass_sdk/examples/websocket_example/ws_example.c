/**
 * WebSocket客户端使用示例
 *
 * 编译:
 * gcc -o ws_example ws_example.c ../src/ws_client.c ../third_party/mongoose/mongoose.c \
 *     -I../include -I../third_party/mongoose -lpthread
 */

#include "ws_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

// 全局WebSocket客户端
static ws_client_t *g_client = NULL;
static volatile int g_running = 1;

// WebSocket事件回调
void ws_callback(ws_event_type_t event, const char *data, size_t len, void *user_data) {
    switch (event) {
        case WS_EVENT_CONNECTED:
            printf("🔗 WebSocket已连接\n");
            // 连接成功后发送一条消息
            ws_client_send_text(g_client, "{\"type\":\"hello\",\"message\":\"Hello from AI-Core\"}");
            break;

        case WS_EVENT_MESSAGE:
            printf("📨 收到消息 (%zu bytes): %.*s\n", len, (int)len, data);

            // 简单的echo回复
            if (len > 0 && data[0] == '{') {
                // JSON消息，可以解析处理
                printf("   JSON消息已接收\n");
            }
            break;

        case WS_EVENT_DISCONNECTED:
            printf("❌ WebSocket已断开\n");
            g_running = 0;
            break;

        case WS_EVENT_ERROR:
            printf("⚠️  WebSocket错误: %.*s\n", (int)len, data);
            g_running = 0;
            break;
    }
}

// WebSocket运行线程
void *ws_thread(void *arg) {
    ws_client_t *client = (ws_client_t *)arg;
    ws_client_run(client);  // 阻塞运行事件循环
    return NULL;
}

// 信号处理（Ctrl+C优雅退出）
void signal_handler(int sig) {
    printf("\n收到信号 %d，正在退出...\n", sig);
    g_running = 0;
    if (g_client) {
        ws_client_stop(g_client);
    }
}

// 示例1: 连接到公共WebSocket测试服务器
void example_websocket_echo(const char *url) {
    printf("\n=== 示例1: WebSocket Echo服务器 ===\n");
    printf("连接到: %s\n", url);

    // 创建WebSocket客户端
    g_client = ws_client_create(url, ws_callback, NULL);
    if (!g_client) {
        printf("❌ 创建WebSocket客户端失败\n");
        return;
    }

    // 在独立线程中运行
    pthread_t thread;
    pthread_create(&thread, NULL, ws_thread, g_client);

    // 等待连接建立
    sleep(2);

    // 发送一些测试消息
    if (g_running) {
        printf("📤 发送消息 1...\n");
        ws_client_send_text(g_client, "Hello WebSocket!");
        sleep(1);

        printf("📤 发送消息 2...\n");
        ws_client_send_text(g_client, "{\"type\":\"test\",\"data\":\"测试数据\"}");
        sleep(1);

        printf("📤 发送消息 3...\n");
        ws_client_send_text(g_client, "Goodbye!");
        sleep(1);
    }

    // 停止并清理
    ws_client_stop(g_client);
    pthread_join(thread, NULL);
    ws_client_destroy(g_client);
    g_client = NULL;

    printf("✅ WebSocket示例完成\n");
}

// 示例2: WebSocket心跳保活
void example_websocket_keepalive(const char *url) {
    printf("\n=== 示例2: WebSocket心跳保活 ===\n");
    printf("连接到: %s\n", url);

    g_client = ws_client_create(url, ws_callback, NULL);
    if (!g_client) {
        printf("❌ 创建WebSocket客户端失败\n");
        return;
    }

    pthread_t thread;
    pthread_create(&thread, NULL, ws_thread, g_client);

    sleep(2);

    // 模拟心跳
    int heartbeat_count = 0;
    while (g_running && heartbeat_count < 5) {
        char heartbeat[64];
        snprintf(heartbeat, sizeof(heartbeat), "{\"type\":\"ping\",\"seq\":%d}", heartbeat_count);

        printf("💓 发送心跳 #%d: %s\n", heartbeat_count, heartbeat);
        ws_client_send_text(g_client, heartbeat);

        sleep(3);  // 每3秒发送一次心跳
        heartbeat_count++;
    }

    ws_client_stop(g_client);
    pthread_join(thread, NULL);
    ws_client_destroy(g_client);
    g_client = NULL;

    printf("✅ 心跳示例完成\n");
}

// 示例3: 发送二进制数据
void example_websocket_binary(const char *url) {
    printf("\n=== 示例3: WebSocket二进制数据 ===\n");
    printf("连接到: %s\n", url);

    g_client = ws_client_create(url, ws_callback, NULL);
    if (!g_client) {
        printf("❌ 创建WebSocket客户端失败\n");
        return;
    }

    pthread_t thread;
    pthread_create(&thread, NULL, ws_thread, g_client);

    sleep(2);

    // 发送二进制数据
    uint8_t binary_data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0xFF, 0xFE, 0xFD};
    printf("📤 发送二进制数据 (%zu bytes)...\n", sizeof(binary_data));
    ws_client_send_binary(g_client, binary_data, sizeof(binary_data));

    sleep(2);

    ws_client_stop(g_client);
    pthread_join(thread, NULL);
    ws_client_destroy(g_client);
    g_client = NULL;

    printf("✅ 二进制数据示例完成\n");
}

void print_usage(const char *prog) {
    printf("用法:\n");
    printf("  %s                              - 使用默认URL运行echo示例\n", prog);
    printf("  %s <url>                        - 使用指定URL运行echo示例\n", prog);
    printf("  %s echo <url>                   - 使用指定URL运行echo示例\n", prog);
    printf("  %s keepalive <url>              - 使用指定URL运行心跳示例\n", prog);
    printf("  %s binary <url>                 - 使用指定URL运行二进制示例\n", prog);
    printf("\n示例:\n");
    printf("  %s ws://localhost:8080/\n", prog);
    printf("  %s echo ws://echo.websocket.org/\n", prog);
    printf("  %s keepalive wss://example.com/ws\n", prog);
}

int main(int argc, char *argv[]) {
    printf("WebSocket客户端使用示例\n");
    printf("=======================\n");

    // 注册信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 默认URL
    const char *default_url = "wss://ws.postman-echo.com/raw";
    const char *url = default_url;
    const char *mode = "echo";

    // 解析参数
    if (argc == 1) {
        // 无参数：使用默认URL运行echo
        url = default_url;
        mode = "echo";
    } else if (argc == 2) {
        // 一个参数：判断是模式还是URL
        if (strcmp(argv[1], "echo") == 0 ||
            strcmp(argv[1], "keepalive") == 0 ||
            strcmp(argv[1], "binary") == 0) {
            // 是模式，使用默认URL
            mode = argv[1];
            url = default_url;
        } else if (strncmp(argv[1], "ws://", 5) == 0 ||
                   strncmp(argv[1], "wss://", 6) == 0) {
            // 是URL，使用echo模式
            url = argv[1];
            mode = "echo";
        } else {
            print_usage(argv[0]);
            return 1;
        }
    } else if (argc == 3) {
        // 两个参数：模式 + URL
        mode = argv[1];
        url = argv[2];

        if (strcmp(mode, "echo") != 0 &&
            strcmp(mode, "keepalive") != 0 &&
            strcmp(mode, "binary") != 0) {
            printf("❌ 错误：未知的模式 '%s'\n\n", mode);
            print_usage(argv[0]);
            return 1;
        }
    } else {
        print_usage(argv[0]);
        return 1;
    }

    // 运行对应示例
    if (strcmp(mode, "echo") == 0) {
        example_websocket_echo(url);
    } else if (strcmp(mode, "keepalive") == 0) {
        example_websocket_keepalive(url);
    } else if (strcmp(mode, "binary") == 0) {
        example_websocket_binary(url);
    }

    printf("\n✅ 程序退出\n");
    return 0;
}
