# GPIO Event Broadcast Service - Client Integration Guide

[中文版本](GPIO_Client_API.md) | English Version

> **Version**: v1.1 | **Date**: 2025-10-10 | **Status**: ✅ Production Ready

---

## 📖 Table of Contents

- [Quick Start](#quick-start)
- [Integration Guide](#ii-external-integration-guide)
- [Configuration Parameters](#iii-configuration-parameters)
- [Troubleshooting](#iv-troubleshooting)
- [Complete Example](#v-complete-example)

---

## 🚀 Quick Start

### Function Introduction

Listen for GPIO button events, supports multi-process asynchronous callbacks. Provides event notifications for button press, short press, long press, release, etc.

### Compile and Run

#### 1. Compile and Run Client Example
```bash
# Compile client example
cd ai_glass_sdk/examples/gpio_client
make

# Run example
./gpio_event_client_example
```

### Minimal Client Code

```c
#include "ai_gpio.h"

void my_callback(gpio_event_t event, int gpio, void *data) {
    if (event == GPIO_EVENT_PRESS) {
        printf("Button pressed\n");
    }
}

int main() {
    gpio_event_client_t client = {0};

    // Create and connect
    ai_gpio_event_client_create(&client);
    ai_gpio_event_client_connect(&client);

    // Subscribe to events
    ai_gpio_event_client_subscribe(&client, my_callback, NULL);

    // Wait for events...
    while (1) sleep(1);

    // Cleanup
    ai_gpio_event_client_unsubscribe(&client);
    ai_gpio_event_client_destroy(&client);

    return 0;
}
```

### Main Features

- ✅ Supports multiple processes listening to the same GPIO simultaneously
- ✅ Low latency event notification (< 13ms)
- ✅ Supports up to 64 concurrent clients
- ✅ Event history cache (32 events)
- ✅ Asynchronous callback mechanism
- ✅ Client can exit normally when server stops

### Performance Metrics

| Metric | Value |
|------|-----|
| Event Latency | < 13ms |
| Memory Usage | 4KB (Shared Memory) |
| CPU Usage | < 1% |
| Client Capacity | 64 |

---

## II. External Integration Guide

### 2.1 Compile Client Program

#### Link SDK Library

```bash
# Compile SDK library first
cd ai_glass_sdk
make

# Compile your own program
arm-rockchip831-linux-uclibcgnueabihf-gcc \
    -o my_gpio_client \
    my_gpio_client.c \
    -I/path/to/ai_glass_sdk/include \
    -L/path/to/ai_glass_sdk/lib \
    -lai_glass_sdk \
    -lpthread -lrt
```

### 2.2 Client API

#### Basic Flow

```c
#include "ai_gpio.h"

// 1. Create client
gpio_event_client_t client = {0};
ai_gpio_event_client_create(&client);

// 2. Connect to service
ai_gpio_event_client_connect(&client);

// 3. Subscribe to events
ai_gpio_event_client_subscribe(&client, my_callback, NULL);

// 4. Program continues running, events notified via callback asynchronously
while (running) {
    sleep(1);
}

// 5. Cleanup resources
ai_gpio_event_client_unsubscribe(&client);
ai_gpio_event_client_destroy(&client);
```

#### Event Callback Function

```c
void my_callback(gpio_event_t event, int gpio_num, void *user_data) {
    switch (event) {
        case GPIO_EVENT_PRESS:
            printf("GPIO%d Button Pressed\n", gpio_num);
            // Handle press event
            break;

        case GPIO_EVENT_RELEASE:
            printf("GPIO%d Button Released\n", gpio_num);
            // Handle release event
            break;

        case GPIO_EVENT_ERROR:
            printf("GPIO%d Error\n", gpio_num);
            break;
    }
}
```

#### Core API Description

| API | Description |
|-----|------|
| `ai_gpio_event_client_create()` | Create client instance |
| `ai_gpio_event_client_connect()` | Connect to GPIO event service |
| `ai_gpio_event_client_subscribe()` | Subscribe to GPIO events (async callback) |
| `ai_gpio_event_client_unsubscribe()` | Unsubscribe |
| `ai_gpio_event_client_disconnect()` | Disconnect |
| `ai_gpio_event_client_destroy()` | Destroy client, free resources |
| `ai_gpio_event_client_is_service_alive()` | Check if service is alive |

---

## III. Configuration Parameters

### Event Types

| Event | Description |
|------|------|
| `GPIO_EVENT_PRESS` | Button Pressed |
| `GPIO_EVENT_CLICK` | Short Click (≤2 seconds) |
| `GPIO_EVENT_LONG_CLICK` | Long Click (>2 seconds) |
| `GPIO_EVENT_RELEASE` | Button Released |
| `GPIO_EVENT_ERROR` | Error |

### Performance Metrics

- Event Latency: < 13ms
- Supports up to 64 concurrent clients
- Event Queue: 32 event cache

---

## IV. Troubleshooting

### Connection Failure

**Issue**: `ai_gpio_event_client_connect()` returns -1

**Cause**:
- Service not started or abnormal
- GPIO module not initialized

**Solution**:
- Use `ai_gpio_event_client_is_service_alive()` to check service status
- Confirm server is running normally

### Events Not Received

**Cause**:
- Callback function not set correctly
- GPIO hardware not triggered
- Subscription failed

**Solution**:
- Check return value of `ai_gpio_event_client_subscribe()`
- Verify callback function is correctly registered
- Manually trigger GPIO test

### Multiple Clients

✅ Fully supports multiple clients listening to the same GPIO simultaneously

---

## V. Complete Example

Reference file: `ai_glass_sdk/examples/gpio_client/gpio_event_client_example.c`

```c
#include "ai_gpio.h"
#include <stdio.h>
#include <signal.h>

static volatile int g_running = 1;
static int g_press_count = 0;
static int g_release_count = 0;

void signal_handler(int sig) {
    g_running = 0;
}

void my_gpio_event_callback(gpio_event_t event, int gpio_num, void *user_data) {
    switch (event) {
        case GPIO_EVENT_PRESS:
            g_press_count++;
            printf("═══════════════════════════════════════════\n");
            printf("  🔴 GPIO%d Button Pressed Event\n", gpio_num);
            printf("  Press Count: %d\n", g_press_count);
            printf("═══════════════════════════════════════════\n\n");
            break;

        case GPIO_EVENT_RELEASE:
            g_release_count++;
            printf("═══════════════════════════════════════════\n");
            printf("  ⚪ GPIO%d Button Released Event\n", gpio_num);
            printf("  Release Count: %d\n", g_release_count);
            printf("═══════════════════════════════════════════\n\n");
            break;

        case GPIO_EVENT_ERROR:
            printf("❌ GPIO%d Error\n", gpio_num);
            break;
    }
}

int main() {
    gpio_event_client_t client = {0};

    // Register signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("═══════════════════════════════════════════\n");
    printf("  GPIO Event Client Example\n");
    printf("═══════════════════════════════════════════\n\n");

    // 1. Create client
    printf("📝 [Step 1/3] Creating GPIO event client...\n");
    if (ai_gpio_event_client_create(&client) != 0) {
        printf("❌ Failed to create client\n");
        return -1;
    }
    printf("✅ Client created\n\n");

    // 2. Connect to service
    printf("📝 [Step 2/3] Connecting to GPIO event broadcast service...\n");
    if (ai_gpio_event_client_connect(&client) != 0) {
        printf("❌ Failed to connect to service, please check service status\n");
        ai_gpio_event_client_destroy(&client);
        return -1;
    }
    printf("✅ Connected to service\n\n");

    // 3. Subscribe to events
    printf("📝 [Step 3/3] Subscribing to GPIO events...\n");
    if (ai_gpio_event_client_subscribe(&client, my_gpio_event_callback, NULL) != 0) {
        printf("❌ Failed to subscribe to events\n");
        ai_gpio_event_client_disconnect(&client);
        ai_gpio_event_client_destroy(&client);
        return -1;
    }

    printf("\n═══════════════════════════════════════════\n");
    printf("  🎧 Listening... Please press GPIO button\n");
    printf("  💡 Hint: Press Ctrl+C to exit program\n");
    printf("═══════════════════════════════════════════\n\n");

    // Main loop
    while (g_running) {
        sleep(1);
    }

    // Cleanup
    printf("\n📝 Cleaning up resources...\n");
    ai_gpio_event_client_unsubscribe(&client);
    ai_gpio_event_client_disconnect(&client);
    ai_gpio_event_client_destroy(&client);

    printf("\n═══════════════════════════════════════════\n");
    printf("  Statistics:\n");
    printf("  Press Count: %d\n", g_press_count);
    printf("  Release Count: %d\n", g_release_count);
    printf("═══════════════════════════════════════════\n");
    printf("✅ Program exited normally\n");

    return 0;
}
```

---

## Related Files

- **Header File**: `ai_glass_sdk/include/ai_gpio.h`
- **Example Program**: `ai_glass_sdk/examples/gpio_client/gpio_event_client_example.c`
- **SDK README**: `ai_glass_sdk/README.md`

---

**Version**: v1.1
**Date**: 2025-10-10
**Author**: AI Media Service Team
**Status**: ✅ Production Ready
