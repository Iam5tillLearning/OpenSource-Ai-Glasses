# Camera Client API - Integration Guide

[中文版本](Camera_Client_API.md) | English Version

> **Version**: v1.0 | **Date**: 2025-10-10

---

## 📖 Table of Contents

- [Quick Start](#quick-start)
- [API Reference](#api-reference)
- [Complete Example](#complete-example)
- [Troubleshooting](#troubleshooting)

---

## 🚀 Quick Start

### Function Introduction

Camera function call, supports JPEG and NV12 image formats.

### Compile Client Program

#### Link SDK Library

```bash
# Compile SDK library first
cd ai_glass_sdk
make

# Compile your own program
arm-rockchip831-linux-uclibcgnueabihf-gcc \
    -o my_camera_client \
    my_camera_client.c \
    -I/path/to/ai_glass_sdk/include \
    -L/path/to/ai_glass_sdk/lib \
    -lai_glass_sdk \
    -lrt
```

### Run Example Program

```bash
# Run client example
cd ai_glass_sdk/examples/example_media_client
./example_media_client /mnt/sdcard
```

### Minimal Client Code

```c
#include "ai_camera.h"
#include <stdio.h>

int main() {
    // 1. Initialize client
    ai_core_client_t *client = ai_core_init();
    if (!client) {
        printf("Initialization failed\n");
        return -1;
    }

    // 2. Capture image
    ai_core_data_t data;
    int result = ai_core_capture(client, &data, 5000);

    if (result == AI_MEDIA_SUCCESS) {
        printf("Capture success: %zu bytes, %dx%d\n",
               data.size, data.width, data.height);

        // 3. Process image data (e.g., save to file)
        FILE *fp = fopen("/tmp/capture.jpg", "wb");
        fwrite(data.data, 1, data.size, fp);
        fclose(fp);

        // 4. Free image memory
        ai_core_free_data(&data);
    }

    // 5. Cleanup resources
    ai_core_cleanup(client);
    return 0;
}
```

### Main Features

- ✅ Supports JPEG and NV12 formats
- ✅ Supports multi-client concurrent access
- ✅ Configurable timeout
- ✅ Complete error handling

---

## 📋 API Reference

### Data Structures

#### ai_core_client_t

Client handle (opaque type), created via `ai_core_init()`.

#### ai_core_data_t

Image data structure:

```c
typedef struct {
    unsigned char *data;    // Image data pointer
    size_t size;            // Data size (bytes)
    int width;              // Image width
    int height;             // Image height
    int format;             // Image format
    int sequence;           // Frame sequence number
} ai_core_data_t;
```

#### Image Format Constants

```c
#define AI_MEDIA_FORMAT_JPEG  0    // JPEG format
#define AI_MEDIA_FORMAT_NV12  1    // NV12 format
```

#### Error Codes

```c
#define AI_MEDIA_SUCCESS           0    // Success
#define AI_MEDIA_ERROR_INIT       -1    // Initialization failed
#define AI_MEDIA_ERROR_TIMEOUT    -2    // Timeout
#define AI_MEDIA_ERROR_CAPTURE    -3    // Capture failed
#define AI_MEDIA_ERROR_MEMORY     -4    // Memory error
```

### Core API

#### ai_core_init()

Initialize client, establish connection with server.

```c
ai_core_client_t* ai_core_init(void);
```

**Return Value**:
- Success: Client handle pointer
- Failure: NULL

**Description**:
- Connect to camera service
- Automatically retry connection (up to 3 times)

---

#### ai_core_capture()

Capture one frame of image.

```c
int ai_core_capture(ai_core_client_t *client,
                     ai_core_data_t *data,
                     int timeout_ms);
```

**Parameters**:
- `client` - Client handle
- `data` - Output image data (filled by function)
- `timeout_ms` - Timeout (milliseconds), 0 means no timeout

**Return Value**:
- `AI_MEDIA_SUCCESS` (0) - Success
- `AI_MEDIA_ERROR_TIMEOUT` (-2) - Timeout
- `AI_MEDIA_ERROR_CAPTURE` (-3) - Capture failed
- `AI_MEDIA_ERROR_MEMORY` (-4) - Memory allocation failed

**Description**:
- Function allocates memory to store image data
- Must call `ai_core_free_data()` to free memory
- Supports concurrent calls (multiple clients capturing simultaneously)

---

#### ai_core_free_data()

Free image data memory.

```c
void ai_core_free_data(ai_core_data_t *data);
```

**Parameters**:
- `data` - Image data to free

**Description**:
- Free memory allocated by `ai_core_capture()`
- `data->data` will be set to NULL after call

---

#### ai_core_cleanup()

Cleanup client resources.

```c
void ai_core_cleanup(ai_core_client_t *client);
```

**Parameters**:
- `client` - Client handle

**Description**:
- Disconnect from server
- Free all resources

---

#### ai_core_get_error_string()

Get error message corresponding to error code.

```c
const char* ai_core_get_error_string(int error_code);
```

**Parameters**:
- `error_code` - Error code

**Return Value**:
- Error message string

**Example**:
```c
int result = ai_core_capture(client, &data, 5000);
if (result != AI_MEDIA_SUCCESS) {
    printf("Error: %s\n", ai_core_get_error_string(result));
}
```

---

## 💡 Complete Example

Reference file: `ai_glass_sdk/examples/example_media_client/example_media_client.c`

```c
#include "ai_camera.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char *argv[]) {
    const char *save_path = (argc > 1) ? argv[1] : "/tmp";
    int capture_count = 0;

    printf("═══════════════════════════════════════════\n");
    printf("  Camera Client Example\n");
    printf("  Save Path: %s\n", save_path);
    printf("═══════════════════════════════════════════\n\n");

    // 1. Initialize client
    printf("📝 Initializing client...\n");
    ai_core_client_t *client = ai_core_init();
    if (!client) {
        printf("❌ Initialization failed, please ensure server is started\n");
        return -1;
    }
    printf("✅ Client initialized\n\n");

    // 2. Capture multiple frames
    for (int i = 0; i < 5; i++) {
        printf("📷 [%d/5] Capturing image...\n", i + 1);

        ai_core_data_t data;
        int result = ai_core_capture(client, &data, 5000);

        if (result == AI_MEDIA_SUCCESS) {
            const char *format_str = (data.format == AI_MEDIA_FORMAT_JPEG) ?
                                     "JPEG" : "NV12";

            printf("✅ Capture success:\n");
            printf("   Format: %s\n", format_str);
            printf("   Resolution: %dx%d\n", data.width, data.height);
            printf("   Size: %zu bytes\n", data.size);
            printf("   Sequence: %d\n", data.sequence);

            // Save to file
            char filename[256];
            const char *ext = (data.format == AI_MEDIA_FORMAT_JPEG) ?
                              "jpg" : "nv12";
            snprintf(filename, sizeof(filename),
                     "%s/capture_%03d.%s", save_path, i + 1, ext);

            FILE *fp = fopen(filename, "wb");
            if (fp) {
                fwrite(data.data, 1, data.size, fp);
                fclose(fp);
                printf("   Saved: %s\n", filename);
            } else {
                printf("   ⚠️  Save failed: %s\n", filename);
            }

            // Free memory
            ai_core_free_data(&data);
            capture_count++;
        } else {
            printf("❌ Capture failed: %s\n",
                   ai_core_get_error_string(result));
        }

        printf("\n");
        sleep(1);  // Wait 1 second before next capture
    }

    // 3. Cleanup resources
    printf("📝 Cleaning up resources...\n");
    ai_core_cleanup(client);

    printf("\n═══════════════════════════════════════════\n");
    printf("  Done! Successfully captured %d frames\n", capture_count);
    printf("═══════════════════════════════════════════\n");

    return 0;
}
```

---

## 🔧 Troubleshooting

### 1. Client Initialization Failure

**Error**: `ai_core_init()` returns NULL

**Cause**:
- Server not started
- Server camera mode not enabled
- Unix Socket file does not exist

**Solution**:
```bash
# Check if server is running
ps aux | grep ai-core

# Check Socket file
ls -la /tmp/ai-core_camera_ctrl

# Ensure server enables camera
./ai-core --enable-camera --enable-jpeg
```

---

### 2. Capture Timeout

**Error**: `ai_core_capture()` returns `AI_MEDIA_ERROR_TIMEOUT`

**Cause**:
- Server camera busy
- Camera hardware failure
- Timeout setting too short

**Solution**:
```c
// Increase timeout
int result = ai_core_capture(client, &data, 10000);  // 10 seconds

// Or no timeout limit
int result = ai_core_capture(client, &data, 0);
```

---

### 3. Image Format Mismatch

**Issue**: Expect JPEG but received NV12

**Cause**: Server did not enable JPEG encoding

**Solution**:
```bash
# Ensure server started with --enable-jpeg
./ai-core --enable-camera --enable-jpeg
```

---

### 4. Memory Leak

**Issue**: Memory usage increases after long running

**Cause**: Forgot to call `ai_core_free_data()`

**Solution**:
```c
// ❌ Error: Memory not freed
ai_core_capture(client, &data, 5000);
// ... use data ...
// Forgot to free!

// ✅ Correct: Always free memory
ai_core_capture(client, &data, 5000);
// ... use data ...
ai_core_free_data(&data);  // Must call
```

---

## ⚙️ System Requirements

### Server Requirements

Server must enable camera mode:
```bash
# Minimal configuration
./ai-core --enable-camera

# Recommended configuration (JPEG + 3A)
./ai-core --enable-camera --enable-jpeg --enable-3a
```

### Compilation Dependencies

- **Cross-compilation Toolchain**: arm-rockchip831-linux-uclibcgnueabihf-gcc
- **System Library**: librt (POSIX real-time extension)

### Runtime Environment

- Linux System
- Sufficient system memory

---

## 📊 Performance Features

| Feature | Description |
|------|------|
| **Low Latency** | Typical capture latency < 100ms |
| **Concurrency Support** | Supports multiple clients capturing simultaneously |
| **Image Format** | Supports JPEG (compressed) and NV12 (raw YUV) |
| **Max Resolution** | 1920x1080 |

---

## 🔗 Related Documentation

- **Example Program**: `ai_glass_sdk/examples/example_media_client/`
- **Header File**: `ai_glass_sdk/include/ai_camera.h`
- **SDK README**: `ai_glass_sdk/README.md`

---

**Version**: v1.0
**Date**: 2025-10-10
**Status**: ✅ Production Ready
