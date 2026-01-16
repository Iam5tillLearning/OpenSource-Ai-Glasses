# Text Event Client SDK Developer Guide

> **Note**: This document describes the Text Event Client API in `ai_glass_sdk`. This interface is used to receive ASR (Speech-to-Text) and LLM (Large Language Model) text stream events from the system.

## 1. Overview

The `ai_text_event` module provides a lightweight client that allows applications to receive system-generated text events in real-time via callback functions. Main use cases include:
- Real-time display of speech recognition results.
- Displaying streaming responses from LLMs.
- Handling system notifications or prompts.

## 2. Data Structures

### 2.1 Text Source Enum (`ai_text_source_t`)

```c
typedef enum {
    AI_TEXT_SOURCE_ASR = 0,  // Speech-to-Text (ASR)
    AI_TEXT_SOURCE_LLM = 1,  // Large Language Model (LLM)
    AI_TEXT_SOURCE_SYS = 2   // System Notification (SYSTEM)
} ai_text_source_t;
```

### 2.2 Text Event Struct (`ai_text_event_t`)

```c
typedef struct {
    ai_text_source_t source; // Source of the text
    const char *text;        // Text content (UTF-8)
    int is_final;           // Is final result (1: Final, 0: Intermediate)
} ai_text_event_t;
```

### 2.3 Callback Function Prototype

```c
/**
 * @brief Text event callback function
 * @param event Pointer to event data
 * @param user_data User-defined data
 */
typedef void (*ai_text_event_cb)(const ai_text_event_t *event, void *user_data);
```

## 3. API Functions

### 3.1 Create Client

```c
/**
 * @brief Create a text event client
 * @param cb Event callback function
 * @param user_data User-defined data, will be passed back in the callback
 * @return Returns client handle on success, NULL on failure
 */
ai_text_event_client_t* ai_text_event_client_create(ai_text_event_cb cb, void *user_data);
```

### 3.2 Start Client

```c
/**
 * @brief Start the client and connect to service
 * @param client Client handle
 * @return Returns 0 on success, -1 on failure
 */
int ai_text_event_client_start(ai_text_event_client_t *client);
```

### 3.3 Destroy Client

```c
/**
 * @brief Destroy the client and release resources
 * @param client Client handle
 */
void ai_text_event_client_destroy(ai_text_event_client_t *client);
```

## 4. Usage Example

The following code demonstrates how to create a simple text event listener:

```c
#include <stdio.h>
#include <unistd.h>
#include "ai_text_event.h"

// Event callback function
void on_text_event(const ai_text_event_t *event, void *user_data) {
    const char *source_str = "UNKNOWN";
    switch (event->source) {
        case AI_TEXT_SOURCE_ASR: source_str = "ASR"; break;
        case AI_TEXT_SOURCE_LLM: source_str = "LLM"; break;
        case AI_TEXT_SOURCE_SYS: source_str = "SYS"; break;
    }

    printf("[%s] %s (final=%d)\n", source_str, event->text, event->is_final);
}

int main() {
    // 1. Create client
    ai_text_event_client_t *client = ai_text_event_client_create(on_text_event, NULL);
    if (!client) {
        fprintf(stderr, "Failed to create client\n");
        return -1;
    }

    // 2. Start client
    if (ai_text_event_client_start(client) != 0) {
        fprintf(stderr, "Failed to start client\n");
        ai_text_event_client_destroy(client);
        return -1;
    }

    printf("Client started. Waiting for events...\n");

    // 3. Main loop (Applications usually have their own main loop)
    while (1) {
        sleep(1);
    }

    // 4. Destroy client
    ai_text_event_client_destroy(client);
    return 0;
}
```

## 5. Notes

1.  **Callback Thread**: The callback function is typically executed in a separate receiver thread. Do not perform time-consuming operations in the callback. If you need to update the UI or perform complex processing, it is recommended to post the data to the main thread queue.
2.  **Lifecycle**: Ensure `ai_text_event_client_destroy` is called before the program exits to avoid incomplete resource release.
3.  **Chinese Support**: The `text` field is UTF-8 encoded. Ensure the display terminal or UI control supports UTF-8.
