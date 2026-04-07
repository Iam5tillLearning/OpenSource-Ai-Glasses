# Record Audio Example 使用说明

## 简介

`record_audio_example` 演示如何通过 SDK 命令完成录音全流程：

1. 发送 `RECORD_START`
2. 等待用户回车
3. 发送 `RECORD_STOP`
4. 将服务端返回的录音文件复制到固定路径 `/tmp/test_record_audio`

该示例会打印每一步状态，便于确认当前流程和最终文件位置。

## 编译

```bash
cd ai_glass_sdk/examples/record_audio_example
make
```

编译产物：

- `../build/record_audio_example`

## 前置条件

1. `ai-core` 已运行且支持音频控制命令（`RECORD_START/RECORD_STOP`）
2. 推荐 `ai-core` 启动参数包含：

```bash
./ai-core --enable-gpio
```

说明：当前录音控制链路依赖 GPIO 录音线程，因此需启用 `--enable-gpio`。
`--disable-aicore-physical-actions` 不是录音必要条件，仅在你希望避免物理按键自动动作时可选开启。

## 使用方法

### 交互式运行（回车停止）

```bash
../build/record_audio_example
```

### 脚本方式自动停止（示例：4秒后停止）

```bash
(sleep 4; echo) | ../build/record_audio_example
```

## 预期输出示例

```text
[SAMPLE] record_audio_example started.
[SAMPLE] Target output path: /tmp/test_record_audio
[SAMPLE] Step 1/5: connect audio control socket...
[SAMPLE] Step 2/5: start recording...
[SAMPLE] Recording status after start: RECORDING
[SAMPLE] Step 3/5: recording now. Press ENTER to stop...
[SAMPLE] Step 4/5: stop recording and fetch source path...
[SAMPLE] Source record path from ai-core: /tmp/my_recording.pcm
[SAMPLE] Step 5/5: copy recorded file to /tmp/test_record_audio ...
[SAMPLE][OK] Record file is ready.
[SAMPLE][OK] Read your file from: /tmp/test_record_audio
```

## 文件检查

```bash
ls -l /tmp/test_record_audio
wc -c /tmp/test_record_audio
```

## 返回值

| 返回值 | 说明 |
|---|---|
| `0` | 成功 |
| `1` | 失败（服务端不支持、启动失败、停止失败、复制失败等） |

## 常见问题

### 1) `record start failed: ...`

请检查：

- `ai-core` 是否以 `--enable-gpio` 启动
- 当前是否已有录音任务在运行
- 音频控制 Socket 是否可用（`/tmp/ai-core_audio_ctrl`）

### 2) 生成了 `/tmp/my_recording.pcm` 但没有 `/tmp/test_record_audio`

表示复制步骤失败，请检查 `/tmp` 可写权限和剩余空间。

## 相关 API 文档

- [Audio_Client_API.md](../../docs/Audio_Client_API.md)
