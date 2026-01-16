# Audio Play Example 使用说明

## 简介

`audio_play_example` 是一个命令行工具，用于控制 AI Core 的音频播放功能。

### 支持的播放方式
1. **PCM文件播放** - 播放本地PCM格式音频文件
2. **TTS文本播放** - 文本转语音并播放，支持智能缓存

### 核心特性
- ✅ 简单易用的命令行接口
- ✅ 支持音量控制和强制播放
- ✅ TTS智能缓存，提高响应速度
- ✅ 丰富的使用场景和脚本集成
- ✅ 完整的错误处理和状态反馈

## 编译工具
```bash
cd ai_glass_sdk/examples/audio_play_example
make
```

## 基本用法

### 命令格式
```bash
# PCM文件播放
audio_play_example -f <pcm_file> [options]

# TTS文本播放
audio_play_example -t <text> [options]

# 停止播放
audio_play_example -S
```

## 完整参数说明

### PCM文件播放参数
| 参数 | 长格式 | 说明 | 示例 |
|------|--------|------|------|
| `-f` | `--file` | PCM文件路径（必填） | `-f /tmp/audio.pcm` |
| `-r` | `--rate` | 采样率 (Hz) | `-r 48000` |
| `-c` | `--channels` | 声道数 (1-8) | `-c 2` |
| `-b` | `--bitwidth` | 位宽 (8/16/24/32) | `-b 16` |

### TTS文本播放参数
| 参数 | 长格式 | 说明 | 示例 |
|------|--------|------|------|
| `-t` | `--tts` | 要播报的文本（必填） | `-t "你好"` |
|  | `--no-cache` | 禁用缓存，每次重新生成 | `--no-cache` |

### 通用参数
| 参数 | 长格式 | 说明 | 默认值 |
|------|--------|------|--------|
| `-v` | `--volume` | 音量 (0-100) | 使用服务器默认值 |
| `-F` | `--force` | 强制播放（打断当前播放） | 排队播放 |
| `-s` | `--socket` | Unix Socket路径 | `/tmp/ai-core_audio_ctrl` |
| `-S` | `--stop` | 停止当前播放 | - |
| `-h` | `--help` | 显示帮助信息 | - |

## 使用示例

### 1. PCM文件播放

#### 基础播放
```bash
# 使用默认参数播放
./audio_play_example -f /tmp/audio.pcm
```

#### 指定音频参数
```bash
# 指定采样率和音量
./audio_play_example -f /tmp/audio.pcm -r 48000 -v 80

# 完整参数
./audio_play_example -f /tmp/music.pcm -r 48000 -c 2 -b 16 -v 90
```

#### 强制播放（打断当前播放）
```bash
./audio_play_example -f /tmp/urgent.pcm -F
```

### 2. TTS文本播放

#### 简单播放
```bash
# 播放中文文本
./audio_play_example -t "你好，欢迎使用AI语音助手"

# 播放英文文本
./audio_play_example -t "Hello World"
```

#### 强制播放重要通知
```bash
# 打断当前播放，立即播报
./audio_play_example -t "重要通知：系统将在5分钟后重启" -F
```

#### 高音量播放
```bash
./audio_play_example -t "警报！检测到异常" -v 100 -F
```

#### 禁用缓存（每次重新生成）
```bash
# 适用于动态内容，如时间、温度等
./audio_play_example -t "当前时间是上午10点30分" --no-cache

# 结合其他参数
./audio_play_example -t "室外温度25度" --no-cache -v 80
```

### 3. 播放控制

#### 停止播放
```bash
./audio_play_example -S
```

#### 使用自定义Socket路径
```bash
./audio_play_example -s /tmp/custom_socket -t "测试消息"
```

### 4. 批量播放

#### 连续播放多条TTS
```bash
#!/bin/bash
# 批量播放脚本
./audio_play_example -t "第一条消息"
sleep 1
./audio_play_example -t "第二条消息"
sleep 1
./audio_play_example -t "第三条消息"
```

#### 循环播放
```bash
#!/bin/bash
# 循环播报时间
while true; do
    current_time=$(date "+%H点%M分")
    ./audio_play_example -t "当前时间：${current_time}" --no-cache
    sleep 60  # 每分钟播报一次
done
```

## 常见场景

### 场景1: 智能门铃播报
```bash
# 有访客时播报
./audio_play_example -t "有访客按门铃" -F

# 快递到达
./audio_play_example -t "您的快递已到达，请及时取件" -v 90
```

### 场景2: 设备状态提醒
```bash
# 系统启动
./audio_play_example -t "系统初始化完成，准备就绪"

# 低电量提醒
./audio_play_example -t "电量不足，请及时充电" -F -v 100

# 温度异常
./audio_play_example -t "温度过高，请检查设备" -F --no-cache
```

### 场景3: 定时广播
```bash
#!/bin/bash
# 整点报时
hour=$(date "+%H")
./audio_play_example -t "现在是${hour}点整" --no-cache -v 70
```

### 场景4: 混合播放（TTS + 音效）
```bash
# 先播提示音，再播TTS
./audio_play_example -f /tmp/beep.pcm
sleep 1
./audio_play_example -t "操作成功"
```

## 返回值

| 返回值 | 说明 |
|--------|------|
| `0` | 成功 |
| `非0` | 失败（参数错误、服务未启动、网络问题等） |

## 输出示例

### 成功播放TTS
```
Playing TTS:
  Text: "你好，欢迎使用AI语音助手"
  Mode: Queue play
  Cache: Enabled
✅ TTS playback started
```

### 强制播放PCM
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

### 停止播放
```
Stopping audio playback...
✅ Audio stopped
```

## 错误处理

### 常见错误及解决方案

#### 1. 服务未启动
```
ERROR: Failed to initialize audio client
```
**解决**: 先启动 `ai-core` 服务

#### 2. Socket路径不存在
```
ERROR: Failed to connect to socket
```
**解决**: 检查Socket路径是否正确，默认为 `/tmp/ai-core_audio_ctrl`

#### 3. 文件不存在
```
ERROR: File not found
```
**解决**: 确认PCM文件路径正确且文件存在

#### 4. 参数冲突
```
ERROR: Cannot use both --file and --tts at the same time
```
**解决**: 同时只能使用一种播放方式（PCM文件或TTS）

#### 5. TTS服务器不可达
```
❌ Failed to play TTS: API call failed
```
**解决**:
- 检查网络连接
- 确认TTS服务器地址配置正确（服务端配置：`--tts-server` 和 `--tts-port`）
- 检查文本编码是否为UTF-8

---

## 🔧 编程接口

如果需要在程序中集成音频播放功能，可以使用SDK提供的编程接口：

### 基本C语言示例

```c
#include "ai_audio.h"

// 初始化客户端
ai_audio_t *client = ai_audio_init(NULL);

// PCM文件播放
ai_audio_play_simple(client, "/path/to/audio.pcm");

// TTS文本播放
ai_audio_play_tts_simple(client, "你好世界");

// 清理资源
ai_audio_cleanup(client);
```

### 详细API文档

完整的编程接口文档，请参见：
**📚 [Audio Client API 开发指南](../../docs/Audio_Client_API.md)**

该文档包含：
- 详细的API函数说明
- 数据结构定义
- 完整的C代码示例
- 错误处理和调试技巧

## 高级用法

### 集成到脚本
```bash
#!/bin/bash
# 智能提醒脚本

play_tts() {
    ./audio_play_example -t "$1" -v ${2:-80}
}

# 使用函数
play_tts "早上好" 70
sleep 2
play_tts "今天天气晴朗" 70
```

### 错误处理示例
```bash
#!/bin/bash
if ./audio_play_example -t "测试消息"; then
    echo "播放成功"
else
    echo "播放失败，错误码: $?"
    # 进行错误处理...
fi
```

### 与其他程序集成
```bash
# 监听文件变化并播报
inotifywait -m /path/to/watch -e create |
while read path action file; do
    ./audio_play_example -t "检测到新文件: ${file}"
done
```

## 性能提示

1. **使用缓存**: 对于重复播报的文本，启用缓存可大幅提高响应速度
2. **批量播放**: 多条消息之间适当添加延迟，避免丢失
3. **强制播放**: 仅用于紧急通知，避免频繁打断正常播放

## 注意事项

1. 此工具需要在目标ARM设备上运行，无法在x86主机上直接执行
2. TTS功能需要网络连接到TTS服务器（默认：xiaoyunyun.com:7863）
3. PCM文件必须是原始PCM格式，不支持MP3、WAV等编码格式
4. 音量范围0-100，超出范围会自动限制
5. TTS缓存目录为 `/oem/usr/bin/resources/cache`，确保有写权限

## 相关命令

- 查看帮助: `./audio_play_example --help`
- 查看版本: `file ./audio_play_example`
- 检查服务: `ps aux | grep ai-core`
- 查看Socket: `ls -la /tmp/ai-core_audio_ctrl`

## 🔗 相关文档

- **编程接口**: [Audio Client API 开发指南](../../docs/Audio_Client_API.md)
- **TTS功能**: [TTS客户端API](../../docs/TTS_Client_API.md)
- **SDK文档**: `../../README.md`
- **头文件**: `../../include/ai_audio.h`
