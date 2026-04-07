# Camera Capture Example User Guide

[中文版本](README.md) | English Version

## Introduction

`camera_capture_example` is a demo program showing how to use AI Camera SDK to capture camera images from shared memory.

### Core Features
- Read camera data from shared memory
- Support JPEG and NV12 formats
- Automatically save captured images to file
- Timed capture (default every 3 seconds)
- Complete error handling and status feedback

## How It Works

### Architecture
```
┌─────────────────┐
│   ai-core Service│
│  Camera Service │
│   (Writes SHM)  │
22: └────────┬────────┘
23:          │ Shared Memory
24:          │ /ai-core_shm
25:     ┌────▼────┐
26:     │  Client  │
27:     │ (Reads SHM) │
28:     └─────────┘
```

1. **Server** (ai-core): Captures images from camera, writes to shared memory
2. **Shared Memory**: High-performance IPC mechanism
3. **Client**: Reads shared memory data and saves to file

## Compilation

```bash
cd ai_glass_sdk/examples/camera_capture_example
make
```

Output: `camera_capture_example`

## Prerequisites

### 1. Start AI Core Service (Enable Camera)
```bash
cd service
./build/ai-core --enable-camera
```

### 2. Confirm Camera Availability
Ensure camera is connected and driver is working.

## Usage

### Basic Run (Save to /tmp)
```bash
./camera_capture_example
```

### Specify Save Path
```bash
./camera_capture_example /path/to/save
```

## Program Output Example

### Startup
```
🚀 [EXAMPLE] AI Media Client Example Starting...
📁 [EXAMPLE] Media data will be saved to: /tmp
💡 [EXAMPLE] Press Ctrl+C to exit

📸 [EXAMPLE] Capturing media data #1...
```

### Successful Capture
```
✅ [EXAMPLE] Capture successful:
   Size: 245678 bytes
   Resolution: 1920x1080
   Format: JPEG
   Sequence: 1
💾 [EXAMPLE] Media data saved to: /tmp/capture_001.jpg
```

### Continuous Capture
Program captures every 3 seconds:
```
📸 [EXAMPLE] Capturing media data #1...
✅ [EXAMPLE] Capture successful:
   Size: 245678 bytes
   Resolution: 1920x1080
   Format: JPEG
   Sequence: 1
💾 [EXAMPLE] Media data saved to: /tmp/capture_001.jpg

📸 [EXAMPLE] Capturing media data #2...
✅ [EXAMPLE] Capture successful:
   Size: 248123 bytes
   Resolution: 1920x1080
   Format: JPEG
   Sequence: 2
💾 [EXAMPLE] Media data saved to: /tmp/capture_002.jpg
```

### Exit Program
Press `Ctrl+C` to exit:
```
^C
🛑 [EXAMPLE] Received signal 2, exiting...

📊 [EXAMPLE] Total captures: 5
✅ [EXAMPLE] AI Media Client Example Finished
```

## Captured File Formats

### JPEG Format
```bash
# Filename format
capture_001.jpg
capture_002.jpg
capture_003.jpg
...

# View directly
eog capture_001.jpg       # Linux
open capture_001.jpg      # macOS
```

### NV12 Format (Raw YUV Data)
```bash
# Filename format
capture_001.nv12
capture_002.nv12
...

# View with specialized tool (e.g., ffplay)
ffplay -f rawvideo -pixel_format nv12 -video_size 1920x1080 capture_001.nv12
```

## Usage Scenarios

### Scenario 1: Timed Photo
```c
#include "ai_camera.h"

int main(void) {
    ai_core_client_t *client = ai_core_init();

    while (1) {
        ai_core_data_t data;
        if (ai_core_capture(client, &data, 5000) == AI_MEDIA_SUCCESS) {
            // Save image
            save_image_to_file(&data, "/tmp/photo.jpg");
            ai_core_free_data(&data);
        }
        sleep(60);  // Photo every minute
    }

    ai_core_cleanup(client);
    return 0;
}
```

### Scenario 2: On-demand Photo
```c
int take_photo(const char *filename) {
    ai_core_client_t *client = ai_core_init();
    if (!client) return -1;

    ai_core_data_t data;
    int result = ai_core_capture(client, &data, 5000);

    if (result == AI_MEDIA_SUCCESS) {
        FILE *fp = fopen(filename, "wb");
        fwrite(data.data, 1, data.size, fp);
        fclose(fp);
        ai_core_free_data(&data);
    }

    ai_core_cleanup(client);
    return result;
}

// Usage
take_photo("/tmp/snapshot.jpg");
```

### Scenario 3: Image Analysis
```c
void analyze_images(void) {
    ai_core_client_t *client = ai_core_init();

    for (int i = 0; i < 10; i++) {
        ai_core_data_t data;
        if (ai_core_capture(client, &data, 5000) == AI_MEDIA_SUCCESS) {
            // Analyze image (e.g., face detection, object recognition)
            analyze_image_data(data.data, data.size, data.width, data.height);
            ai_core_free_data(&data);
        }
        sleep(1);
    }

    ai_core_cleanup(client);
}
```

### Scenario 4: Motion Detection
```c
void motion_detection(void) {
    ai_core_client_t *client = ai_core_init();
    ai_core_data_t prev_data = {0}, curr_data;

    while (1) {
        if (ai_core_capture(client, &curr_data, 5000) == AI_MEDIA_SUCCESS) {
            if (prev_data.data) {
                // Compare current frame with previous frame
                if (detect_motion(&prev_data, &curr_data)) {
                    printf("Motion detected!\n");
                    save_image_to_file(&curr_data, "/tmp/motion_detected.jpg");
                }
                ai_core_free_data(&prev_data);
            }
            prev_data = curr_data;
        }
        usleep(100000);  // 100ms
    }

    ai_core_cleanup(client);
}
```

## Error Handling

### Common Errors and Solutions

#### 1. Capture Failed - Service Unavailable
```
❌ [EXAMPLE] Capture failed: Initialization error
🛑 [EXAMPLE] Service unavailable, exiting...
```
**Solution**:
- Check if `ai-core` service is running
- Confirm started with `--enable-camera` parameter

#### 2. Capture Timeout
```
❌ [EXAMPLE] Capture failed: Timeout
```
**Solution**:
- Camera might not be initialized correctly
- Check camera hardware connection
- Check server logs for detailed error

#### 3. File Save Failed
```
❌ [EXAMPLE] Failed to save data to /tmp/capture_001.jpg: Permission denied
```
**Solution**:
- Check write permission of save path
- Ensure sufficient disk space

#### 4. Partial Data Write
```
⚠️ [EXAMPLE] Warning: Only wrote 100000/245678 bytes to /tmp/capture_001.jpg
```
**Solution**:
- Check disk space
- Check file system status

## Programming Interface

### Core API Functions

```c
// 1. Initialize media client
ai_core_client_t* ai_core_init(void);

// 2. Capture media data
int ai_core_capture(ai_core_client_t *client,
                   ai_core_data_t *data,
                   int timeout_ms);

// 3. Free media data
void ai_core_free_data(ai_core_data_t *data);

// 4. Cleanup client
void ai_core_cleanup(ai_core_client_t *client);

// 5. Get error description
const char* ai_core_get_error_string(int error_code);
```

### Data Structures

```c
// Media data structure
typedef struct {
    uint8_t *data;          // Image data pointer
    size_t size;            // Data size (bytes)
    int width;              // Image width
    int height;             // Image height
    ai_media_format_t format;  // Format (JPEG/NV12)
    int sequence;           // Sequence number
} ai_core_data_t;

// Media format
typedef enum {
    AI_MEDIA_FORMAT_JPEG = 0,  // JPEG compressed format
    AI_MEDIA_FORMAT_NV12 = 1   // NV12 YUV format
} ai_media_format_t;
```

### Error Codes

```c
#define AI_MEDIA_SUCCESS         0   // Success
#define AI_MEDIA_ERROR_INIT     -1   // Initialization error
#define AI_MEDIA_ERROR_TIMEOUT  -2   // Timeout
#define AI_MEDIA_ERROR_INVALID  -3   // Invalid parameter
#define AI_MEDIA_ERROR_NO_DATA  -4   // No data
```

### Complete API Documentation
For detailed programming interface documentation, please refer to:
**📚 [Camera Client API Development Guide](../../docs/Camera_Client_API.en.md)**

## Performance Features

### Advantages
- **Zero Copy**: Use shared memory, avoid data copy
- **Low Latency**: Read directly from shared memory, latency < 10ms
- **High Throughput**: Support high frame rate image transmission (e.g., 30fps)

### Performance Data (Reference)
- **JPEG 1920x1080**: Approx 200-300KB, capture latency < 5ms
- **NV12 1920x1080**: Approx 3MB, capture latency < 10ms

## Debugging Tips

### 1. Check Shared Memory
```bash
# View shared memory object
ls -lh /dev/shm/ai-core_shm

# Check permission and size
```

### 2. Monitor Capture Rate
Modify source code to add performance statistics:
```c
struct timeval start, end;
gettimeofday(&start, NULL);

ai_core_capture(client, &data, 5000);

gettimeofday(&end, NULL);
long elapsed_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
printf("Capture time: %ld us\n", elapsed_us);
```

### 3. View Server Log
Server will output camera status and shared memory update logs.

### 4. Verify Image Integrity
```bash
# Check if JPEG file is complete
file capture_001.jpg
jpeginfo capture_001.jpg

# View image info
identify capture_001.jpg  # ImageMagick tool
```

## Timeout Setting

`ai_core_capture()` timeout parameter (ms):
- **-1**: Wait forever (not recommended)
- **0**: Return immediately (non-blocking)
- **> 0**: Specify timeout (recommended: 3000-5000ms)

```c
// Recommended setting
ai_core_capture(client, &data, 5000);  // 5 seconds timeout

// Non-blocking mode
ai_core_capture(client, &data, 0);     // Return immediately

// Wait forever (use with caution)
ai_core_capture(client, &data, -1);
```

## Return Values

| Value | Description |
|--------|------|
| `0` | Normal exit |
| `-1` | Initialization failed or capture failed |

## Notes

1. This program needs to run on target ARM device, cannot execute directly on x86 host
2. Must start `ai-core` service first and enable camera function
3. Captured data needs to be freed using `ai_core_free_data()` to avoid memory leak
4. Ensure save path has enough disk space (JPEG approx 200-300KB/frame, NV12 approx 3MB/frame)
5. Shared memory path is `/ai-core_shm`, ensure access permission

## Custom Modification

### Modify Capture Interval
Edit `camera_capture_example.c`:
```c
// Default every 3 seconds (implemented using loop)
for (int i = 0; i < 5 && g_running; i++) {
    usleep(100000); // 100ms
}

// Modify to every 1 second
for (int i = 0; i < 10 && g_running; i++) {
    usleep(100000); // Total 1 second
}
```

### Modify Save Filename Format
```c
// Default format
snprintf(filename, sizeof(filename), "%s/capture_%03d.%s", save_path, count, ext);

// Add timestamp
time_t now = time(NULL);
snprintf(filename, sizeof(filename), "%s/capture_%ld_%03d.%s", save_path, now, count, ext);
```

### Save Only Specific Format
```c
if (data.format == AI_MEDIA_FORMAT_JPEG) {
    // Save JPEG only
    save_image_to_file(&data, filename);
} else {
    printf("Skip NV12 format\n");
}
```

## Performance Optimization Suggestions

1. **Control Capture Frequency**: Set reasonable capture interval according to actual needs
2. **Free Memory Timely**: Free `data` immediately after capture to avoid memory accumulation
3. **Async Save**: Consider using independent thread to save file to avoid blocking capture
4. **Limit File Count**: Implement file rotation to avoid filling up disk

## Related Commands

- Check Service: `ps aux | grep ai-core`
- View Shared Memory: `ls -lh /dev/shm/ai-core_shm`
- View Socket: `ls -la /tmp/ai-core_camera_ctrl`
- View Image: `eog /tmp/capture_*.jpg`

## Related Documentation

- **Programming Interface**: [Camera Client API Development Guide](../../docs/Camera_Client_API.en.md)
- **Camera Service**: [Camera Service Implementation](../../../docs/server/CAMERA_SERVICE_IMPLEMENTATION.md) (Note: File might be missing)
- **SDK Documentation**: `../../README.en.md`
- **Header File**: `../../include/ai_camera.h`

## Example Code Location

Complete source code: `camera_capture_example.c`

You can refer to the code implementation and modify/extend according to actual needs.
