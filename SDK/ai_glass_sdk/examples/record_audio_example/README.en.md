# Record Audio Example Guide

[中文版本](README.md) | English Version

## Introduction

`record_audio_example` demonstrates SDK-driven recording flow:

1. Send `RECORD_START`
2. Wait for ENTER
3. Send `RECORD_STOP`
4. Copy returned record file to fixed path: `/tmp/test_record_audio`

The sample prints clear step-by-step logs and final file path.

## Build

```bash
cd ai_glass_sdk/examples/record_audio_example
make
```

Output binary:

- `../build/record_audio_example`

## Prerequisites

1. `ai-core` is running and supports `RECORD_START/RECORD_STOP`
2. Recommended startup:

```bash
./ai-core --enable-gpio
```

Note: current recording control loop depends on GPIO recording thread, so `--enable-gpio` is required.  
`--disable-aicore-physical-interaction` is optional and only needed when you want to prevent AI-Core auto physical-button actions.

## Usage

### Interactive mode (press ENTER to stop)

```bash
../build/record_audio_example
```

### Script mode auto-stop (example: stop after 4 seconds)

```bash
(sleep 4; echo) | ../build/record_audio_example
```

## Expected Output

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

## Verify Output File

```bash
ls -l /tmp/test_record_audio
wc -c /tmp/test_record_audio
```

## Exit Codes

| Code | Description |
|---|---|
| `0` | Success |
| `1` | Failure (unsupported command/start/stop/copy failure) |

## Troubleshooting

### 1) `record start failed: ...`

Check:

- `ai-core` started with `--enable-gpio`
- no other recording is already running
- `/tmp/ai-core_audio_ctrl` is available

### 2) `/tmp/my_recording.pcm` exists but `/tmp/test_record_audio` missing

The copy step failed. Check `/tmp` write permission and free space.

## Related API Doc

- [Audio_Client_API.en.md](../../docs/Audio_Client_API.en.md)
