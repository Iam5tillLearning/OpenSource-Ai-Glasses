# Disable AI-Core Physical Actions Example Guide

[中文版本](README.md) | English Version

## Introduction

`disable_aicore_physical_actions_example` enables:

- `ai_audio_set_disable_aicore_physical_actions(client, 1)`

After enabling this mode:

- AI-Core stops auto-triggering physical button actions (record/capture/barge-in)
- GPIO event broadcast remains available for SDK consumers

This sample performs full verification flow:

1. Query current mode
2. Set mode to enabled
3. Query again and verify

## Build

```bash
cd ai_glass_sdk/examples/disable_aicore_physical_actions_example
make
```

Output binary:

- `../build/disable_aicore_physical_actions_example`

## Prerequisites

1. `ai-core` is running and audio control socket is available (default: `/tmp/ai-core_audio_ctrl`)
2. `ai-core` version supports `DISABLE_AICORE_PHYSICAL_ACTIONS` command

## Usage

### Default socket

```bash
../build/disable_aicore_physical_actions_example
```

### Custom socket

```bash
../build/disable_aicore_physical_actions_example -s /tmp/ai-core_audio_ctrl
```

## Expected Output

```text
[SAMPLE] disable_aicore_physical_actions_example started.
[SAMPLE] Step 1/4: connect audio control socket...
[SAMPLE] Step 2/4: query current disable_aicore_physical_actions state...
[SAMPLE] Current state: DISABLED
[SAMPLE] Step 3/4: set disable_aicore_physical_actions to ENABLED...
[SAMPLE] Step 4/4: re-check disable_aicore_physical_actions state...
[SAMPLE][OK] disable_aicore_physical_actions is now ENABLED.
[SAMPLE][OK] AI-Core physical auto actions are disabled, GPIO events remain available.
```

## Exit Codes

| Code | Description |
|---|---|
| `0` | Success |
| `1` | Failure (connect/protocol/verification failure) |

## Troubleshooting

### 1) `Server response error (-5)`

Your `ai-core` likely does not support this command/protocol version.
Upgrade to a newer `ai-core` build with `DISABLE_AICORE_PHYSICAL_ACTIONS`.

### 2) Initialization failed

Check:

- `ai-core` process is alive
- `/tmp/ai-core_audio_ctrl` exists and is reachable

## Related API Doc

- [Audio_Client_API.en.md](../../docs/Audio_Client_API.en.md)
