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
- [音频客户端API](Audio_Client_API.md) - 音频播放和TTS功能
- [显示客户端API](Display_Client_API.md) - 帧缓冲提交和焦点管理
- [日志系统API](Log_API.md) - 统一日志输出和毫秒级时间戳

### 🗣️ TTS功能专题
- [TTS客户端API](TTS_Client_API.md) - TTS功能使用、配置、协议和故障排查完整指南

### 🔧 示例程序文档
- [GPIO事件客户端](../examples/gpio_client/) - GPIO事件订阅示例
- [摄像头客户端](../examples/example_media_client/) - 图像捕获示例
- [音频播放客户端](../examples/audio_play_client/) - PCM播放和TTS功能示例

---

## 📖 推荐阅读路径

### 1. 新手入门
1. 阅读 [SDK快速入门](../README.md) 了解基本概念
2. 查看 [GPIO客户端API](GPIO_Client_API.md) 学习事件订阅
3. 运行 [GPIO事件客户端示例](../examples/gpio_client/)

### 2. 摄像头开发
1. 阅读 [摄像头客户端API](Camera_Client_API.md)
2. 查看 [摄像头客户端示例](../examples/example_media_client/)
3. 了解共享内存零拷贝机制

### 3. 音频开发
1. 阅读 [音频客户端API](Audio_Client_API.md)
2. 查看 [音频播放客户端示例](../examples/audio_play_client/)
3. 如需TTS功能，阅读 [TTS专题文档](#-tts功能专题)

### 4. TTS功能开发
1. 阅读 [TTS客户端API](TTS_Client_API.md)
2. 根据指南配置TTS服务器和客户端
3. 遇到问题查看指南中的故障排查章节

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
- **音频播放控制** - PCM播放和TTS文本转语音
- **显示服务** - 帧缓冲传输和多客户端焦点管理
- **TTS缓存** - 智能MD5缓存减少云端调用

---

## 🔗 快速链接

### 常用API快速参考
- `ai_gpio_event_client_create()` - 创建GPIO客户端
- `ai_core_init()` - 初始化摄像头客户端
- `ai_audio_init()` - 初始化音频客户端
- `ai_audio_play_tts_simple()` - 简单TTS播放
- `ai_display_init()` - 初始化显示客户端
- `ai_display_commit_frame()` - 提交帧更新
- `log_info()` - 输出信息日志（带时间戳）
- `log_error()` - 输出错误日志（带时间戳）

### 示例程序路径
- GPIO示例：`../examples/gpio_client/`
- 音频示例：`../examples/audio_play_client/`
- 摄像头示例：`../examples/example_media_client/`

### 头文件位置
- GPIO API：`../include/ai_gpio.h`
- 摄像头API：`../include/ai_camera.h`
- 音频API：`../include/ai_audio.h`
- 显示API：`../include/ai_display.h`
- IPC基础：`../include/ai_ipc.h`
- 日志API：`../include/ai_log.h`

---

## ❓ 获取帮助

### 常见问题
1. **客户端连接失败** - 检查服务端是否启动
2. **GPIO事件收不到** - 确认GPIO硬件配置
3. **TTS播放失败** - 查看 [TTS客户端API](TTS_Client_API.md) 中的故障排查章节
4. **摄像头捕获超时** - 检查设备权限和3A初始化

### 文档反馈
如果在使用过程中发现问题或有改进建议，请通过以下方式反馈：
- 提交Issue到项目仓库
- 联系技术支持团队

---

*最后更新：2025-10-27*