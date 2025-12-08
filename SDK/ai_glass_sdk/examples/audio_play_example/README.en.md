# Audio Play Example User Guide

[中文版本](README.md) | English Version

## Introduction

`audio_play_example` is a command-line tool for controlling AI Core's audio playback function.

### Supported Playback Modes
1. **PCM File Playback** - Play local PCM format audio files
2. **TTS Text Playback** - Text-to-speech playback, supports intelligent caching

### Core Features
- ✅ Simple and easy-to-use command-line interface
- ✅ Supports volume control and force playback
- ✅ TTS intelligent caching for faster response
- ✅ Rich usage scenarios and script integration
- ✅ Complete error handling and status feedback

## Compilation Tool
```bash
cd ai_glass_sdk/examples/audio_play_example
make
```

## Basic Usage

### Command Format
```bash
# PCM File Playback
audio_play_example -f <pcm_file> [options]

# TTS Text Playback
audio_play_example -t <text> [options]

# Stop Playback
audio_play_example -S
```

## Complete Parameter Description

### PCM File Playback Parameters
| Parameter | Long Format | Description | Example |
|------|--------|------|------|
| `-f` | `--file` | PCM file path (required) | `-f /tmp/audio.pcm` |
| `-r` | `--rate` | Sample rate (Hz) | `-r 48000` |
| `-c` | `--channels` | Channels (1-8) | `-c 2` |
| `-b` | `--bitwidth` | Bit width (8/16/24/32) | `-b 16` |

### TTS Text Playback Parameters
| Parameter | Long Format | Description | Example |
|------|--------|------|------|
| `-t` | `--tts` | Text to speak (required) | `-t "Hello"` |
|  | `--no-cache` | Disable cache, regenerate every time | `--no-cache` |

### General Parameters
| Parameter | Long Format | Description | Default |
|------|--------|------|--------|
| `-v` | `--volume` | Volume (0-100) | Use server default |
| `-F` | `--force` | Force play (interrupt current) | Queue play |
| `-s` | `--socket` | Unix Socket path | `/tmp/ai-core_audio_ctrl` |
| `-S` | `--stop` | Stop current playback | - |
| `-h` | `--help` | Show help info | - |

## Usage Examples

### 1. PCM File Playback

#### Basic Playback
```bash
# Play with default parameters
./audio_play_example -f /tmp/audio.pcm
```

#### Specify Audio Parameters
```bash
# Specify sample rate and volume
./audio_play_example -f /tmp/audio.pcm -r 48000 -v 80

# Complete parameters
./audio_play_example -f /tmp/music.pcm -r 48000 -c 2 -b 16 -v 90
```

#### Force Play (Interrupt Current)
```bash
./audio_play_example -f /tmp/urgent.pcm -F
```

### 2. TTS Text Playback

#### Simple Playback
```bash
# Play Chinese text
./audio_play_example -t "你好，欢迎使用AI语音助手"

# Play English text
./audio_play_example -t "Hello World"
```

#### Force Play Important Notification
```bash
# Interrupt current playback, speak immediately
./audio_play_example -t "Important notice: System will restart in 5 minutes" -F
```

#### High Volume Playback
```bash
./audio_play_example -t "Alert! Anomaly detected" -v 100 -F
```

#### Disable Cache (Regenerate Every Time)
```bash
# Suitable for dynamic content, like time, temperature
./audio_play_example -t "Current time is 10:30 AM" --no-cache

# Combine with other parameters
./audio_play_example -t "Outdoor temperature 25 degrees" --no-cache -v 80
```

### 3. Playback Control

#### Stop Playback
```bash
./audio_play_example -S
```

#### Use Custom Socket Path
```bash
./audio_play_example -s /tmp/custom_socket -t "Test message"
```

### 4. Batch Playback

#### Play Multiple TTS Sequentially
```bash
#!/bin/bash
# Batch playback script
./audio_play_example -t "First message"
sleep 1
./audio_play_example -t "Second message"
sleep 1
./audio_play_example -t "Third message"
```

#### Loop Playback
```bash
#!/bin/bash
# Loop time announcement
while true; do
    current_time=$(date "+%H:%M")
    ./audio_play_example -t "Current time: ${current_time}" --no-cache
    sleep 60  # Announce every minute
done
```

## Common Scenarios

### Scenario 1: Smart Doorbell Announcement
```bash
# Announce when visitor presses doorbell
./audio_play_example -t "Visitor at the door" -F

# Package arrival
./audio_play_example -t "Your package has arrived, please pick it up" -v 90
```

### Scenario 2: Device Status Reminder
```bash
# System startup
./audio_play_example -t "System initialization complete, ready"

# Low battery reminder
./audio_play_example -t "Low battery, please charge" -F -v 100

# Temperature anomaly
./audio_play_example -t "Temperature too high, please check device" -F --no-cache
```

### Scenario 3: Scheduled Broadcast
```bash
#!/bin/bash
# Hourly chime
hour=$(date "+%H")
./audio_play_example -t "It is now ${hour} o'clock" --no-cache -v 70
```

### Scenario 4: Mixed Playback (TTS + Sound Effect)
```bash
# Play prompt sound first, then TTS
./audio_play_example -f /tmp/beep.pcm
sleep 1
./audio_play_example -t "Operation successful"
```

## Return Values

| Value | Description |
|--------|------|
| `0` | Success |
| `Non-0` | Failure (Parameter error, service not started, network issue, etc.) |

## Output Examples

### Successful TTS Playback
```
Playing TTS:
  Text: "Hello, welcome to AI Voice Assistant"
  Mode: Queue play
  Cache: Enabled
✅ TTS playback started
```

### Force Play PCM
```
Playing audio:
  File: /tmp/urgent.pcm
  Volume: 90%
  Mode: Force play (interrupt current)
  Sample rate: 48000 Hz
  Channels: 2
  Bit width: 16
✅ Audio playback started
```

### Stop Playback
```
Stopping audio playback...
✅ Audio stopped
```

## Error Handling

### Common Errors and Solutions

#### 1. Service Not Started
```
ERROR: Failed to initialize audio client
```
**Solution**: Start `ai-core` service first

#### 2. Socket Path Not Found
```
ERROR: Failed to connect to socket
```
**Solution**: Check if Socket path is correct, default is `/tmp/ai-core_audio_ctrl`

#### 3. File Not Found
```
ERROR: File not found
```
**Solution**: Confirm PCM file path is correct and file exists

#### 4. Parameter Conflict
```
ERROR: Cannot use both --file and --tts at the same time
```
**Solution**: Use only one playback mode at a time (PCM file or TTS)

#### 5. TTS Server Unreachable
```
❌ Failed to play TTS: API call failed
```
**Solution**:
- Check network connection
- Confirm TTS server address configuration (Server config: `--tts-server` and `--tts-port`)
- Check if text encoding is UTF-8

---

## 🔧 Programming Interface

If you need to integrate audio playback function in your program, you can use the programming interface provided by the SDK:

### Basic C Language Example

```c
#include "ai_audio.h"

// Initialize client
ai_audio_t *client = ai_audio_init(NULL);

// PCM file playback
ai_audio_play_simple(client, "/path/to/audio.pcm");

// TTS text playback
ai_audio_play_tts_simple(client, "Hello World");

// Cleanup resources
ai_audio_cleanup(client);
```

### Detailed API Documentation

For complete programming interface documentation, please refer to:
**📚 [Audio Client API Development Guide](../../docs/Audio_Client_API.en.md)**

This document contains:
- Detailed API function descriptions
- Data structure definitions
- Complete C code examples
- Error handling and debugging tips

## Advanced Usage

### Integrate into Script
```bash
#!/bin/bash
# Smart reminder script

play_tts() {
    ./audio_play_example -t "$1" -v ${2:-80}
}

# Use function
play_tts "Good morning" 70
sleep 2
play_tts "It's sunny today" 70
```

### Error Handling Example
```bash
#!/bin/bash
if ./audio_play_example -t "Test message"; then
    echo "Playback success"
else
    echo "Playback failed, error code: $?"
    # Handle error...
fi
```

### Integrate with Other Programs
```bash
# Monitor file changes and announce
inotifywait -m /path/to/watch -e create |
while read path action file; do
    ./audio_play_example -t "New file detected: ${file}"
done
```

## Performance Tips

1. **Use Cache**: For repeated text, enabling cache can significantly improve response speed
2. **Batch Playback**: Add appropriate delay between multiple messages to avoid loss
3. **Force Play**: Use only for urgent notifications to avoid frequent interruption of normal playback

## Notes

1. This tool needs to run on the target ARM device, cannot execute directly on x86 host
2. TTS function requires network connection to TTS server (Default: xiaoyunyun.com:7863)
3. PCM file must be raw PCM format, does not support MP3, WAV, etc.
4. Volume range 0-100, out of range will be automatically limited
5. TTS cache directory is `/oem/usr/bin/resources/cache`, ensure write permission

## Related Commands

- View Help: `./audio_play_example --help`
- View Version: `file ./audio_play_example`
- Check Service: `ps aux | grep ai-core`
- View Socket: `ls -la /tmp/ai-core_audio_ctrl`

## 🔗 Related Documentation

- **Programming Interface**: [Audio Client API Development Guide](../../docs/Audio_Client_API.en.md)
- **TTS Function**: [TTS Client API](../../docs/TTS_Client_API.en.md) (Note: File might be missing)
- **SDK Documentation**: `../../README.en.md`
- **Header File**: `../../include/ai_audio.h`
