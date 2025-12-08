# 摄像头客户端API - 接入指南

中文版本 | [English Version](Camera_Client_API.en.md)

> **版本**: v1.0 | **日期**: 2025-10-10

---

## 📖 目录

- [快速开始](#快速开始)
- [API参考](#api参考)
- [完整示例](#完整示例)
- [故障排查](#故障排查)

---

## 🚀 快速开始

### 功能简介

调用摄像头功能，支持JPEG和NV12两种图像格式。

### 编译客户端程序

#### 链接SDK库

```bash
# 先编译SDK库
cd ai_glass_sdk
make

# 编译自己的程序
arm-rockchip831-linux-uclibcgnueabihf-gcc \
    -o my_camera_client \
    my_camera_client.c \
    -I/path/to/ai_glass_sdk/include \
    -L/path/to/ai_glass_sdk/lib \
    -lai_glass_sdk \
    -lrt
```

### 运行示例程序

```bash
# 运行客户端示例
cd ai_glass_sdk/examples/example_media_client
./example_media_client /mnt/sdcard
```

### 最小客户端代码

```c
#include "ai_camera.h"
#include <stdio.h>

int main() {
    // 1. 初始化客户端
    ai_core_client_t *client = ai_core_init();
    if (!client) {
        printf("初始化失败\n");
        return -1;
    }

    // 2. 捕获图像
    ai_core_data_t data;
    int result = ai_core_capture(client, &data, 5000);

    if (result == AI_MEDIA_SUCCESS) {
        printf("捕获成功: %zu bytes, %dx%d\n",
               data.size, data.width, data.height);

        // 3. 处理图像数据（例如保存到文件）
        FILE *fp = fopen("/tmp/capture.jpg", "wb");
        fwrite(data.data, 1, data.size, fp);
        fclose(fp);

        // 4. 释放图像内存
        ai_core_free_data(&data);
    }

    // 5. 清理资源
    ai_core_cleanup(client);
    return 0;
}
```

### 主要特性

- ✅ 支持JPEG和NV12两种格式
- ✅ 支持多客户端并发访问
- ✅ 可配置超时时间
- ✅ 完整的错误处理

---

## 📋 API参考

### 数据结构

#### ai_core_client_t

客户端句柄（不透明类型），通过 `ai_core_init()` 创建。

#### ai_core_data_t

图像数据结构：

```c
typedef struct {
    unsigned char *data;    // 图像数据指针
    size_t size;            // 数据大小（字节）
    int width;              // 图像宽度
    int height;             // 图像高度
    int format;             // 图像格式
    int sequence;           // 帧序号
} ai_core_data_t;
```

#### 图像格式常量

```c
#define AI_MEDIA_FORMAT_JPEG  0    // JPEG格式
#define AI_MEDIA_FORMAT_NV12  1    // NV12格式
```

#### 错误码

```c
#define AI_MEDIA_SUCCESS           0    // 成功
#define AI_MEDIA_ERROR_INIT       -1    // 初始化失败
#define AI_MEDIA_ERROR_TIMEOUT    -2    // 超时
#define AI_MEDIA_ERROR_CAPTURE    -3    // 捕获失败
#define AI_MEDIA_ERROR_MEMORY     -4    // 内存错误
```

### 核心API

#### ai_core_init()

初始化客户端，建立与服务端的连接。

```c
ai_core_client_t* ai_core_init(void);
```

**返回值**：
- 成功：客户端句柄指针
- 失败：NULL

**说明**：
- 连接到摄像头服务
- 自动重试连接（最多3次）

---

#### ai_core_capture()

捕获一帧图像。

```c
int ai_core_capture(ai_core_client_t *client,
                     ai_core_data_t *data,
                     int timeout_ms);
```

**参数**：
- `client` - 客户端句柄
- `data` - 输出的图像数据（由函数填充）
- `timeout_ms` - 超时时间（毫秒），0表示不超时

**返回值**：
- `AI_MEDIA_SUCCESS` (0) - 成功
- `AI_MEDIA_ERROR_TIMEOUT` (-2) - 超时
- `AI_MEDIA_ERROR_CAPTURE` (-3) - 捕获失败
- `AI_MEDIA_ERROR_MEMORY` (-4) - 内存分配失败

**说明**：
- 函数会分配内存存储图像数据
- 必须调用 `ai_core_free_data()` 释放内存
- 支持并发调用（多个客户端同时捕获）

---

#### ai_core_free_data()

释放图像数据内存。

```c
void ai_core_free_data(ai_core_data_t *data);
```

**参数**：
- `data` - 要释放的图像数据

**说明**：
- 释放由 `ai_core_capture()` 分配的内存
- 调用后 `data->data` 将被置为NULL

---

#### ai_core_cleanup()

清理客户端资源。

```c
void ai_core_cleanup(ai_core_client_t *client);
```

**参数**：
- `client` - 客户端句柄

**说明**：
- 断开与服务端的连接
- 释放所有资源

---

#### ai_core_get_error_string()

获取错误码对应的错误信息。

```c
const char* ai_core_get_error_string(int error_code);
```

**参数**：
- `error_code` - 错误码

**返回值**：
- 错误信息字符串

**示例**：
```c
int result = ai_core_capture(client, &data, 5000);
if (result != AI_MEDIA_SUCCESS) {
    printf("错误: %s\n", ai_core_get_error_string(result));
}
```

---

## 💡 完整示例

参考文件: `ai_glass_sdk/examples/example_media_client/example_media_client.c`

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
    printf("  摄像头客户端示例\n");
    printf("  保存路径: %s\n", save_path);
    printf("═══════════════════════════════════════════\n\n");

    // 1. 初始化客户端
    printf("📝 初始化客户端...\n");
    ai_core_client_t *client = ai_core_init();
    if (!client) {
        printf("❌ 初始化失败，请确保服务端已启动\n");
        return -1;
    }
    printf("✅ 客户端已初始化\n\n");

    // 2. 捕获多帧图像
    for (int i = 0; i < 5; i++) {
        printf("📷 [%d/5] 捕获图像中...\n", i + 1);

        ai_core_data_t data;
        int result = ai_core_capture(client, &data, 5000);

        if (result == AI_MEDIA_SUCCESS) {
            const char *format_str = (data.format == AI_MEDIA_FORMAT_JPEG) ?
                                     "JPEG" : "NV12";

            printf("✅ 捕获成功:\n");
            printf("   格式: %s\n", format_str);
            printf("   分辨率: %dx%d\n", data.width, data.height);
            printf("   大小: %zu bytes\n", data.size);
            printf("   序列号: %d\n", data.sequence);

            // 保存到文件
            char filename[256];
            const char *ext = (data.format == AI_MEDIA_FORMAT_JPEG) ?
                              "jpg" : "nv12";
            snprintf(filename, sizeof(filename),
                     "%s/capture_%03d.%s", save_path, i + 1, ext);

            FILE *fp = fopen(filename, "wb");
            if (fp) {
                fwrite(data.data, 1, data.size, fp);
                fclose(fp);
                printf("   已保存: %s\n", filename);
            } else {
                printf("   ⚠️  保存失败: %s\n", filename);
            }

            // 释放内存
            ai_core_free_data(&data);
            capture_count++;
        } else {
            printf("❌ 捕获失败: %s\n",
                   ai_core_get_error_string(result));
        }

        printf("\n");
        sleep(1);  // 等待1秒再捕获下一帧
    }

    // 3. 清理资源
    printf("📝 清理资源...\n");
    ai_core_cleanup(client);

    printf("\n═══════════════════════════════════════════\n");
    printf("  完成！成功捕获 %d 帧图像\n", capture_count);
    printf("═══════════════════════════════════════════\n");

    return 0;
}
```

---

## 🔧 故障排查

### 1. 客户端初始化失败

**错误**：`ai_core_init()` 返回 NULL

**原因**：
- 服务端未启动
- 服务端未启用摄像头模式
- Unix Socket文件不存在

**解决**：
```bash
# 检查服务端是否运行
ps aux | grep ai-core

# 检查Socket文件
ls -la /tmp/ai-core_camera_ctrl

# 确保服务端启用摄像头
./ai-core --enable-camera --enable-jpeg
```

---

### 2. 捕获超时

**错误**：`ai_core_capture()` 返回 `AI_MEDIA_ERROR_TIMEOUT`

**原因**：
- 服务端摄像头忙碌
- 摄像头硬件故障
- 超时时间设置过短

**解决**：
```c
// 增加超时时间
int result = ai_core_capture(client, &data, 10000);  // 10秒

// 或者不限制超时
int result = ai_core_capture(client, &data, 0);
```

---

### 3. 图像格式不符合预期

**问题**：期望JPEG但收到NV12

**原因**：服务端未启用JPEG编码

**解决**：
```bash
# 确保服务端启动时使用 --enable-jpeg
./ai-core --enable-camera --enable-jpeg
```

---

### 4. 内存泄漏

**问题**：长时间运行后内存占用增加

**原因**：忘记调用 `ai_core_free_data()`

**解决**：
```c
// ❌ 错误：未释放内存
ai_core_capture(client, &data, 5000);
// ... 使用data ...
// 忘记释放！

// ✅ 正确：始终释放内存
ai_core_capture(client, &data, 5000);
// ... 使用data ...
ai_core_free_data(&data);  // 必须调用
```

---

## ⚙️ 系统要求

### 服务端要求

服务端必须启用摄像头模式：
```bash
# 最小配置
./ai-core --enable-camera

# 推荐配置（JPEG + 3A）
./ai-core --enable-camera --enable-jpeg --enable-3a
```

### 编译依赖

- **交叉编译工具链**: arm-rockchip831-linux-uclibcgnueabihf-gcc
- **系统库**: librt (POSIX实时扩展)

### 运行环境

- Linux系统
- 足够的系统内存

---

## 📊 性能特性

| 特性 | 说明 |
|------|------|
| **低延迟** | 典型捕获延迟 < 100ms |
| **并发支持** | 支持多个客户端同时捕获 |
| **图像格式** | 支持JPEG（压缩）和NV12（原始YUV） |
| **最大分辨率** | 1920x1080 |

---

## 🔗 相关文档

- **示例程序**: `ai_glass_sdk/examples/example_media_client/`
- **头文件**: `ai_glass_sdk/include/ai_camera.h`
- **SDK README**: `ai_glass_sdk/README.md`

---

**版本**: v1.0
**日期**: 2025-10-10
**状态**: ✅ 生产就绪
