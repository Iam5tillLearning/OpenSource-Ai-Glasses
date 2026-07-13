# AI Display Client API

## Overview

The AI Display Client API provides an interface for communicating with the `ai_display_service` display service. It enables high-performance framebuffer submission and display control through shared memory and Unix Socket.

## Key Features

- **Dual buffer slot design**: Supports main application and overlay layer
- **Zero-copy transfer**: Framebuffer transfer based on shared memory
- **Reliable communication**: Unix Socket command channel
- **Multi-client support**: Focus request for display priority management

## Architecture

```
┌─────────────────┐     ┌─────────────────────┐
│  Client App     │     │  ai_display_service │
│                 │     │                     │
│  ┌───────────┐  │     │  ┌───────────┐      │
│  │framebuffer│◄─┼─SHM─┼─►│framebuffer│      │
│  └───────────┘  │     │  └───────────┘      │
│       │         │     │        ▲            │
│       ▼         │     │        │            │
│  ┌───────────┐  │     │  ┌─────┴─────┐      │
│  │  commit   │──┼─UDS─┼─►│  Refresh  │      │
│  └───────────┘  │     │  └───────────┘      │
└─────────────────┘     └─────────────────────┘
```

## API Reference

### Core Functions

| Function | Description |
|----------|-------------|
| `ai_display_init()` | Initialize client, return handle |
| `ai_display_connect()` | Connect to display service |
| `ai_display_disconnect()` | Disconnect (reconnectable) |
| `ai_display_get_framebuffer()` | Get main framebuffer pointer (Slot 0) |
| `ai_display_get_framebuffer_slot()` | Get specified slot framebuffer pointer |
| `ai_display_commit_frame()` | Commit frame update to screen |
| `ai_display_request_focus()` | Request display focus |
| `ai_display_cleanup()` | Cleanup resources |

### Helper Functions

| Function | Description |
|----------|-------------|
| `ai_display_is_connected()` | Check connection status |
| `ai_display_get_error_string()` | Get error description string |

### Error Codes

| Error Code | Value | Description |
|------------|-------|-------------|
| `AI_DISPLAY_SUCCESS` | 0 | Success |
| `AI_DISPLAY_ERROR_INIT` | -1 | Initialization failed |
| `AI_DISPLAY_ERROR_CONNECT` | -2 | Connection failed |
| `AI_DISPLAY_ERROR_SHM` | -3 | Shared memory operation failed |
| `AI_DISPLAY_ERROR_SOCKET` | -4 | Socket operation failed |
| `AI_DISPLAY_ERROR_SEND` | -5 | Send failed |
| `AI_DISPLAY_ERROR_PARAM` | -6 | Invalid parameter |
| `AI_DISPLAY_ERROR_NOT_READY` | -7 | Client not ready |

## Usage Example

### Basic Usage

```c
#include "ai_display.h"
#include <string.h>

int main() {
    // 1. Initialize client
    ai_display_client_t *client = ai_display_init();
    if (!client) {
        printf("Initialization failed\n");
        return -1;
    }

    // 2. Connect to service
    int ret = ai_display_connect(client);
    if (ret != AI_DISPLAY_SUCCESS) {
        printf("Connection failed: %s\n", ai_display_get_error_string(ret));
        ai_display_cleanup(client);
        return -1;
    }

    // 3. Get framebuffer
    uint8_t *framebuffer = ai_display_get_framebuffer(client);
    if (!framebuffer) {
        printf("Failed to get framebuffer\n");
        ai_display_cleanup(client);
        return -1;
    }

    // 4. Write frame data (example: fill with black)
    memset(framebuffer, 0, AI_DISPLAY_FRAME_SIZE);

    // 5. Commit frame
    ret = ai_display_commit_frame(client, 0, 0, 0, AI_DISPLAY_WIDTH, AI_DISPLAY_HEIGHT);
    if (ret != AI_DISPLAY_SUCCESS) {
        printf("Commit failed: %s\n", ai_display_get_error_string(ret));
    }

    // 6. Cleanup
    ai_display_cleanup(client);
    return 0;
}
```

### Using Overlay Slot

```c
// Get overlay framebuffer (Slot 1)
uint8_t *overlay = ai_display_get_framebuffer_slot(client, 1);
if (overlay) {
    // Draw overlay content...

    // Commit overlay frame
    ai_display_commit_frame(client, 1, 0, 0, AI_DISPLAY_WIDTH, AI_DISPLAY_HEIGHT);
}
```

### Request Display Focus

```c
// Request foreground display in multi-client scenario
int ret = ai_display_request_focus(client);
if (ret == AI_DISPLAY_SUCCESS) {
    printf("Display focus acquired\n");
}
```

## Build and Link

```bash
# Compile
gcc -I/path/to/ai_glass_sdk/include -c your_app.c -o your_app.o

# Link (static library)
gcc your_app.o -L/path/to/ai_glass_sdk/lib -lai_glass_sdk -lrt -o your_app

# Link (shared library)
gcc your_app.o -L/path/to/ai_glass_sdk/lib -lai_glass_sdk -lrt -o your_app
```

## Notes

1. **Service must be running first**: `ai_display_service` must be started before calling `ai_display_connect()`
2. **Thread safety**: A single client handle should not be used concurrently across multiple threads
3. **Framebuffer format**: Uses 4BPP format, size is `AI_DISPLAY_FRAME_SIZE` bytes
4. **Resource cleanup**: Always call `ai_display_cleanup()` when done to release resources
