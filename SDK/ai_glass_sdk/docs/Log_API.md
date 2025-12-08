# 日志系统API - 使用指南

中文版本 | [English Version](Log_API.en.md)

> **版本**: v1.0 | **日期**: 2025-10-27 | **状态**: ✅ 生产就绪

---

## 📖 目录

- [快速开始](#快速开始)
- [API参考](#二api参考)
- [使用示例](#三使用示例)
- [集成方法](#四集成方法)
- [输出格式](#五输出格式)
- [注意事项](#六注意事项)

---

## 🚀 快速开始

### 功能简介

统一的日志输出系统，为所有日志自动添加**毫秒级精确时间戳**。支持信息、错误、调试、警告四种日志级别。

### 主要特性

- ✅ **毫秒级时间戳** - 精确到毫秒的时间记录（格式：`YYYY-MM-DD HH:MM:SS.mmm`）
- ✅ **多日志级别** - 支持 info、error、debug、warn 四种级别
- ✅ **使用简单** - API 与 printf 完全一致，无学习成本
- ✅ **无编码问题** - 直接使用 vprintf，支持中文、emoji 等所有字符
- ✅ **零依赖** - 仅依赖标准C库，无外部依赖
- ✅ **线程安全** - 基于标准输出，线程安全

### 最小示例

```c
#include "ai_log.h"

int main() {
    log_info("✅ 程序启动成功\n");
    log_error("❌ 连接失败: %s\n", "Connection refused");
    log_debug("📡 收到数据: %d 字节\n", 1024);
    log_warn("⚠️  内存使用率: %d%%\n", 85);
    return 0;
}
```

**输出效果**：
```
[2025-10-27 14:23:45.123] ✅ 程序启动成功
[2025-10-27 14:23:45.456] ❌ 连接失败: Connection refused
[2025-10-27 14:23:45.789] 📡 收到数据: 1024 字节
[2025-10-27 14:23:46.012] ⚠️  内存使用率: 85%
```

---

## 二、API参考

### 2.1 头文件

```c
#include "ai_log.h"
```

**头文件路径**: `ai_glass_sdk/include/ai_log.h`

### 2.2 日志函数

#### `log_info()` - 信息日志

```c
void log_info(const char *fmt, ...);
```

**功能**: 输出信息级别的日志

**参数**:
- `fmt` - 格式化字符串（与 printf 格式完全相同）
- `...` - 可变参数列表

**示例**:
```c
log_info("✅ 音频播放成功\n");
log_info("连接到服务器: %s:%d\n", "192.168.1.100", 8080);
log_info("处理完成，共 %d 个文件\n", file_count);
```

---

#### `log_error()` - 错误日志

```c
void log_error(const char *fmt, ...);
```

**功能**: 输出错误级别的日志

**参数**:
- `fmt` - 格式化字符串（与 printf 格式完全相同）
- `...` - 可变参数列表

**示例**:
```c
log_error("❌ 文件打开失败: %s\n", strerror(errno));
log_error("内存分配失败，需要 %zu 字节\n", required_size);
log_error("Socket连接超时\n");
```

---

#### `log_debug()` - 调试日志

```c
void log_debug(const char *fmt, ...);
```

**功能**: 输出调试级别的日志

**参数**:
- `fmt` - 格式化字符串（与 printf 格式完全相同）
- `...` - 可变参数列表

**示例**:
```c
log_debug("📡 收到消息: 类型=%d, 长度=%d\n", msg_type, msg_len);
log_debug("内部状态: state=%d, retry=%d\n", current_state, retry_count);
log_debug("性能统计: 耗时 %ld ms\n", elapsed_time);
```

---

#### `log_warn()` - 警告日志

```c
void log_warn(const char *fmt, ...);
```

**功能**: 输出警告级别的日志

**参数**:
- `fmt` - 格式化字符串（与 printf 格式完全相同）
- `...` - 可变参数列表

**示例**:
```c
log_warn("⚠️  缓冲区使用率过高: %d%%\n", usage_percent);
log_warn("检测到网络延迟: %d ms\n", latency);
log_warn("配置文件不存在，使用默认配置\n");
```

---

## 三、使用示例

### 3.1 替换现有 printf

**替换前**:
```c
printf("✅ [AUDIO_PLAYER] 音频播放器创建成功\n");
printf("❌ [TTS_CACHE] 创建Socket失败: %s\n", strerror(errno));
printf("📡 [DEBUG-SELECT] 开始等待socket数据...\n");
```

**替换后**:
```c
log_info("✅ [AUDIO_PLAYER] 音频播放器创建成功\n");
log_error("❌ [TTS_CACHE] 创建Socket失败: %s\n", strerror(errno));
log_debug("📡 [DEBUG-SELECT] 开始等待socket数据...\n");
```

### 3.2 完整的客户端程序示例

```c
#include "ai_log.h"
#include "ai_audio.h"
#include <errno.h>
#include <string.h>

int main() {
    ai_audio_client_t client;
    int ret;

    log_info("=== 音频客户端启动 ===\n");

    // 初始化客户端
    ret = ai_audio_init(&client);
    if (ret != 0) {
        log_error("❌ 初始化失败: 错误码 %d\n", ret);
        return -1;
    }
    log_info("✅ 客户端初始化成功\n");

    // 播放音频
    log_info("开始播放音频文件...\n");
    ret = ai_audio_play(&client, "/tmp/test.pcm", 80, 48000, 2, 16, 0);
    if (ret != 0) {
        log_error("❌ 播放失败: %s\n", ai_audio_get_error_string(ret));
        ai_audio_cleanup(&client);
        return -1;
    }
    log_info("✅ 音频播放成功\n");

    // 清理资源
    ai_audio_cleanup(&client);
    log_info("=== 程序正常退出 ===\n");

    return 0;
}
```

**输出效果**:
```
[2025-10-27 14:23:45.001] === 音频客户端启动 ===
[2025-10-27 14:23:45.015] ✅ 客户端初始化成功
[2025-10-27 14:23:45.018] 开始播放音频文件...
[2025-10-27 14:23:46.123] ✅ 音频播放成功
[2025-10-27 14:23:46.125] === 程序正常退出 ===
```

### 3.3 性能调试示例

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

    log_debug("开始处理数据: %zu 字节\n", len);

    // 处理数据...

    long elapsed = get_time_ms() - start;
    if (elapsed > 100) {
        log_warn("⚠️  处理耗时过长: %ld ms\n", elapsed);
    } else {
        log_debug("处理完成: %ld ms\n", elapsed);
    }
}
```

### 3.4 错误处理示例

```c
#include "ai_log.h"
#include <errno.h>
#include <string.h>
#include <fcntl.h>

int open_config_file(const char *path) {
    log_debug("尝试打开配置文件: %s\n", path);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        log_error("❌ 打开配置文件失败: %s (错误: %s)\n",
                  path, strerror(errno));
        return -1;
    }

    log_info("✅ 配置文件打开成功: %s (fd=%d)\n", path, fd);
    return fd;
}
```

---

## 四、集成方法

### 4.1 SDK 库已包含日志系统

日志系统已经编译到 AI Glass SDK 库中，无需额外编译。

**库文件**:
- 静态库: `ai_glass_sdk/lib/libai_glass_sdk.a`
- 动态库: `ai_glass_sdk/lib/libai_glass_sdk.so`

### 4.2 编译自己的程序

#### 链接 SDK 库

```bash
arm-rockchip831-linux-uclibcgnueabihf-gcc \
    -o my_app my_app.c \
    -I/path/to/ai_glass_sdk/include \
    -L/path/to/ai_glass_sdk/lib \
    -lai_glass_sdk \
    -lpthread -lrt
```

### 4.3 在项目中使用

**步骤**:
1. 包含头文件: `#include "ai_log.h"`
2. 使用日志函数: `log_info()`, `log_error()` 等
3. 编译时链接 SDK 库

**注意**: 服务端和客户端都可以使用日志系统。

---

## 五、输出格式

### 5.1 时间戳格式

```
[YYYY-MM-DD HH:MM:SS.mmm] 日志内容
```

**说明**:
- `YYYY-MM-DD` - 年月日
- `HH:MM:SS` - 时分秒（24小时制）
- `mmm` - 毫秒（000-999）

### 5.2 示例输出

```
[2025-10-27 14:23:45.123] ✅ 程序启动成功
[2025-10-27 14:23:45.456] 连接到服务器: 192.168.1.100:8080
[2025-10-27 14:23:46.789] 📡 收到响应: 1024 字节
[2025-10-27 14:23:47.012] ❌ 解析失败: Invalid JSON
[2025-10-27 14:23:47.234] 重试中... (1/3)
```

### 5.3 时间精度

- **精度**: 毫秒级（1ms）
- **实现**: 使用 `clock_gettime(CLOCK_REALTIME)` 获取纳秒级时间
- **时区**: 本地时区（使用 `localtime_r` 转换）

---

## 六、注意事项

### 6.1 字符编码

✅ **完全兼容原有 printf 的编码方式**
- 支持中文、英文、emoji 等所有字符
- 不做任何编码转换
- 不会造成乱码问题

**原理**: 使用 `vprintf()` 直接输出，只在前面添加时间戳字符串。

### 6.2 线程安全

✅ **基本线程安全**
- `printf()` 本身是线程安全的（有内部锁）
- 多线程调用日志函数不会导致崩溃
- 但同一时刻多个线程输出可能交错（这是 printf 的正常行为）

### 6.3 性能开销

**时间开销**:
- 每次日志调用增加约 **1-2 微秒**（获取时间戳）
- 对于嵌入式系统来说开销很小

**建议**:
- 在性能关键路径上，可以减少 `log_debug()` 调用
- 正常的 `log_info()` 和 `log_error()` 开销可忽略

### 6.4 输出目标

当前所有日志输出到 **标准输出（stdout）**。

**未来可扩展**:
- 输出到文件
- 输出到 syslog
- 动态控制日志级别
- 添加日志过滤

### 6.5 与现有代码集成

**平滑替换**:
1. 先编译日志系统（已完成）
2. 在需要的文件中 `#include "ai_log.h"`
3. 逐步将 `printf()` 替换为 `log_info()` 等
4. 可以渐进式替换，新旧代码可以共存

**替换建议**:
- ✅ 成功、完成类信息 → `log_info()`
- ❌ 错误、失败类信息 → `log_error()`
- 📡 调试、性能类信息 → `log_debug()`
- ⚠️  警告、提示类信息 → `log_warn()`

### 6.6 格式化字符串

**完全兼容 printf 格式**:
```c
// 字符串
log_info("消息: %s\n", str);

// 整数
log_info("计数: %d, 大小: %zu\n", count, size);

// 十六进制
log_debug("地址: 0x%p, 值: 0x%08X\n", ptr, value);

// 浮点数
log_info("进度: %.2f%%\n", progress);
```

---

## 七、常见问题

### Q1: 时间戳会不会影响性能？

**答**: 开销极小（1-2微秒），对嵌入式系统影响可忽略。

### Q2: 支持中文和 emoji 吗？

**答**: 完全支持。日志系统不做编码转换，原样输出。

### Q3: 可以关闭某个级别的日志吗？

**答**: 当前版本不支持。未来版本可以考虑添加日志级别控制。

### Q4: 日志会保存到文件吗？

**答**: 当前版本输出到标准输出。可以通过重定向保存：
```bash
./my_app > app.log 2>&1
```

### Q5: 多线程环境下日志会乱吗？

**答**: `printf()` 是线程安全的，不会崩溃。但多线程同时输出可能交错，这是正常现象。

---

## 八、版本历史

### v1.0 (2025-10-27)
- ✅ 初始版本发布
- ✅ 支持毫秒级时间戳
- ✅ 提供 info、error、debug、warn 四种日志级别
- ✅ 集成到 AI Glass SDK 库

---

## 九、相关链接

- **主 SDK 文档**: [AI Glass SDK README](../README.md)
- **API 头文件**: `ai_glass_sdk/include/ai_log.h`
- **其他 API 文档**:
  - [GPIO 客户端 API](GPIO_Client_API.md)
  - [摄像头客户端 API](Camera_Client_API.md)
  - [音频客户端 API](Audio_Client_API.md)
  - [TTS 客户端 API](TTS_Client_API.md)

---

*最后更新：2025-10-27*
