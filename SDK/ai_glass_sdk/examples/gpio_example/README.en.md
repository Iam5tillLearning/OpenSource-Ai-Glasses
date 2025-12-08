# GPIO Example User Guide

[中文版本](README.md) | English Version

## Introduction

`gpio_example` is a demo program showing how to use AI Glass SDK's GPIO event broadcast service to subscribe and handle GPIO button events.

### Core Features
- Subscribe to GPIO button events (Press/Release)
- Asynchronous event callback mechanism, real-time response
- Multi-process sharing same GPIO event source
- No need to access GPIO hardware directly
- Complete event statistics and debug info

## How It Works

### Architecture
```
┌─────────────────┐
│   ai-core Service│  (Monitors GPIO HW)
│  GPIO Manager   │
└────────┬────────┘
         │ Broadcast Event
         ├─────────┐
         │         │
    ┌────▼───┐  ┌─▼──────┐
    │ Client 1│  │ Client 2│
    └────────┘  └────────┘
```

1. **Server** (ai-core): Monitors GPIO hardware state changes
2. **Event Broadcast**: Broadcasts events to all subscribed clients via Unix Domain Socket
3. **Client**: Receives events and executes callback function in independent thread

## Compilation

```bash
cd ai_glass_sdk/examples/gpio_example
make
```

Output: `gpio_example`

## Prerequisites

### 1. Start AI Core Service (Enable GPIO)
```bash
# In service directory
cd service
./build/ai-core --enable-gpio
```

### 2. Confirm GPIO Configuration
Server needs to correctly configure the GPIO pin to monitor (default is gpio-1, corresponding to left temple physical button).

## Usage

### Basic Run
```bash
./gpio_example
```

### Program Output Example
```
═══════════════════════════════════════════════════════════
  GPIO Event Client Example Program
═══════════════════════════════════════════════════════════

【Function】Subscribe to GPIO events, receive button notifications in real-time

【Usage】
  ./gpio_example

【Prerequisites】
  1. Ensure ai-core server is started
  2. Server needs to enable GPIO function (--enable-gpio)

【Exit】
  Press Ctrl+C to exit program

═══════════════════════════════════════════════════════════

═══════════════════════════════════════════════════════════
  GPIO Event Client - Async Callback Mode
═══════════════════════════════════════════════════════════

📝 [Step 1/3] Creating GPIO event client...
✅ Client created

📝 [Step 2/3] Connecting to GPIO event broadcast service...
✅ Connected to service

📝 [Step 3/3] Subscribing to GPIO events...
✅ Subscribed to GPIO events
   - Local Notify Socket: /tmp/gpio_notify_12345
   - Current Event Sequence: 0

═══════════════════════════════════════════════════════════
  🎧 Listening... Please press GPIO button
  💡 Hint: Press Ctrl+C to exit program
═══════════════════════════════════════════════════════════
```

### Button Event Output Example

#### Press Event
```
═══════════════════════════════════════════
  🔴 GPIO1 Button Press Event
───────────────────────────────────────────
  Timestamp: 1234567890123 us
  Press Count: 1
═══════════════════════════════════════════
```

#### Release Event
```
═══════════════════════════════════════════
  ⚪ GPIO1 Button Release Event
───────────────────────────────────────────
  Timestamp: 1234567891234 us
  Release Count: 1
═══════════════════════════════════════════
```

### Exit Program
Press `Ctrl+C` to exit, program will automatically clean up resources:
```
🛑 Received exit signal, preparing to close...

📝 Cleaning up resources...
   - Unregistered notify socket
✅ Resources cleaned up

═══════════════════════════════════════════════════════════
  Program Exited
───────────────────────────────────────────────────────────
  Total Press Count: 5
  Total Release Count: 5
═══════════════════════════════════════════════════════════
```

## Program Function Description

### Event Types
- **GPIO_EVENT_PRESS** - Button Pressed
- **GPIO_EVENT_RELEASE** - Button Released
- **GPIO_EVENT_ERROR** - Error Event

### Callback Function
Program handles events via `my_gpio_event_callback()` function:
```c
void my_gpio_event_callback(gpio_event_t event_type,
                           int gpio_number,
                           void *user_data) {
    // Handle event
}
```

### Features
1. **Real-time Response**: Callback executes in independent thread, does not block main thread
2. **Event Statistics**: Automatically counts press and release times
3. **Heartbeat Check**: Checks service status every 10 seconds
4. **Graceful Exit**: Ctrl+C triggers resource cleanup

## Usage Scenarios

### Scenario 1: Button Trigger Recording
```c
void my_gpio_event_callback(gpio_event_t event_type, int gpio_number, void *user_data) {
    if (event_type == GPIO_EVENT_PRESS) {
        // Start recording
        start_recording();
    } else if (event_type == GPIO_EVENT_RELEASE) {
        // Stop recording
        stop_recording();
    }
}
```

### Scenario 2: Button Counter
```c
static int button_press_count = 0;

void my_gpio_event_callback(gpio_event_t event_type, int gpio_number, void *user_data) {
    if (event_type == GPIO_EVENT_PRESS) {
        button_press_count++;
        printf("Press Count: %d\n", button_press_count);
    }
}
```

### Scenario 3: Long Press Detection
```c
static uint64_t press_timestamp = 0;

void my_gpio_event_callback(gpio_event_t event_type, int gpio_number, void *user_data) {
    if (event_type == GPIO_EVENT_PRESS) {
        press_timestamp = ai_gpio_get_timestamp_us();
    } else if (event_type == GPIO_EVENT_RELEASE) {
        uint64_t duration = ai_gpio_get_timestamp_us() - press_timestamp;
        if (duration > 2000000) {  // 2 seconds
            printf("Long press detected!\n");
        }
    }
}
```

## Error Handling

### Common Errors and Solutions

#### 1. Connection Failed
```
❌ Connection failed, please ensure ai-core is started and GPIO enabled
```
**Solution**:
- Check if `ai-core` service is running
- Confirm started with `--enable-gpio` parameter

#### 2. Subscription Failed
```
❌ Subscription failed
```
**Solution**:
- Check if GPIO manager is correctly initialized
- View server logs for detailed error info

#### 3. Service Stopped
```
⚠️  Service stopped, preparing to exit
```
**Solution**:
- Server exited unexpectedly, restart `ai-core` service

## Return Values

| Value | Description |
|--------|------|
| `0` | Normal exit |
| `-1` | Initialization failed or connection failed |

## Programming Interface

### Core API Functions

```c
// 1. Create client
int ai_gpio_event_client_create(gpio_event_client_t *client);

// 2. Connect to service
int ai_gpio_event_client_connect(gpio_event_client_t *client);

// 3. Subscribe to events (register callback)
int ai_gpio_event_client_subscribe(gpio_event_client_t *client,
                                   gpio_event_callback_t callback,
                                   void *user_data);

// 4. Check service status
int ai_gpio_event_client_is_service_alive(gpio_event_client_t *client);

// 5. Unsubscribe
void ai_gpio_event_client_unsubscribe(gpio_event_client_t *client);

// 6. Disconnect
void ai_gpio_event_client_disconnect(gpio_event_client_t *client);

// 7. Destroy client
void ai_gpio_event_client_destroy(gpio_event_client_t *client);

// Auxiliary function
uint64_t ai_gpio_get_timestamp_us(void);  // Get microsecond timestamp
```

### Complete API Documentation
For detailed programming interface documentation, please refer to:
**📚 [GPIO Client API Development Guide](../../docs/GPIO_Client_API.en.md)**

## Multi-Client Support

Multiple clients can subscribe to the same GPIO event simultaneously:

```bash
# Terminal 1
./gpio_example

# Terminal 2
./gpio_example

# Both clients will receive the same button events
```

## Performance Features

- **Low Latency**: Event broadcast latency typically < 1ms
- **High Concurrency**: Supports multiple clients subscribing simultaneously
- **Lightweight**: Single client occupies minimal memory (< 100KB)

## Debugging Tips

### 1. View Server Log
```bash
# Server will output GPIO state changes
# Check for event broadcast logs
```

### 2. Check Socket Connection
```bash
# View Client Socket
ls -la /tmp/gpio_notify_*

# View Server Socket
ls -la /tmp/ai-core_gpio_*
```

### 3. Enable Verbose Log
Modify log level in source code to see more debug info.

## Notes

1. This program needs to run on target ARM device, cannot execute directly on x86 host
2. Must start `ai-core` service first and enable GPIO function
3. Callback function executes in independent thread, pay attention to thread safety
4. Ensure sufficient permission to access `/tmp` directory

## Related Commands

- View Help: Running program automatically shows usage instructions
- Check Service: `ps aux | grep ai-core`
- View Socket: `ls -la /tmp/gpio_*`

## Related Documentation

- **Programming Interface**: [GPIO Client API Development Guide](../../docs/GPIO_Client_API.en.md)
- **GPIO Architecture**: [GPIO Architecture](../../../docs/server/GPIO_ARCHITECTURE.md) (Note: File might be missing)
- **SDK Documentation**: `../../README.en.md`
- **Header File**: `../../include/ai_gpio.h`
