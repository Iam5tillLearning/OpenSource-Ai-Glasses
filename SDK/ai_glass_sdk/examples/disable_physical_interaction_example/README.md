# Disable Physical Interaction Example 使用说明

## 简介

`disable_physical_interaction_example` 用于通过 SDK 显式启用：

- `ai_audio_set_sdk_control_mode(client, 1)`

启用后行为：

- AI-Core 不再自动执行物理按键触发的业务动作（录音/拍照/抢话）
- GPIO 事件广播仍保留，SDK 侧可继续订阅和处理

该示例会执行“查询当前状态 -> 设置为启用 -> 再次查询校验”完整流程，并输出清晰状态日志。

## 编译

```bash
cd ai_glass_sdk/examples/disable_physical_interaction_example
make
```

编译产物：

- `../build/disable_physical_interaction_example`

## 前置条件

1. `ai-core` 已运行，且音频控制 Socket 可用（默认 `/tmp/ai-core_audio_ctrl`）
2. `ai-core` 版本支持 `SDK_CONTROL_MODE` 音频控制命令

## 使用方法

### 默认 Socket 路径

```bash
../build/disable_physical_interaction_example
```

### 自定义 Socket 路径

```bash
../build/disable_physical_interaction_example -s /tmp/ai-core_audio_ctrl
```

## 预期输出示例

```text
[SAMPLE] disable_physical_interaction_example started.
[SAMPLE] Step 1/4: connect audio control socket...
[SAMPLE] Step 2/4: query current sdk control mode...
[SAMPLE] Current mode: DISABLED
[SAMPLE] Step 3/4: set sdk control mode to ENABLED...
[SAMPLE] Step 4/4: re-check mode...
[SAMPLE][OK] Physical interaction disable mode is now ENABLED.
[SAMPLE][OK] AI-Core physical auto actions are disabled, GPIO events remain available.
```

## 返回值

| 返回值 | 说明 |
|---|---|
| `0` | 成功 |
| `1` | 失败（连接失败、服务端不支持、状态校验失败等） |

## 常见问题

### 1) 返回 `Server response error (-5)`

说明服务端 `ai-core` 版本不支持该控制命令或协议不匹配。
请升级到支持 `SDK_CONTROL_MODE` 的 `ai-core`。

### 2) 初始化失败

请检查：

- `ai-core` 进程是否在运行
- `/tmp/ai-core_audio_ctrl` 是否存在且可访问

## 相关 API 文档

- [Audio_Client_API.md](../../docs/Audio_Client_API.md)
