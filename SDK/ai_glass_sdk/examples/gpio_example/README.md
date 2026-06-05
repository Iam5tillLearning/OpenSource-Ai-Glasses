# GPIO Example 使用说明

## 简介

`gpio_example` 是一个演示程序，展示如何使用 AI Glass SDK 的 GPIO Hub 事件中心订阅和处理 GPIO 按键事件。

### 核心特性
- 订阅 GPIO 按键事件（按下/释放）
- 异步事件回调机制，实时响应
- 默认订阅 Hub 中全部活跃 GPIO，按下任一 GPIO 时输出实际编号
- 支持 `-g <gpio_number>` 只订阅指定 GPIO
- 多进程共享同一 GPIO Hub 事件源
- 无需直接访问 GPIO 硬件
- 完整的事件统计和调试信息

## 工作原理

### 架构说明
```
┌─────────────────┐
│   ai-core 服务   │  (监控GPIO硬件)
│  GPIO Manager   │
└────────┬────────┘
         │ 广播事件
         ├─────────┐
         │         │
    ┌────▼───┐  ┌─▼──────┐
    │ 客户端1 │  │ 客户端2 │
    └────────┘  └────────┘
```

1. **服务端** (ai-core)：监控 GPIO 硬件状态变化
2. **GPIO Hub**：通过统一共享内存和 Unix Domain Socket 广播事件给所有订阅客户端
3. **客户端**：接收事件并在独立线程中执行回调函数

## 编译

```bash
cd ai_glass_sdk/examples/gpio_example
make
```

编译产物：`gpio_example`

## 前置条件

### 1. 启动 AI Core 服务（启用 GPIO 功能）
```bash
# 在服务端目录
cd service
./build/ai-core --enable-gpio
```

### 2. 确认 GPIO 配置
服务端需要正确配置要监控的 GPIO 引脚。若设备需要同时暴露多个 GPIO，启动时指定 `--gpio-numbers`，例如：

```bash
./build/ai-core --enable-gpio --gpio-number 1 --gpio-numbers 0,1,75
```

## 使用方法

### 基本运行
```bash
# 订阅 Hub 中全部活跃 GPIO，适合排查当前到底按下了哪个 GPIO
./gpio_example

# 只订阅 GPIO 1
./gpio_example -g 1
```

### 程序输出示例
```
═══════════════════════════════════════════════════════════
  GPIO事件客户端 - Hub异步回调模式
  监听目标: 全部活跃GPIO
═══════════════════════════════════════════════════════════

📝 [步骤1/3] 创建GPIO Hub事件客户端...
✅ 客户端已创建

📝 [步骤2/3] 连接到GPIO Hub事件中心...
✅ 已连接到服务

📌 当前Hub活跃GPIO: GPIO0(释放) GPIO1(释放) GPIO75(释放)

📝 [步骤3/3] 订阅GPIO事件...
✅ 已订阅GPIO事件
   - 本地通知Socket: /tmp/ai_gpio_hub_client_12345_123456789
   - 当前事件序列号: 0

═══════════════════════════════════════════════════════════
  🎧 监听中... 请按下任一活跃GPIO按键
  💡 提示：按 Ctrl+C 退出程序
═══════════════════════════════════════════════════════════
```

### 按键事件输出示例

#### 按下事件
```
═══════════════════════════════════════════
  🔴 GPIO1 按键按下事件
───────────────────────────────────────────
  时间戳: 1234567890123 us
  按下次数: 1
═══════════════════════════════════════════
```

#### 释放事件
```
═══════════════════════════════════════════
  ⚪ GPIO1 按键释放事件
───────────────────────────────────────────
  时间戳: 1234567891234 us
  释放次数: 1
═══════════════════════════════════════════
```

### 退出程序
按 `Ctrl+C` 退出，程序会自动清理资源：
```
🛑 收到退出信号，准备关闭...

📝 清理资源...
   - 已注销通知Socket
✅ 资源已清理

═══════════════════════════════════════════════════════════
  程序退出
───────────────────────────────────────────────────────────
  总按下次数: 5
  总释放次数: 5
═══════════════════════════════════════════════════════════
```

## 程序功能说明

### 事件类型
- **GPIO_EVENT_PRESS** - 按键按下
- **GPIO_EVENT_RELEASE** - 按键释放
- **GPIO_EVENT_ERROR** - 错误事件

### 回调函数
程序通过 `my_gpio_event_callback()` 函数处理事件：
```c
void my_gpio_event_callback(gpio_event_t event_type,
                           int gpio_number,
                           void *user_data) {
    // 处理事件
}
```

### 特性
1. **实时响应**：回调在独立线程中执行，不阻塞主线程
2. **事件统计**：自动统计按下和释放次数
3. **心跳检查**：每 10 秒检查一次服务状态
4. **优雅退出**：Ctrl+C 触发资源清理

## 使用场景

### 场景1：按键触发录音
```c
void my_gpio_event_callback(gpio_event_t event_type, int gpio_number, void *user_data) {
    if (event_type == GPIO_EVENT_PRESS) {
        // 开始录音
        start_recording();
    } else if (event_type == GPIO_EVENT_RELEASE) {
        // 停止录音
        stop_recording();
    }
}
```

### 场景2：按键计数器
```c
static int button_press_count = 0;

void my_gpio_event_callback(gpio_event_t event_type, int gpio_number, void *user_data) {
    if (event_type == GPIO_EVENT_PRESS) {
        button_press_count++;
        printf("按键次数: %d\n", button_press_count);
    }
}
```

### 场景3：长按检测
```c
static uint64_t press_timestamp = 0;

void my_gpio_event_callback(gpio_event_t event_type, int gpio_number, void *user_data) {
    if (event_type == GPIO_EVENT_PRESS) {
        press_timestamp = ai_gpio_get_timestamp_us();
    } else if (event_type == GPIO_EVENT_RELEASE) {
        uint64_t duration = ai_gpio_get_timestamp_us() - press_timestamp;
        if (duration > 2000000) {  // 2秒
            printf("检测到长按！\n");
        }
    }
}
```

## 错误处理

### 常见错误及解决方案

#### 1. 连接失败
```
❌ 连接失败，请确保ai-core已启动并启用GPIO功能
```
**解决**：
- 检查 `ai-core` 服务是否运行
- 确认启动时使用了 `--enable-gpio` 参数

#### 2. 订阅失败
```
❌ 订阅失败
```
**解决**：
- 检查 GPIO Hub 是否正确初始化
- 查看服务端日志了解详细错误信息

#### 3. 服务停止
```
⚠️  服务已停止，准备退出
```
**解决**：
- 服务端意外退出，重启 `ai-core` 服务

## 返回值

| 返回值 | 说明 |
|--------|------|
| `0` | 正常退出 |
| `-1` | 初始化失败或连接失败 |

## 编程接口

### 核心 API 函数

```c
// 1. 创建 Hub 客户端
int ai_gpio_hub_client_create(gpio_event_hub_client_t *client);

// 2. 连接到 GPIO Hub
int ai_gpio_hub_client_connect(gpio_event_hub_client_t *client);

// 3. 订阅指定 GPIO
int ai_gpio_hub_client_subscribe_gpios(gpio_event_hub_client_t *client,
                                       const int *gpio_list,
                                       int gpio_count,
                                       gpio_event_callback_t callback,
                                       void *user_data);

// 4. 订阅全部 GPIO
int ai_gpio_hub_client_subscribe_all(gpio_event_hub_client_t *client,
                                     gpio_event_callback_t callback,
                                     void *user_data);

// 5. 获取活跃 GPIO 列表
int ai_gpio_hub_client_get_active_gpios(gpio_event_hub_client_t *client,
                                        int *gpio_list,
                                        int max_count);

// 6. 检查服务状态
int ai_gpio_hub_client_is_service_alive(gpio_event_hub_client_t *client);

// 7. 销毁客户端
void ai_gpio_hub_client_destroy(gpio_event_hub_client_t *client);

// 辅助函数
uint64_t ai_gpio_get_timestamp_us(void);  // 获取微秒级时间戳
```

### 完整 API 文档
详细的编程接口文档，请参见：
**📚 [GPIO Client API 开发指南](../../docs/GPIO_Client_API.md)**

## 多客户端支持

多个客户端可以同时订阅同一个 GPIO 事件：

```bash
# 终端1
./gpio_example

# 终端2
./gpio_example

# 两个客户端都会收到同样的按键事件
```

## 性能特点

- **低延迟**：事件广播延迟通常 < 1ms
- **高并发**：支持多个客户端同时订阅
- **轻量级**：单个客户端仅占用少量内存（< 100KB）

## 调试技巧

### 1. 查看服务端日志
```bash
# 服务端会输出GPIO状态变化
# 查看是否有事件广播日志
```

### 2. 检查 Socket 连接
```bash
# 查看客户端 Socket
ls -la /tmp/ai_gpio_hub_client_*

# 查看服务端 Socket
ls -la /tmp/ai_gpio_event_hub_broadcast

# 查看 Hub 共享内存
ls -la /dev/shm/ai_gpio_event_hub
```

### 3. 启用详细日志
修改源代码中的日志级别，可以看到更多调试信息。

## 注意事项

1. 此程序需要在目标 ARM 设备上运行，无法在 x86 主机上直接执行
2. 必须先启动 `ai-core` 服务且启用 GPIO 功能
3. 回调函数在独立线程中执行，注意线程安全
4. 确保有足够的权限访问 `/tmp` 目录

## 相关命令

- 查看帮助: 运行程序会自动显示使用说明
- 检查服务: `ps aux | grep ai-core`
- 查看 Socket: `ls -la /tmp/ai_gpio_event_hub_broadcast /tmp/ai_gpio_hub_client_*`

## 相关文档

- **编程接口**: [GPIO Client API 开发指南](../../docs/GPIO_Client_API.md)
- **GPIO 架构**: [GPIO Architecture](../../../docs/server/GPIO_ARCHITECTURE.md)
- **SDK 文档**: `../../README.md`
- **头文件**: `../../include/ai_gpio.h`
