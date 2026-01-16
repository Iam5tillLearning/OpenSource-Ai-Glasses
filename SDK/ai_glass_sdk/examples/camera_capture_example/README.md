# Camera Capture Example 使用说明

## 简介

`camera_capture_example` 是一个演示程序，展示如何使用 AI Camera SDK 从共享内存中捕获摄像头图像数据。

### 核心特性
- 从共享内存读取摄像头数据
- 支持 JPEG 和 NV12 格式
- 自动保存捕获的图像到文件
- 定时捕获（默认每 3 秒）
- 完整的错误处理和状态反馈

## 工作原理

### 架构说明
```
┌─────────────────┐
│   ai-core 服务   │
│  Camera Service │
│   (写入SHM)     │
└────────┬────────┘
         │ 共享内存
         │ /ai-core_shm
    ┌────▼────┐
    │  客户端  │
    │ (读取SHM) │
    └─────────┘
```

1. **服务端** (ai-core)：从摄像头获取图像，写入共享内存
2. **共享内存**：高性能的进程间通信机制
3. **客户端**：读取共享内存数据并保存到文件

## 编译

```bash
cd ai_glass_sdk/examples/camera_capture_example
make
```

编译产物：`camera_capture_example`

## 前置条件

### 1. 启动 AI Core 服务（启用摄像头功能）
```bash
cd service
./build/ai-core --enable-camera
```

### 2. 确认摄像头可用
确保设备已连接摄像头并且驱动正常工作。

## 使用方法

### 基本运行（保存到 /tmp）
```bash
./camera_capture_example
```

### 指定保存路径
```bash
./camera_capture_example /path/to/save
```

## 程序输出示例

### 启动阶段
```
🚀 [EXAMPLE] AI Media Client Example Starting...
📁 [EXAMPLE] Media data will be saved to: /tmp
💡 [EXAMPLE] Press Ctrl+C to exit

📸 [EXAMPLE] Capturing media data #1...
```

### 成功捕获
```
✅ [EXAMPLE] Capture successful:
   Size: 245678 bytes
   Resolution: 1920x1080
   Format: JPEG
   Sequence: 1
💾 [EXAMPLE] Media data saved to: /tmp/capture_001.jpg
```

### 持续捕获
程序会每 3 秒自动捕获一次：
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

### 退出程序
按 `Ctrl+C` 退出：
```
^C
🛑 [EXAMPLE] Received signal 2, exiting...

📊 [EXAMPLE] Total captures: 5
✅ [EXAMPLE] AI Media Client Example Finished
```

## 捕获的文件格式

### JPEG 格式
```bash
# 文件名格式
capture_001.jpg
capture_002.jpg
capture_003.jpg
...

# 可直接查看
eog capture_001.jpg       # Linux
open capture_001.jpg      # macOS
```

### NV12 格式（原始 YUV 数据）
```bash
# 文件名格式
capture_001.nv12
capture_002.nv12
...

# 需要专门工具查看（如 ffplay）
ffplay -f rawvideo -pixel_format nv12 -video_size 1920x1080 capture_001.nv12
```

## 使用场景

### 场景 1：定时拍照
```c
#include "ai_camera.h"

int main(void) {
    ai_core_client_t *client = ai_core_init();

    while (1) {
        ai_core_data_t data;
        if (ai_core_capture(client, &data, 5000) == AI_MEDIA_SUCCESS) {
            // 保存图像
            save_image_to_file(&data, "/tmp/photo.jpg");
            ai_core_free_data(&data);
        }
        sleep(60);  // 每分钟拍一张
    }

    ai_core_cleanup(client);
    return 0;
}
```

### 场景 2：按需拍照
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

// 使用
take_photo("/tmp/snapshot.jpg");
```

### 场景 3：图像分析
```c
void analyze_images(void) {
    ai_core_client_t *client = ai_core_init();

    for (int i = 0; i < 10; i++) {
        ai_core_data_t data;
        if (ai_core_capture(client, &data, 5000) == AI_MEDIA_SUCCESS) {
            // 分析图像（如人脸检测、物体识别等）
            analyze_image_data(data.data, data.size, data.width, data.height);
            ai_core_free_data(&data);
        }
        sleep(1);
    }

    ai_core_cleanup(client);
}
```

### 场景 4：运动检测
```c
void motion_detection(void) {
    ai_core_client_t *client = ai_core_init();
    ai_core_data_t prev_data = {0}, curr_data;

    while (1) {
        if (ai_core_capture(client, &curr_data, 5000) == AI_MEDIA_SUCCESS) {
            if (prev_data.data) {
                // 比较当前帧和前一帧
                if (detect_motion(&prev_data, &curr_data)) {
                    printf("检测到运动！\n");
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

## 错误处理

### 常见错误及解决方案

#### 1. 捕获失败 - 服务不可用
```
❌ [EXAMPLE] Capture failed: Initialization error
🛑 [EXAMPLE] Service unavailable, exiting...
```
**解决**：
- 检查 `ai-core` 服务是否运行
- 确认启动时使用了 `--enable-camera` 参数

#### 2. 捕获超时
```
❌ [EXAMPLE] Capture failed: Timeout
```
**解决**：
- 摄像头可能未正确初始化
- 检查摄像头硬件连接
- 查看服务端日志了解详细错误

#### 3. 文件保存失败
```
❌ [EXAMPLE] Failed to save data to /tmp/capture_001.jpg: Permission denied
```
**解决**：
- 检查保存路径的写权限
- 确保磁盘空间充足

#### 4. 部分数据写入
```
⚠️ [EXAMPLE] Warning: Only wrote 100000/245678 bytes to /tmp/capture_001.jpg
```
**解决**：
- 检查磁盘空间
- 检查文件系统状态

## 编程接口

### 核心 API 函数

```c
// 1. 初始化媒体客户端
ai_core_client_t* ai_core_init(void);

// 2. 捕获媒体数据
int ai_core_capture(ai_core_client_t *client,
                   ai_core_data_t *data,
                   int timeout_ms);

// 3. 释放媒体数据
void ai_core_free_data(ai_core_data_t *data);

// 4. 清理客户端
void ai_core_cleanup(ai_core_client_t *client);

// 5. 获取错误描述
const char* ai_core_get_error_string(int error_code);
```

### 数据结构

```c
// 媒体数据结构
typedef struct {
    uint8_t *data;          // 图像数据指针
    size_t size;            // 数据大小（字节）
    int width;              // 图像宽度
    int height;             // 图像高度
    ai_media_format_t format;  // 格式（JPEG/NV12）
    int sequence;           // 序列号
} ai_core_data_t;

// 媒体格式
typedef enum {
    AI_MEDIA_FORMAT_JPEG = 0,  // JPEG 压缩格式
    AI_MEDIA_FORMAT_NV12 = 1   // NV12 YUV 格式
} ai_media_format_t;
```

### 错误代码

```c
#define AI_MEDIA_SUCCESS         0   // 成功
#define AI_MEDIA_ERROR_INIT     -1   // 初始化错误
#define AI_MEDIA_ERROR_TIMEOUT  -2   // 超时
#define AI_MEDIA_ERROR_INVALID  -3   // 无效参数
#define AI_MEDIA_ERROR_NO_DATA  -4   // 无数据
```

### 完整 API 文档
详细的编程接口文档，请参见：
**📚 [Camera Client API 开发指南](../../docs/Camera_Client_API.md)**

## 性能特点

### 优势
- **零拷贝**：使用共享内存，避免数据拷贝
- **低延迟**：直接从共享内存读取，延迟 < 10ms
- **高吞吐**：支持高帧率图像传输（如 30fps）

### 性能数据（参考）
- **JPEG 1920x1080**：约 200-300KB，捕获延迟 < 5ms
- **NV12 1920x1080**：约 3MB，捕获延迟 < 10ms

## 调试技巧

### 1. 检查共享内存
```bash
# 查看共享内存对象
ls -lh /dev/shm/ai-core_shm

# 查看权限和大小
```

### 2. 监控捕获速率
修改源代码添加性能统计：
```c
struct timeval start, end;
gettimeofday(&start, NULL);

ai_core_capture(client, &data, 5000);

gettimeofday(&end, NULL);
long elapsed_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
printf("捕获耗时: %ld us\n", elapsed_us);
```

### 3. 查看服务端日志
服务端会输出摄像头状态和共享内存更新日志。

### 4. 验证图像完整性
```bash
# 检查 JPEG 文件是否完整
file capture_001.jpg
jpeginfo capture_001.jpg

# 查看图像信息
identify capture_001.jpg  # ImageMagick 工具
```

## 超时设置

`ai_core_capture()` 的超时参数（毫秒）：
- **-1**：永久等待（不推荐）
- **0**：立即返回（非阻塞）
- **> 0**：指定超时时间（推荐：3000-5000ms）

```c
// 推荐设置
ai_core_capture(client, &data, 5000);  // 5秒超时

// 非阻塞模式
ai_core_capture(client, &data, 0);     // 立即返回

// 永久等待（慎用）
ai_core_capture(client, &data, -1);
```

## 返回值

| 返回值 | 说明 |
|--------|------|
| `0` | 正常退出 |
| `-1` | 初始化失败或捕获失败 |

## 注意事项

1. 此程序需要在目标 ARM 设备上运行，无法在 x86 主机上直接执行
2. 必须先启动 `ai-core` 服务且启用摄像头功能
3. 捕获的数据需要使用 `ai_core_free_data()` 释放，避免内存泄漏
4. 确保保存路径有足够的磁盘空间（JPEG约 200-300KB/帧，NV12约 3MB/帧）
5. 共享内存路径为 `/ai-core_shm`，确保有访问权限

## 自定义修改

### 修改捕获间隔
编辑 `camera_capture_example.c`：
```c
// 默认每 3 秒（实际实现使用循环实现）
for (int i = 0; i < 5 && g_running; i++) {
    usleep(100000); // 100ms
}

// 修改为每 1 秒
for (int i = 0; i < 10 && g_running; i++) {
    usleep(100000); // 总共 1 秒
}
```

### 修改保存文件名格式
```c
// 默认格式
snprintf(filename, sizeof(filename), "%s/capture_%03d.%s", save_path, count, ext);

// 添加时间戳
time_t now = time(NULL);
snprintf(filename, sizeof(filename), "%s/capture_%ld_%03d.%s", save_path, now, count, ext);
```

### 只保存特定格式
```c
if (data.format == AI_MEDIA_FORMAT_JPEG) {
    // 只保存 JPEG
    save_image_to_file(&data, filename);
} else {
    printf("跳过 NV12 格式\n");
}
```

## 性能优化建议

1. **控制捕获频率**：根据实际需求设置合理的捕获间隔
2. **及时释放内存**：捕获后立即释放 `data`，避免内存堆积
3. **异步保存**：考虑使用独立线程保存文件，避免阻塞捕获
4. **限制文件数量**：实现文件轮转，避免磁盘被填满

## 相关命令

- 检查服务: `ps aux | grep ai-core`
- 查看共享内存: `ls -lh /dev/shm/ai-core_shm`
- 查看 Socket: `ls -la /tmp/ai-core_camera_ctrl`
- 查看图像: `eog /tmp/capture_*.jpg`

## 相关文档

- **编程接口**: [Camera Client API 开发指南](../../docs/Camera_Client_API.md)
- **摄像头服务**: [Camera Service Implementation](../../../docs/server/CAMERA_SERVICE_IMPLEMENTATION.md)
- **SDK 文档**: `../../README.md`
- **头文件**: `../../include/ai_camera.h`

## 示例代码位置

完整源代码：`camera_capture_example.c`

你可以参考代码实现，根据实际需求进行修改和扩展。
