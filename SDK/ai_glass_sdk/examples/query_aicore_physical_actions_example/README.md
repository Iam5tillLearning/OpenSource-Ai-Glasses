# Query AI-Core Physical Actions Example 使用说明

## 简介

`query_aicore_physical_actions_example` 用于通过 SDK 只读查询：

- `ai_audio_get_disable_aicore_physical_actions(client, &disabled)`

它不会修改 `ai-core` 当前运行态，适合在现场排查时确认“物理按键自动动作是否被禁用”，避免像 `disable_aicore_physical_actions_example` 那样把状态改成 `ENABLED`。

## 编译

```bash
cd ai_glass_sdk/examples/query_aicore_physical_actions_example
make
```

编译产物：

- `../build/query_aicore_physical_actions_example`

## 前置条件

1. `ai-core` 已运行，且音频控制 Socket 可用（默认 `/tmp/ai-core_audio_ctrl`）
2. `ai-core` 版本支持 `DISABLE_AICORE_PHYSICAL_ACTIONS` 音频控制命令

## 使用方法

### 默认 Socket 路径

```bash
../build/query_aicore_physical_actions_example
```

### 自定义 Socket 路径

```bash
../build/query_aicore_physical_actions_example -s /tmp/ai-core_audio_ctrl
```

## 预期输出示例

```text
[SAMPLE] query_aicore_physical_actions_example started.
[SAMPLE] Step 1/2: connect audio control socket...
[SAMPLE] Step 2/2: query current disable_aicore_physical_actions state...
[SAMPLE][OK] Current disable_aicore_physical_actions: DISABLED
[SAMPLE][OK] Query-only sample, no runtime state was changed.
```

## 返回值

| 返回值 | 说明 |
|---|---|
| `0` | 成功 |
| `1` | 失败（连接失败、服务端不支持、查询失败等） |

## 常见问题

### 1) 返回 `Server response error (-5)`

说明服务端 `ai-core` 版本不支持该控制命令或协议不匹配。
请升级到支持 `DISABLE_AICORE_PHYSICAL_ACTIONS` 的 `ai-core`。

### 2) 初始化失败

请检查：

- `ai-core` 进程是否在运行
- `/tmp/ai-core_audio_ctrl` 是否存在且可访问

## 相关 API 文档

- [Audio_Client_API.md](../../docs/Audio_Client_API.md)
