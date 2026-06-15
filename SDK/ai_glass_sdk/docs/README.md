# AI Glass SDK 文档中心

中文版本 | [English Version](README.en.md)

欢迎来到AI Glass SDK文档中心。这里包含了使用AI Glass客户端SDK所需的所有文档。

---

## 📚 文档分类

### 🎯 快速开始
- [SDK快速入门](../README.md) - SDK概述和基本使用方法
- [安装和编译](../README.md#-快速开始) - 如何编译SDK和示例程序

### 🔌 客户端API文档
- [GPIO客户端API](GPIO_Client_API.md) - GPIO按键事件订阅和异步回调
- [摄像头客户端API](Camera_Client_API.md) - 图像捕获和零拷贝传输
- [音频客户端API](Audio_Client_API.md) - 音频播放与资源控制
- [显示客户端API](Display_Client_API.md) - 帧缓冲提交和焦点管理
- [BLE 文本客户端 API](BLE_Client_API.md) - 订阅/发送 `datatype` 文本消息
- [文本事件客户端 API](Text_Event_Client_API.md) - ASR / LLM / System 文本流监听
- [日志系统API](Log_API.md) - 统一日志输出和毫秒级时间戳

### 🔧 示例程序文档
- [GPIO事件客户端](../examples/gpio_example/) - GPIO事件订阅示例
- [摄像头客户端](../examples/camera_capture_example/) - 图像捕获示例
- [音频播放客户端](../examples/audio_play_example/) - 音频播放示例
- [禁用 AI-Core 物理动作示例](../examples/disable_aicore_physical_actions_example/) - 禁用 AI-Core 自动物理按键动作
- [只读查询 AI-Core 物理动作状态示例](../examples/query_aicore_physical_actions_example/) - 不修改运行态，仅查询物理动作禁用状态
- [SDK 控制录音示例](../examples/record_audio_example/) - 开始/停止录音并保存到固定路径
- [媒体资源切换控制台](../examples/media_resource_control/) - 相机/音频资源释放与回收示例
- [文本事件客户端](../examples/text_event_example/) - 文本流监听示例
- [BLE 往返 Demo](../examples/ble_demo/) - Android 端和眼镜端 BLE 文本往返示例

---

## 📖 推荐阅读路径

### 1. 新手入门
1. 阅读 [SDK快速入门](../README.md) 了解基本概念
2. 查看 [GPIO客户端API](GPIO_Client_API.md) 学习事件订阅
3. 运行 [GPIO事件客户端示例](../examples/gpio_example/)

### 2. 摄像头开发
1. 阅读 [摄像头客户端API](Camera_Client_API.md)
2. 查看 [摄像头客户端示例](../examples/camera_capture_example/)
3. 了解共享内存零拷贝机制

### 3. 音频开发
1. 阅读 [音频客户端API](Audio_Client_API.md)
2. 查看 [音频播放客户端示例](../examples/audio_play_example/)
3. 如需禁用 AI-Core 物理动作，查看 [禁用 AI-Core 物理动作示例](../examples/disable_aicore_physical_actions_example/)
4. 如需脚本触发录音，查看 [SDK 控制录音示例](../examples/record_audio_example/)
5. 如需应用切换，查看 [媒体资源切换控制台](../examples/media_resource_control/)

### 4. BLE 文本开发
1. 阅读 [BLE 文本客户端 API](BLE_Client_API.md)
2. 确认 `bt_service` 已提供 `/var/run/ai_ble.sock`
3. 查看 [BLE 往返 Demo](../examples/ble_demo/)
4. 按 `datatype` 订阅消息，例如 `display.text`

---

## 🏗️ SDK架构概览

```
外部应用
    ↓
AI Glass SDK (本SDK)
    ↓
AI Media Service (服务端)
    ↓
硬件资源 (GPIO、摄像头、音频)
```

### 支持的功能模块
- **GPIO事件订阅** - 多进程GPIO事件监听
- **摄像头调用** - 零拷贝图像传输
- **音频播放控制** - PCM播放与资源控制
- **显示服务** - 帧缓冲传输和多客户端焦点管理
- **BLE 文本通道** - 按 `datatype` 路由 BLE 文本消息

---

## 🔗 快速链接

### 常用API快速参考
- `ai_gpio_event_client_create()` - 创建GPIO客户端
- `ai_core_init()` - 初始化摄像头客户端
- `ai_audio_init()` - 初始化音频客户端
- `ai_display_init()` - 初始化显示客户端
- `ai_display_commit_frame()` - 提交帧更新
- `ai_ble_client_create()` - 创建 BLE 文本客户端
- `ai_ble_send()` - 发送 BLE 文本消息
- `log_info()` - 输出信息日志（带时间戳）
- `log_error()` - 输出错误日志（带时间戳）

### 示例程序路径
- GPIO示例：`../examples/gpio_example/`
- 音频示例：`../examples/audio_play_example/`
- 摄像头示例：`../examples/camera_capture_example/`
- 禁用 AI-Core 物理动作示例：`../examples/disable_aicore_physical_actions_example/`
- 只读查询 AI-Core 物理动作状态示例：`../examples/query_aicore_physical_actions_example/`
- 录音示例：`../examples/record_audio_example/`
- 媒体资源切换示例：`../examples/media_resource_control/`
- BLE 往返 Demo：`../examples/ble_demo/`

### 头文件位置
- GPIO API：`../include/ai_gpio.h`
- 摄像头API：`../include/ai_camera.h`
- 音频API：`../include/ai_audio.h`
- 显示API：`../include/ai_display.h`
- BLE API：`../include/ai_ble.h`
- 文本事件 API：`../include/ai_text_event.h`
- IPC基础：`../include/ai_ipc.h`
- 日志API：`../include/ai_log.h`

---

## ❓ 获取帮助

### 常见问题
1. **客户端连接失败** - 检查服务端是否启动
2. **GPIO事件收不到** - 确认GPIO硬件配置
4. **摄像头捕获超时** - 检查设备权限和3A初始化

### 文档反馈
如果在使用过程中发现问题或有改进建议，请通过以下方式反馈：
- 提交Issue到项目仓库
- 联系技术支持团队

---

*最后更新：2026-05-14*
