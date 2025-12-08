# Audio Client API Development Guide

[中文版本](Audio_Client_API.md) | English Version

> **Version**: v1.0 | **Date**: 2025-10-10 | **Status**: ✅ Production Ready

---

## 📖 Table of Contents

- [Quick Start](#quick-start)
- [API Reference](#api-reference)
- [Complete Example](#complete-example)
- [Troubleshooting](#troubleshooting)
- [Command Line Tool](#command-line-tool)

---

## 🚀 Quick Start

### Function Introduction

Control audio playback, supports PCM file playback and TTS text-to-speech.

### Compile Client Program

#### Link SDK Library

```bash
# Compile SDK library first
cd ai_glass_sdk
make

# Compile your own program
arm-rockchip831-linux-uclibcgnueabihf-gcc \
    -o my_audio_app \
    my_audio_app.c \
    -I/path/to/ai_glass_sdk/include \
    -L/path/to/ai_glass_sdk/lib \
    -lai_glass_sdk
```

### Run Example Program

```bash
# Compile SDK example program
cd ai_glass_sdk/examples/audio_play_client
make

# Push and run SDK example program
adb push ./audio_play_client /path/you/like
./audio_play_client -f /path/to/audio.pcm -v 80
```

### Minimal Client Code

```c
#include "ai_audio.h"
#include <stdio.h>

int main() {
    // 1. Initialize client
    ai_audio_t *client = ai_audio_init(NULL);  // Use default path
    if (!client) {
        printf("Initialization failed\n");
        return -1;
    }

    // 2. Play audio (use default parameters)
    int result = ai_audio_play_simple(client, "/oem/usr/bin/resources/beep.pcm");
    if (result != AI_AUDIO_SUCCESS) {
        printf("Play failed: %s\n", ai_audio_get_error_string(result));
    }

    // 3. Cleanup resources
    ai_audio_cleanup(client);
    return 0;
}
```

### Advanced Playback Example

```c
#include "ai_audio.h"

int main() {
    ai_audio_t *client = ai_audio_init(NULL);

    // Create playback parameters
    ai_audio_params_t params = {
        .file_path = "/path/to/audio.pcm",
        .volume = 80,              // Volume 80%
        .force = 1,                // Force play (interrupt current)
        .sample_rate = 48000,      // 48kHz sample rate
        .channels = 2,             // Stereo
        .bit_width = 16            // 16-bit
    };

    // Play
    ai_audio_play(client, &params);

    // Cleanup
    ai_audio_cleanup(client);
    return 0;
}
```

### Main Features

- ✅ Simple and easy-to-use C API
- ✅ Supports PCM file playback and TTS text-to-speech
- ✅ Various audio parameter configurations (sample rate, channels, bit width)
- ✅ Force play mode (interrupt current playback)
- ✅ Queued play mode
- ✅ Volume control (0-100)
- ✅ Stop current playback
- ✅ MD5 smart cache (TTS function)

---

## 📋 API Reference

### Data Structures

#### ai_audio_t

Audio client handle (opaque type), created via `ai_audio_init()`.

#### ai_audio_params_t

Audio playback parameter structure:

```c
typedef struct {
    const char *file_path;    // PCM file path (required for PCM playback)
    int volume;               // Volume (0-100), -1 means use default
    int force;                // Force play flag (0=queue, 1=interrupt)
    int sample_rate;          // Sample rate (8000-96000), -1 means use default
    int channels;             // Channels (1-8), -1 means use default
    int bit_width;            // Bit width (8/16/24/32), -1 means use default
} ai_audio_params_t;
```

#### ai_audio_tts_params_t

TTS text-to-speech parameter structure:

```c
typedef struct {
    const char *text;         // Text to convert (required)
    int volume;               // Volume (0-100), -1 means use default
    int force;                // Force play flag (0=queue, 1=interrupt)
    int use_cache;            // Whether to use cache (0=no, 1=yes, recommended)
} ai_audio_tts_params_t;
```

**Field Description**:
- **file_path**: Full path of PCM file (used only for PCM playback)
- **text**: Text content to convert (used only for TTS playback)
- **volume**: Volume percentage, 0=mute, 100=max
- **force**: 0=queue playback, 1=immediately interrupt current playback
- **use_cache**: TTS cache switch, recommended to enable for faster response
- **sample_rate**: Audio sample rate (Hz)
- **channels**: Number of channels
- **bit_width**: Bits per sample

#### Error Codes

```c
#define AI_AUDIO_SUCCESS           0    // Success
#define AI_AUDIO_ERROR_INIT       -1    // Initialization failed
#define AI_AUDIO_ERROR_CONNECT    -2    // Connection failed
#define AI_AUDIO_ERROR_SEND       -3    // Send failed
#define AI_AUDIO_ERROR_PARAM      -4    // Parameter error
#define AI_AUDIO_ERROR_RESPONSE   -5    // Server response error
```

### Core API

#### ai_audio_init()

Initialize audio client.

```c
ai_audio_t* ai_audio_init(const char *socket_path);
```

**Parameters**:
- `socket_path` - Server connection path, pass NULL to use default path

**Return Value**:
- Success: Client handle pointer
- Failure: NULL

**Description**:
- Create client instance, does not establish actual connection
- Connection is dynamically established each time a command is sent

---

#### ai_audio_play()

Play audio file.

```c
int ai_audio_play(ai_audio_t *client, const ai_audio_params_t *params);
```

**Parameters**:
- `client` - Client handle
- `params` - Playback parameters

**Return Value**:
- `AI_AUDIO_SUCCESS` (0) - Success
- `AI_AUDIO_ERROR_CONNECT` (-2) - Connection to server failed
- `AI_AUDIO_ERROR_SEND` (-3) - Send command failed
- `AI_AUDIO_ERROR_PARAM` (-4) - Parameter error
- `AI_AUDIO_ERROR_RESPONSE` (-5) - Server returned error

**Description**:
- `force=0`: Add audio to the end of the playback queue
- `force=1`: Immediately stop current playback, play new audio
- Unspecified parameters (value -1) will use server default values

---

#### ai_audio_stop()

Stop current playback.

```c
int ai_audio_stop(ai_audio_t *client);
```

**Parameters**:
- `client` - Client handle

**Return Value**:
- `AI_AUDIO_SUCCESS` (0) - Success
- Negative number - Error code

**Description**:
- Immediately stop currently playing audio
- Clear playback queue

---

#### ai_audio_cleanup()

Cleanup client resources.

```c
void ai_audio_cleanup(ai_audio_t *client);
```

**Parameters**:
- `client` - Client handle

**Description**:
- Free client memory
- Does not affect server playback state

---

#### ai_audio_get_error_string()

Get error message corresponding to error code.

```c
const char* ai_audio_get_error_string(int error_code);
```

**Parameters**:
- `error_code` - Error code

**Return Value**:
- Error message string

---

### Auxiliary API

#### ai_audio_create_default_params()

Create default playback parameters.

```c
ai_audio_params_t ai_audio_create_default_params(const char *file_path);
```

**Parameters**:
- `file_path` - PCM file path

**Return Value**:
- Parameter structure, all optional parameters set to -1

**Example**:
```c
ai_audio_params_t params = ai_audio_create_default_params("/path/to/audio.pcm");
params.volume = 80;  // Set volume only
params.force = 1;    // Set force play
ai_audio_play(client, &params);
```

---

#### ai_audio_play_tts()

Play TTS text-to-speech.

```c
int ai_audio_play_tts(ai_audio_t *client, const ai_audio_tts_params_t *params);
```

**Parameters**:
- `client` - Client handle
- `params` - TTS playback parameters

**Return Value**:
- `AI_AUDIO_SUCCESS` (0) - Success
- `AI_AUDIO_ERROR_CONNECT` (-2) - Connection to server failed
- `AI_AUDIO_ERROR_SEND` (-3) - Send command failed
- `AI_AUDIO_ERROR_PARAM` (-4) - Parameter error
- `AI_AUDIO_ERROR_RESPONSE` (-5) - Server returned error

**Description**:
- Convert text to speech and play
- Supports smart cache, same text will reuse cache
- `use_cache=0`: Force regenerate, suitable for dynamic content

---

#### ai_audio_play_tts_simple()

Simplified TTS playback function.

```c
int ai_audio_play_tts_simple(ai_audio_t *client, const char *text);
```

**Parameters**:
- `client` - Client handle
- `text` - Text to convert

**Return Value**:
- Error code

**Description**:
- Play TTS using default parameters
- Equivalent to calling `ai_audio_play_tts()` with default TTS parameters

---

#### ai_audio_play_simple()

Simplified PCM playback function.

```c
int ai_audio_play_simple(ai_audio_t *client, const char *file_path);
```

**Parameters**:
- `client` - Client handle
- `file_path` - PCM file path

**Return Value**:
- Error code

**Description**:
- Play PCM file using default parameters
- Equivalent to `ai_audio_play(client, &ai_audio_create_default_params(file_path))`

---

## 💡 Complete Example

### Example 1: Basic Playback

```c
#include "ai_audio.h"
#include <stdio.h>

int main() {
    ai_audio_t *client = ai_audio_init(NULL);
    if (!client) {
        printf("Failed to initialize client\n");
        return -1;
    }

    int result = ai_audio_play_simple(client, "/oem/usr/bin/resources/beep.pcm");
    if (result != AI_AUDIO_SUCCESS) {
        printf("Play failed: %s\n", ai_audio_get_error_string(result));
    } else {
        printf("Playing audio...\n");
    }

    ai_audio_cleanup(client);
    return 0;
}
```

### Example 2: Custom Parameter Playback

```c
#include "ai_audio.h"
#include <stdio.h>

int main() {
    ai_audio_t *client = ai_audio_init(NULL);

    // Configure playback parameters
    ai_audio_params_t params = {
        .file_path = "/data/alert.pcm",
        .volume = 90,
        .force = 1,           // Force play
        .sample_rate = 48000,
        .channels = 2,
        .bit_width = 16
    };

    printf("Playing audio with custom parameters...\n");
    int result = ai_audio_play(client, &params);
    if (result != AI_AUDIO_SUCCESS) {
        printf("Error: %s\n", ai_audio_get_error_string(result));
    }

    ai_audio_cleanup(client);
    return 0;
}
```

### Example 3: Play Multiple Audio Files

```c
#include "ai_audio.h"
#include <stdio.h>
#include <unistd.h>

int main() {
    ai_audio_t *client = ai_audio_init(NULL);

    const char *files[] = {
        "/data/sound1.pcm",
        "/data/sound2.pcm",
        "/data/sound3.pcm"
    };

    for (int i = 0; i < 3; i++) {
        printf("Playing %s...\n", files[i]);

        ai_audio_params_t params = ai_audio_create_default_params(files[i]);
        params.force = 0;  // Queue play, do not interrupt

        int result = ai_audio_play(client, &params);
        if (result != AI_AUDIO_SUCCESS) {
            printf("Failed to play %s: %s\n", files[i],
                   ai_audio_get_error_string(result));
        }

        sleep(1);  // Wait 1 second before adding next
    }

    ai_audio_cleanup(client);
    return 0;
}
```

### Example 4: TTS Text-to-Speech

```c
#include "ai_audio.h"
#include <stdio.h>

int main() {
    ai_audio_t *client = ai_audio_init(NULL);

    // Simple TTS playback
    printf("Playing TTS...\n");
    int result = ai_audio_play_tts_simple(client, "Hello, welcome to AI Voice Assistant");
    if (result != AI_AUDIO_SUCCESS) {
        printf("TTS failed: %s\n", ai_audio_get_error_string(result));
    }

    // TTS playback with parameters
    ai_audio_tts_params_t tts_params = {
        .text = "This is an important notification",
        .volume = 90,
        .force = 0,      // Queue play
        .use_cache = 1   // Use cache
    };

    ai_audio_play_tts(client, &tts_params);

    // Dynamic content (disable cache)
    ai_audio_tts_params_t dynamic_params = {
        .text = "Current time: October 16, 2025",
        .volume = 80,
        .force = 0,
        .use_cache = 0   // Disable cache because content is dynamic
    };

    ai_audio_play_tts(client, &dynamic_params);

    ai_audio_cleanup(client);
    return 0;
}
```

### Example 5: Emergency Playback (Interrupt Current)

```c
#include "ai_audio.h"
#include <stdio.h>

void play_alert(ai_audio_t *client) {
    // Play alert sound using PCM file
    ai_audio_params_t params = {
        .file_path = "/oem/usr/bin/resources/alert.pcm",
        .volume = 100,        // Max volume
        .force = 1,           // Immediately interrupt current playback
        .sample_rate = 16000,
        .channels = 1,
        .bit_width = 16
    };

    ai_audio_play(client, &params);
}

void play_alert_tts(ai_audio_t *client) {
    // Play emergency message using TTS
    ai_audio_tts_params_t tts_params = {
        .text = "Warning! Abnormal situation detected",
        .volume = 100,
        .force = 1,           // Immediately interrupt current playback
        .use_cache = 1
    };

    ai_audio_play_tts(client, &tts_params);
}

int main() {
    ai_audio_t *client = ai_audio_init(NULL);

    // Play background music normally
    ai_audio_play_simple(client, "/data/background.pcm");

    // Simulate emergency situation
    sleep(2);
    printf("Alert! Playing emergency sound...\n");
    play_alert(client);  // Will interrupt background music

    sleep(3);
    printf("Alert TTS! Playing emergency message...\n");
    play_alert_tts(client);  // Play emergency message using TTS

    ai_audio_cleanup(client);
    return 0;
}
```

---

## 🔧 Troubleshooting

### 1. Client Initialization Failure

**Error**: `ai_audio_init()` returns NULL

**Cause**:
- Memory allocation failed

**Solution**:
- Check system available memory
- Confirm no memory leak

---

### 2. Connection to Server Failed

**Error**: API function returns `AI_AUDIO_ERROR_CONNECT`

**Cause**:
- Server not started
- Connection path error

**Solution**:
```bash
# Check if server is running
ps aux | grep ai-core
```

---

### 3. TTS Playback Failure

**Error**: TTS function returns non-zero value

**Cause**:
- TTS server unreachable
- Network connection issue
- Text encoding issue

**Solution**:
- Check network connection and TTS server configuration
- Ensure text uses UTF-8 encoding
- See [TTS Client API](TTS_Client_API.en.md) for detailed TTS configuration (Note: File might be missing)

---

### 4. Parameter Error

**Error**: Function returns `AI_AUDIO_ERROR_PARAM`

**Cause**:
- Parameter value out of range
- Required parameter is empty

**Solution**:
- Confirm parameters are within valid range:
  - volume: 0-100
  - sample_rate: 8000-96000
  - channels: 1-8
  - bit_width: 8/16/24/32
- Ensure required parameters (file_path or text) are not empty

---

## ⚙️ System Requirements

### Prerequisites

Server (ai-core) must be started.

### Audio File Requirements

- **Format**: PCM (Raw audio)
- **Encoding**: Uncompressed
- **Sample Rate**: 8000-96000 Hz
- **Channels**: 1-8
- **Bit Width**: 8/16/24/32 bit

### Compilation Dependencies

- **Cross-compilation Toolchain**: arm-rockchip831-linux-uclibcgnueabihf-gcc
- **System Library**: Standard C Library

---

## 📊 Performance Features

| Feature | Description |
|------|------|
| **Connection Mode** | Short connection (new connection established for each command) |
| **Response Latency** | < 50ms |
| **Concurrency Support** | Supports multiple clients |
| **Playback Mode** | Queue Play / Force Play |
| **Audio Format** | PCM (Uncompressed) |
| **Sample Rate Range** | 8kHz - 96kHz |

---

## 🛠️ Command Line Tool

Besides programming interface, SDK also provides convenient command line tools:

### Quick Usage

```bash
# PCM file playback
./audio_play_client -f /path/to/audio.pcm -v 80

# TTS text playback
./audio_play_client -t "Hello World" -v 90

# Force play (interrupt current)
./audio_play_client -f /tmp/urgent.pcm -F

# Stop playback
./audio_play_client -S
```

### Detailed Description

For complete command line tool usage instructions, please refer to:
**📖 [Audio Play Client User Guide](../examples/audio_play_client/README.md)**

---

## 🔗 Related Documentation

- **Example Program**: `ai_glass_sdk/examples/audio_play_client/`
- **Header File**: `ai_glass_sdk/include/ai_audio.h`
- **TTS Function**: [TTS Client API](TTS_Client_API.en.md)
- **SDK README**: `ai_glass_sdk/README.md`

---

**Version**: v1.0
**Date**: 2025-10-10
**Status**: ✅ Production Ready
