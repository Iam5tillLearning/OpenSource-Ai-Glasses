# Text Event Client Example

This example demonstrates how to use the AI Glass SDK to subscribe to and handle text event streams from the system (ASR Speech-to-Text and LLM responses).

## Features
When running, the program connects to the `ai-core` server and prints received text messages in real-time:
- **[ASR]**: Displays speech recognition results.
- **[LLM]**: Displays LLM inference responses in a streaming fashion, with automatic line breaks at the end.

## Directory Structure
```
.
├── Makefile       # Build script
├── main.c         # Source code
└── README.md      # Documentation (this file)
```

## Quick Start

### 1. Prerequisites
Ensure the `ai-core` server is running and the relevant text services are enabled.

### 2. Build
Run `make` in the current directory:
```bash
make
```
Upon successful complication, the executable `text_event_client_example` will be generated.

### 3. Run
```bash
./text_event_client_example
```

## Code Walkthrough (main.c)

The main workflow is as follows:

1.  **Create Client**:
    ```c
    ai_text_event_client_t *client = ai_text_event_client_create();
    ```

2.  **Initialize Callback**:
    Register the `on_text_event` function to handle incoming data.
    ```c
    ai_text_event_client_init(client, on_text_event, NULL);
    ```

3.  **Start Connection**:
    ```c
    ai_text_event_client_start(client);
    ```

4.  **Data Processing (Callback)**:
    - Distinguishes between `AI_TEXT_SOURCE_ASR` (Speech Recognition) and `AI_TEXT_SOURCE_LLM` (LLM).
    - For LLM, it detects the `AI_TEXT_TYPE_FINAL` flag to determine if a response sequence has finished.

5.  **Destroy**:
    Call `ai_text_event_client_destroy(client)` to release resources before the program exits.
