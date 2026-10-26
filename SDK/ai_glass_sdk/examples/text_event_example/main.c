/**
 * AI Glass SDK - Text Event Example
 *
 * Demonstrates how to receive and print ASR/LLM text events.
 */

#include "ai_text_event.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

static int running = 1;

static void sig_handler(int sig) {
    running = 0;
}

// Callback function to handle incoming text events
static void on_text_event(ai_text_event_t *event, void *user_data) {
    static int llm_started = 0;

    if (event->source == AI_TEXT_SOURCE_ASR) {
        // ASR 来了，如果之前有 LLM 输出，先加空行分隔
        if (llm_started) {
            printf("\n\n"); // LLM 结束，加空行
            llm_started = 0;
        }
        // ASR 整句显示
        printf("[ASR] %s\n", event->text);
    } else if (event->source == AI_TEXT_SOURCE_LLM) {
        if (!llm_started) {
            printf("[LLM] ");
            llm_started = 1;
        }
        printf("%s", event->text);
        fflush(stdout);

        // FINAL 类型表示 LLM 本轮输出结束
        if (event->type == AI_TEXT_TYPE_FINAL) {
            printf("\n\n"); // LLM 完成，加空行
            llm_started = 0;
        }
    }
}

int main(int argc, char *argv[]) {
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    printf("AI Text Event Client Example\n");
    printf("Connecting to %s...\n", AI_TEXT_EVENT_SOCKET_PATH);

    // 1. Create client
    ai_text_event_client_t *client = ai_text_event_client_create();
    if (!client) {
        fprintf(stderr, "Failed to create client\n");
        return 1;
    }

    // 2. Initialize with callback
    ai_text_event_client_init(client, on_text_event, NULL);

    // 3. Start client
    if (ai_text_event_client_start(client) != 0) {
        fprintf(stderr, "Failed to start client (is ai-core running?)\n");
        ai_text_event_client_destroy(client);
        return 1;
    }

    printf("Client started. Waiting for events (Ctrl+C to exit)...\n");

    // 4. Main loop
    while (running) {
        sleep(1);
    }

    // 5. Cleanup
    printf("Stopping client...\n");
    ai_text_event_client_destroy(client);

    return 0;
}
