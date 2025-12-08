# Log System API - User Guide

[中文版本](Log_API.md) | English Version

> **Version**: v1.0 | **Date**: 2025-10-27 | **Status**: ✅ Production Ready

---

## 📖 Table of Contents

- [Quick Start](#quick-start)
- [API Reference](#ii-api-reference)
- [Usage Examples](#iii-usage-examples)
- [Integration Methods](#iv-integration-methods)
- [Output Format](#v-output-format)
- [Notes](#vi-notes)

---

## 🚀 Quick Start

### Function Introduction

Unified log output system, automatically adds **millisecond-precision timestamps** to all logs. Supports four log levels: info, error, debug, warn.

### Main Features

- ✅ **Millisecond Timestamp** - Time recording precise to milliseconds (Format: `YYYY-MM-DD HH:MM:SS.mmm`)
- ✅ **Multi-Log Levels** - Supports info, error, debug, warn four levels
- ✅ **Simple to Use** - API is completely consistent with printf, no learning cost
- ✅ **No Encoding Issues** - Uses vprintf directly, supports all characters including Chinese and emoji
- ✅ **Zero Dependency** - Only depends on standard C library, no external dependencies
- ✅ **Thread Safe** - Based on standard output, thread safe

### Minimal Example

```c
#include "ai_log.h"

int main() {
    log_info("✅ Program started successfully\n");
    log_error("❌ Connection failed: %s\n", "Connection refused");
    log_debug("📡 Received data: %d bytes\n", 1024);
    log_warn("⚠️  Memory usage: %d%%\n", 85);
    return 0;
}
```

**Output Effect**:
```
[2025-10-27 14:23:45.123] ✅ Program started successfully
[2025-10-27 14:23:45.456] ❌ Connection failed: Connection refused
[2025-10-27 14:23:45.789] 📡 Received data: 1024 bytes
[2025-10-27 14:23:46.012] ⚠️  Memory usage: 85%
```

---

## II. API Reference

### 2.1 Header File

```c
#include "ai_log.h"
```

**Header File Path**: `ai_glass_sdk/include/ai_log.h`

### 2.2 Log Functions

#### `log_info()` - Info Log

```c
void log_info(const char *fmt, ...);
```

**Function**: Output info level log

**Parameters**:
- `fmt` - Format string (same as printf format)
- `...` - Variable argument list

**Example**:
```c
log_info("✅ Audio playback success\n");
log_info("Connected to server: %s:%d\n", "192.168.1.100", 8080);
log_info("Processing complete, total %d files\n", file_count);
```

---

#### `log_error()` - Error Log

```c
void log_error(const char *fmt, ...);
```

**Function**: Output error level log

**Parameters**:
- `fmt` - Format string (same as printf format)
- `...` - Variable argument list

**Example**:
```c
log_error("❌ File open failed: %s\n", strerror(errno));
log_error("Memory allocation failed, need %zu bytes\n", required_size);
log_error("Socket connection timeout\n");
```

---

#### `log_debug()` - Debug Log

```c
void log_debug(const char *fmt, ...);
```

**Function**: Output debug level log

**Parameters**:
- `fmt` - Format string (same as printf format)
- `...` - Variable argument list

**Example**:
```c
log_debug("📡 Received message: type=%d, length=%d\n", msg_type, msg_len);
log_debug("Internal state: state=%d, retry=%d\n", current_state, retry_count);
log_debug("Performance stats: elapsed %ld ms\n", elapsed_time);
```

---

#### `log_warn()` - Warning Log

```c
void log_warn(const char *fmt, ...);
```

**Function**: Output warning level log

**Parameters**:
- `fmt` - Format string (same as printf format)
- `...` - Variable argument list

**Example**:
```c
log_warn("⚠️  Buffer usage too high: %d%%\n", usage_percent);
log_warn("Network latency detected: %d ms\n", latency);
log_warn("Config file not found, using default config\n");
```

---

## III. Usage Examples

### 3.1 Replace Existing printf

**Before**:
```c
printf("✅ [AUDIO_PLAYER] Audio player created successfully\n");
printf("❌ [TTS_CACHE] Create Socket failed: %s\n", strerror(errno));
printf("📡 [DEBUG-SELECT] Start waiting for socket data...\n");
```

**After**:
```c
log_info("✅ [AUDIO_PLAYER] Audio player created successfully\n");
log_error("❌ [TTS_CACHE] Create Socket failed: %s\n", strerror(errno));
log_debug("📡 [DEBUG-SELECT] Start waiting for socket data...\n");
```

### 3.2 Complete Client Program Example

```c
#include "ai_log.h"
#include "ai_audio.h"
#include <errno.h>
#include <string.h>

int main() {
    ai_audio_client_t client;
    int ret;

    log_info("=== Audio Client Start ===\n");

    // Initialize client
    ret = ai_audio_init(&client);
    if (ret != 0) {
        log_error("❌ Initialization failed: error code %d\n", ret);
        return -1;
    }
    log_info("✅ Client initialized successfully\n");

    // Play audio
    log_info("Start playing audio file...\n");
    ret = ai_audio_play(&client, "/tmp/test.pcm", 80, 48000, 2, 16, 0);
    if (ret != 0) {
        log_error("❌ Play failed: %s\n", ai_audio_get_error_string(ret));
        ai_audio_cleanup(&client);
        return -1;
    }
    log_info("✅ Audio playback success\n");

    // Cleanup resources
    ai_audio_cleanup(&client);
    log_info("=== Program exited normally ===\n");

    return 0;
}
```

**Output Effect**:
```
[2025-10-27 14:23:45.001] === Audio Client Start ===
[2025-10-27 14:23:45.015] ✅ Client initialized successfully
[2025-10-27 14:23:45.018] Start playing audio file...
[2025-10-27 14:23:46.123] ✅ Audio playback success
[2025-10-27 14:23:46.125] === Program exited normally ===
```

### 3.3 Performance Debugging Example

```c
#include "ai_log.h"
#include <sys/time.h>

long get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void process_data(const char *data, size_t len) {
    long start = get_time_ms();

    log_debug("Start processing data: %zu bytes\n", len);

    // Process data...

    long elapsed = get_time_ms() - start;
    if (elapsed > 100) {
        log_warn("⚠️  Processing took too long: %ld ms\n", elapsed);
    } else {
        log_debug("Processing complete: %ld ms\n", elapsed);
    }
}
```

### 3.4 Error Handling Example

```c
#include "ai_log.h"
#include <errno.h>
#include <string.h>
#include <fcntl.h>

int open_config_file(const char *path) {
    log_debug("Attempting to open config file: %s\n", path);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        log_error("❌ Failed to open config file: %s (Error: %s)\n",
                  path, strerror(errno));
        return -1;
    }

    log_info("✅ Config file opened successfully: %s (fd=%d)\n", path, fd);
    return fd;
}
```

---

## IV. Integration Methods

### 4.1 SDK Library Includes Log System

The log system is already compiled into the AI Glass SDK library, no extra compilation needed.

**Library Files**:
- Static Library: `ai_glass_sdk/lib/libai_glass_sdk.a`
- Dynamic Library: `ai_glass_sdk/lib/libai_glass_sdk.so`

### 4.2 Compile Your Own Program

#### Link SDK Library

```bash
arm-rockchip831-linux-uclibcgnueabihf-gcc \
    -o my_app my_app.c \
    -I/path/to/ai_glass_sdk/include \
    -L/path/to/ai_glass_sdk/lib \
    -lai_glass_sdk \
    -lpthread -lrt
```

### 4.3 Use in Project

**Steps**:
1. Include header: `#include "ai_log.h"`
2. Use log functions: `log_info()`, `log_error()`, etc.
3. Link SDK library when compiling

**Note**: Both server and client can use the log system.

---

## V. Output Format

### 5.1 Timestamp Format

```
[YYYY-MM-DD HH:MM:SS.mmm] Log Content
```

**Description**:
- `YYYY-MM-DD` - Year-Month-Day
- `HH:MM:SS` - Hour:Minute:Second (24-hour)
- `mmm` - Millisecond (000-999)

### 5.2 Example Output

```
[2025-10-27 14:23:45.123] ✅ Program started successfully
[2025-10-27 14:23:45.456] Connected to server: 192.168.1.100:8080
[2025-10-27 14:23:46.789] 📡 Received response: 1024 bytes
[2025-10-27 14:23:47.012] ❌ Parse failed: Invalid JSON
[2025-10-27 14:23:47.234] Retrying... (1/3)
```

### 5.3 Time Precision

- **Precision**: Millisecond (1ms)
- **Implementation**: Uses `clock_gettime(CLOCK_REALTIME)` to get nanosecond time
- **Timezone**: Local timezone (uses `localtime_r` conversion)

---

## VI. Notes

### 6.1 Character Encoding

✅ **Fully compatible with original printf encoding**
- Supports Chinese, English, emoji, and all characters
- No encoding conversion performed
- No garbled characters

**Mechanism**: Uses `vprintf()` for direct output, only prepends timestamp string.

### 6.2 Thread Safety

✅ **Basic Thread Safety**
- `printf()` itself is thread-safe (has internal lock)
- Multi-thread calls to log functions will not crash
- However, simultaneous output from multiple threads may interleave (normal behavior of printf)

### 6.3 Performance Overhead

**Time Overhead**:
- Each log call adds about **1-2 microseconds** (getting timestamp)
- Negligible for embedded systems

**Recommendation**:
- Reduce `log_debug()` calls in performance-critical paths
- Overhead of normal `log_info()` and `log_error()` is negligible

### 6.4 Output Target

Currently all logs output to **Standard Output (stdout)**.

**Future Extensions**:
- Output to file
- Output to syslog
- Dynamic log level control
- Add log filtering

### 6.5 Integration with Existing Code

**Smooth Replacement**:
1. Compile log system first (done)
2. `#include "ai_log.h"` in needed files
3. Gradually replace `printf()` with `log_info()`, etc.
4. Can replace progressively, new and old code can coexist

**Replacement Suggestions**:
- ✅ Success/Completion info → `log_info()`
- ❌ Error/Failure info → `log_error()`
- 📡 Debug/Performance info → `log_debug()`
- ⚠️  Warning/Hint info → `log_warn()`

### 6.6 Format Strings

**Fully compatible with printf format**:
```c
// String
log_info("Message: %s\n", str);

// Integer
log_info("Count: %d, Size: %zu\n", count, size);

// Hex
log_debug("Address: 0x%p, Value: 0x%08X\n", ptr, value);

// Float
log_info("Progress: %.2f%%\n", progress);
```

---

## VII. FAQ

### Q1: Will timestamp affect performance?

**Ans**: Overhead is minimal (1-2 microseconds), negligible for embedded systems.

### Q2: Supports Chinese and emoji?

**Ans**: Fully supported. Log system does no encoding conversion, outputs as is.

### Q3: Can I disable a certain log level?

**Ans**: Not supported in current version. Future versions may add log level control.

### Q4: Will logs be saved to file?

**Ans**: Current version outputs to standard output. Can save via redirection:
```bash
./my_app > app.log 2>&1
```

### Q5: Will logs be messed up in multi-thread environment?

**Ans**: `printf()` is thread-safe, will not crash. But simultaneous output from multiple threads may interleave, which is normal.

---

## VIII. Version History

### v1.0 (2025-10-27)
- ✅ Initial release
- ✅ Supports millisecond timestamp
- ✅ Provides info, error, debug, warn four log levels
- ✅ Integrated into AI Glass SDK library

---

## IX. Related Links

- **Main SDK Docs**: [AI Glass SDK README](../README.md)
- **API Header**: `ai_glass_sdk/include/ai_log.h`
- **Other API Docs**:
  - [GPIO Client API](GPIO_Client_API.en.md)
  - [Camera Client API](Camera_Client_API.en.md)
  - [Audio Client API](Audio_Client_API.en.md)
  - [TTS Client API](TTS_Client_API.en.md)

---

*Last Updated: 2025-10-27*
