/**
 * =============================================================================
 * AI语音交互系统 - GPIO事件广播服务模块
 * =============================================================================
 *
 * 【模块概述】
 * 该模块实现了GPIO事件的广播服务，解决多个进程无法同时监听同一个GPIO的问题。
 * 采用"单一服务进程 + 多客户端订阅"的架构，通过共享内存和Unix Socket实现
 * 低延迟的事件分发。
 *
 * 【核心特性】
 * - 单一进程独占GPIO硬件访问，避免资源竞争
 * - 支持多个客户端进程同时订阅GPIO事件
 * - 基于共享内存的高性能事件传递（延迟 < 10ms）
 * - 事件环形队列支持历史事件回溯
 * - 自动错误恢复和客户端重连机制
 *
 * 【适用场景】
 * - 多个进程需要响应同一个GPIO按键事件
 * - 需要在进程间共享GPIO状态信息
 * - 需要低延迟的GPIO事件通知
 *
 * 作者：AI Media Service Team
 * 版本：v1.0
 * 日期：2025-10-09
 */

#ifndef ai_gpio_H
#define ai_gpio_H

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// 头文件依赖
// =============================================================================
#include <stdint.h>
#include <pthread.h>
#include "ai_ipc.h"           // 共享内存和Unix Socket

// =============================================================================
// GPIO事件类型定义
// =============================================================================

/**
 * GPIO事件类型枚举
 */
typedef enum {
    GPIO_EVENT_PRESS    = 1,        // 按键按下事件
    GPIO_EVENT_RELEASE  = 2,        // 按键释放事件
    GPIO_EVENT_ERROR    = 3         // GPIO错误事件
} gpio_event_t;

/**
 * GPIO事件回调函数类型定义
 * @param event_type - 事件类型（按下/释放/错误）
 * @param gpio_number - GPIO编号
 * @param user_data - 用户自定义数据指针
 */
typedef void (*gpio_event_callback_t)(gpio_event_t event_type, int gpio_number, void *user_data);

// =============================================================================
// 常量定义
// =============================================================================

/**
 * GPIO事件服务配置常量
 */
#define GPIO_EVENT_SHM_NAME            "/ai_gpio_event_shm"          // 共享内存名称（向后兼容，默认GPIO1）
#define GPIO_EVENT_SHM_SIZE            4096                          // 共享内存大小（4KB）
#define GPIO_EVENT_SOCKET_PATH         "/tmp/ai_gpio_event_broadcast" // Unix Socket路径（向后兼容，默认GPIO1）

/**
 * 多GPIO支持路径模板（v1.1新增）
 * 用法：snprintf(buffer, size, GPIO_EVENT_SHM_NAME_FMT, gpio_number)
 */
#define GPIO_EVENT_MAX_GPIO_COUNT      8                              // 最大支持的GPIO数量
#define GPIO_EVENT_SHM_NAME_FMT        "/ai_gpio_event_shm_%d"        // 共享内存名称模板
#define GPIO_EVENT_SOCKET_PATH_FMT     "/tmp/ai_gpio_event_broadcast_%d" // Unix Socket路径模板

#define GPIO_EVENT_CLIENT_PATH_MAX     108                            // 客户端回调Socket路径长度
#define GPIO_EVENT_REGISTER_CMD        "GPIO_EVENT_REGISTER"         // 客户端注册命令
#define GPIO_EVENT_UNREGISTER_CMD      "GPIO_EVENT_UNREGISTER"       // 客户端注销命令
#define GPIO_EVENT_QUEUE_SIZE          32                            // 事件队列大小（2的幂次方）
#define GPIO_EVENT_MAX_CLIENTS         64                            // 最大客户端数量
#define GPIO_EVENT_HEARTBEAT_INTERVAL  1000                          // 心跳间隔（毫秒）
#define GPIO_EVENT_RECONNECT_DELAY     500                           // 重连延迟（毫秒）


// =============================================================================
// 数据结构定义 - 共享内存部分
// =============================================================================

/**
 * GPIO事件数据结构（存储在共享内存中）
 * 用于在服务端和客户端之间传递GPIO事件信息
 */
typedef struct {
    // 事件元数据
    gpio_event_t event_type;        // 事件类型（按下/释放/错误）
    int          gpio_number;       // GPIO编号
    uint64_t     timestamp_us;      // 事件时间戳（微秒级精度）
    uint32_t     sequence;          // 事件序列号（用于客户端追踪）

    // 扩展信息
    int          press_duration_ms; // 按下持续时间（毫秒，仅释放事件有效）
    int          error_code;        // 错误码（仅错误事件有效）
} gpio_event_data_t;

/**
 * GPIO事件环形队列
 * 用于缓存最近的GPIO事件历史，支持客户端掉线重连后恢复事件
 */
typedef struct {
    gpio_event_data_t events[GPIO_EVENT_QUEUE_SIZE];  // 事件缓冲区
    volatile uint32_t write_index;                    // 写入索引（仅服务端写入）
    volatile uint32_t read_index;                     // 读取索引（客户端各自维护）
} gpio_event_queue_t;

/**
 * GPIO事件共享内存魔数（用于校验）
 */
#define GPIO_EVENT_SHM_MAGIC 0x47504F49  // "GPOI"

/**
 * GPIO状态共享内存结构（主数据结构）
 * 这是服务端和所有客户端共享的核心数据结构
 */
typedef struct {
    // 魔数校验（必须放在第一个字段）
    volatile uint32_t magic;              // 魔数，用于验证共享内存已初始化

    // 服务状态
    volatile int     service_running;     // 服务运行状态（1=运行中, 0=已停止）
    volatile int     gpio_number;         // 当前监控的GPIO编号
    volatile int     current_state;       // 当前GPIO电平状态（0=低电平, 1=高电平）
    volatile int     is_pressed;          // 按键逻辑状态（0=释放, 1=按下）

    // 最新事件（快速访问路径）
    gpio_event_data_t latest_event;       // 最新事件，客户端可直接读取

    // 事件队列（历史记录）
    gpio_event_queue_t event_queue;       // 事件环形队列

    // 客户端管理
    volatile int     client_count;        // 当前连接的客户端数量
    volatile uint32_t broadcast_sequence; // 广播序列号（每次事件递增）

    // 统计信息
    uint64_t         total_press_count;   // 累计按下次数
    uint64_t         total_release_count; // 累计释放次数
    uint64_t         total_error_count;   // 累计错误次数

    // 时间戳（用于心跳检测和同步）
    uint64_t         service_start_time;  // 服务启动时间（微秒）
    uint64_t         last_event_time;     // 最后事件时间（微秒）
    uint64_t         last_heartbeat_time; // 最后心跳时间（秒）

} gpio_event_shm_t;

// =============================================================================
// 数据结构定义 - 服务端部分
// =============================================================================

/**
 * 客户端连接信息
 */
typedef struct {
    char socket_path[GPIO_EVENT_CLIENT_PATH_MAX]; // 客户端回调Unix Socket路径
    uint32_t last_sequence;               // 客户端最后接收的序列号
    int is_active;                        // 是否已注册
    int notify_fail_count;                // 连续通知失败次数
} gpio_client_info_t;

/**
 * GPIO事件广播服务结构体（服务端使用）
 */
typedef struct {
    // 【IPC资源】
    ai_shared_memory_t    shm;              // 共享内存描述符
    gpio_event_shm_t     *shm_ptr;          // 共享内存数据指针
    ai_unix_socket_server_t broadcast_server; // Unix Socket广播服务器

    // 【GPIO监控】
    pthread_t             monitor_thread;   // GPIO硬件监控线程
    int                   monitor_running;  // 监控线程运行状态

    // 【心跳线程】
    pthread_t             heartbeat_thread; // 心跳维护线程
    int                   heartbeat_running; // 心跳线程运行状态

    // 【服务状态】
    int                   service_initialized; // 服务初始化状态
    int                   gpio_number;      // 监控的GPIO编号
    const char           *gpio_debug_path;  // GPIO调试路径
    int                   poll_interval_ms; // 轮询间隔（毫秒）
    int                   use_external_events; // 是否由外部推送事件

    // 【客户端管理】
    gpio_client_info_t    clients[GPIO_EVENT_MAX_CLIENTS]; // 客户端列表
    int                   client_count;     // 当前客户端数量
    pthread_mutex_t       client_mutex;     // 客户端列表保护锁

    // 【线程安全】
    pthread_mutex_t       service_mutex;    // 服务状态保护锁
    volatile int          should_stop;      // 停止信号

    // 【性能统计】
    uint64_t              broadcast_count;  // 广播次数统计

} gpio_event_service_t;

// =============================================================================
// 数据结构定义 - 客户端部分
// =============================================================================

/**
 * GPIO事件订阅客户端结构体（客户端进程使用）
 */
typedef struct {
    // 【IPC连接】
    ai_shared_memory_t    shm;              // 共享内存连接
    gpio_event_shm_t     *shm_ptr;          // 共享内存数据指针

    // 【事件追踪】
    uint32_t              last_sequence;    // 最后处理的事件序列号

    // 【回调函数】
    gpio_event_callback_t event_callback;   // 用户事件回调
    void                 *user_data;        // 用户数据

    // 【客户端通知服务】
    ai_unix_socket_server_t notify_server;  // 客户端用于接收通知的Unix Socket服务器
    int                   notify_server_started; // 通知服务是否已启动
    char                  notify_socket_path[GPIO_EVENT_CLIENT_PATH_MAX]; // 通知Socket路径

    // 【订阅状态】
    volatile int          is_listening;     // 是否已订阅

    // 【状态】
    int                   is_connected;     // 连接状态

} gpio_event_client_t;

// =============================================================================
// 服务端API - 运行在ai-core进程中
// =============================================================================

/**
 * 初始化GPIO事件广播服务
 * 【功能】创建共享内存、Unix Socket服务器，初始化GPIO管理器
 * 【参数】service - 服务描述符指针
 *         gpio_number - 要监控的GPIO编号
 *         gpio_debug_path - GPIO调试路径（可选，传NULL使用默认）
 *         poll_interval_ms - 轮询间隔（毫秒，<=0使用默认值50ms）
 * 【返回】成功返回0，失败返回-1
 * 【注意】此函数不启动监控线程，需要调用ai_gpio_start()启动
 */
int ai_gpio_init(
    gpio_event_service_t *service,
    int gpio_number,
    const char *gpio_debug_path,
    int poll_interval_ms,
    int enable_external_events
);

/**
 * 启动GPIO事件广播服务（非阻塞）
 * 【功能】启动GPIO监控线程和Unix Socket服务器线程
 * 【参数】service - 服务描述符指针
 * 【返回】成功返回0，失败返回-1
 * 【注意】此函数会创建两个后台线程：GPIO监控线程和Socket服务线程
 */
int ai_gpio_start(gpio_event_service_t *service);

/**
 * 停止GPIO事件广播服务
 * 【功能】停止所有线程，关闭所有客户端连接
 * 【参数】service - 服务描述符指针
 * 【注意】此函数会等待所有线程正常退出
 */
void ai_gpio_stop(gpio_event_service_t *service);

/**
 * 清理GPIO事件广播服务资源
 * 【功能】释放共享内存、关闭Socket、销毁互斥锁
 * 【参数】service - 服务描述符指针
 * 【注意】调用此函数前应先调用ai_gpio_stop()
 */
void ai_gpio_cleanup(gpio_event_service_t *service);

/**
 * 获取服务统计信息
 * 【功能】查询服务运行统计数据
 * 【参数】service - 服务描述符指针
 *         press_count - 输出按下次数（可选，传NULL跳过）
 *         release_count - 输出释放次数（可选，传NULL跳过）
 *         client_count - 输出客户端数量（可选，传NULL跳过）
 */
void ai_gpio_get_stats(
    gpio_event_service_t *service,
    uint64_t *press_count,
    uint64_t *release_count,
    int *client_count
);

// =============================================================================
// 客户端API - 运行在其他进程中
// =============================================================================

/**
 * 创建GPIO事件订阅客户端
 * 【功能】初始化客户端结构体
 * 【参数】client - 客户端描述符指针
 * 【返回】成功返回0，失败返回-1
 */
int ai_gpio_event_client_create(gpio_event_client_t *client);

/**
 * 连接到GPIO事件广播服务（默认GPIO1）
 * 【功能】打开共享内存并连接到Unix Socket服务器
 * 【参数】client - 客户端描述符指针
 * 【返回】成功返回0，失败返回-1
 * 【注意】如果服务未启动，此函数会失败。此函数默认连接GPIO1（向后兼容）。
 */
int ai_gpio_event_client_connect(gpio_event_client_t *client);

/**
 * 连接到指定GPIO的事件广播服务（v1.1新增）
 * 【功能】连接到指定GPIO编号的广播服务
 * 【参数】client - 客户端描述符指针
 *         gpio_number - 要连接的GPIO编号
 * 【返回】成功返回0，失败返回-1
 * 【示例】ai_gpio_event_client_connect_gpio(&client, 0) - 连接GPIO0
 */
int ai_gpio_event_client_connect_gpio(gpio_event_client_t *client, int gpio_number);


/**
 * 订阅GPIO事件（启动监听线程）
 * 【功能】启动后台线程监听GPIO事件，通过回调函数异步通知
 * 【参数】client - 客户端描述符指针
 *         event_callback - 事件回调函数
 *         user_data - 用户数据指针（会传递给回调函数）
 * 【返回】成功返回0，失败返回-1
 * 【回调】回调函数在独立线程中执行，需要注意线程安全
 */
int ai_gpio_event_client_subscribe(
    gpio_event_client_t *client,
    gpio_event_callback_t event_callback,
    void *user_data
);

/**
 * 取消订阅GPIO事件
 * 【功能】停止监听线程
 * 【参数】client - 客户端描述符指针
 * 【注意】此函数会等待监听线程正常退出
 */
void ai_gpio_event_client_unsubscribe(gpio_event_client_t *client);

/**
 * 断开与服务的连接
 * 【功能】关闭Socket连接，但保持共享内存映射
 * 【参数】client - 客户端描述符指针
 */
void ai_gpio_event_client_disconnect(gpio_event_client_t *client);

/**
 * 销毁客户端并清理资源
 * 【功能】释放所有资源，包括共享内存映射
 * 【参数】client - 客户端描述符指针
 * 【注意】调用此函数前应先调用ai_gpio_event_client_unsubscribe()
 */
void ai_gpio_event_client_destroy(gpio_event_client_t *client);

/**
 * 检查服务是否可用
 * 【功能】检查GPIO事件广播服务是否正在运行
 * 【参数】client - 客户端描述符指针
 * 【返回】服务可用返回1，不可用返回0
 */
int ai_gpio_event_client_is_service_alive(gpio_event_client_t *client);

/**
 * 发布GPIO事件（外部触发）
 * 【功能】由外部模块主动触发事件广播
 * 【参数】service - 服务描述符指针
 *         event_type - 事件类型（按下/释放/错误）
 *         gpio_number - GPIO编号
 * 【返回】成功返回0，失败返回-1
 */
int ai_gpio_publish_event(
    gpio_event_service_t *service,
    gpio_event_t event_type,
    int gpio_number
);

// =============================================================================
// 辅助工具函数
// =============================================================================

/**
 * 获取当前时间戳（微秒）
 * 【功能】获取高精度时间戳，用于事件时间记录
 * 【返回】当前时间的微秒数
 */
uint64_t ai_gpio_get_timestamp_us(void);

/**
 * 获取当前时间戳（秒）
 * 【功能】获取秒级时间戳，用于心跳检测
 * 【返回】当前时间的秒数
 */
uint64_t ai_gpio_get_timestamp_sec(void);

/**
 * 将gpio_event_t转换为字符串
 * 【功能】调试辅助函数
 * 【参数】event_type - GPIO事件类型
 * 【返回】事件类型字符串
 */
const char* ai_gpio_event_type_to_string(gpio_event_t event_type);

/**
 * 获取指定GPIO的共享内存名称（v1.1新增）
 * 【功能】生成带GPIO编号后缀的共享内存名称
 * 【参数】gpio_number - GPIO编号
 *         buffer - 输出缓冲区
 *         buffer_size - 缓冲区大小
 */
void ai_gpio_get_shm_name(int gpio_number, char *buffer, size_t buffer_size);

/**
 * 获取指定GPIO的Socket路径（v1.1新增）
 * 【功能】生成带GPIO编号后缀的Unix Socket路径
 * 【参数】gpio_number - GPIO编号
 *         buffer - 输出缓冲区
 *         buffer_size - 缓冲区大小
 */
void ai_gpio_get_socket_path(int gpio_number, char *buffer, size_t buffer_size);

// =============================================================================
// GPIO事件中心常量定义（v2.0新增）
// =============================================================================

#define GPIO_HUB_SHM_NAME              "/ai_gpio_event_hub"           // 事件中心共享内存
#define GPIO_HUB_SOCKET_PATH           "/tmp/ai_gpio_event_hub_broadcast" // 事件中心Socket
#define GPIO_HUB_SHM_MAGIC             0x47504855                     // 魔数："GPHU"
#define GPIO_HUB_MAX_GPIO              16                             // 最大GPIO数量
#define GPIO_HUB_EVENT_QUEUE_SIZE      64                             // 事件队列大小
#define GPIO_HUB_SHM_SIZE              8192                           // 共享内存大小

// =============================================================================
// GPIO事件中心共享内存结构（v2.0新增）
// =============================================================================

/**
 * 单个GPIO的状态信息
 */
typedef struct {
    int          gpio_number;        // GPIO编号（-1表示槽位未使用）
    int          is_active;          // 是否已激活监控
    int          current_state;      // 当前电平状态
    int          is_pressed;         // 按键逻辑状态
    uint64_t     last_event_time;    // 最后事件时间（微秒）
    uint64_t     press_count;        // 按下次数统计
    uint64_t     release_count;      // 释放次数统计
} gpio_hub_gpio_state_t;

/**
 * GPIO事件中心共享内存结构
 */
typedef struct {
    volatile uint32_t magic;                // 魔数校验
    volatile int     service_running;       // 服务运行状态
    uint64_t         service_start_time;    // 服务启动时间
    uint64_t         last_heartbeat_time;   // 最后心跳时间

    // GPIO状态数组
    gpio_hub_gpio_state_t gpio_states[GPIO_HUB_MAX_GPIO];
    int              active_gpio_count;     // 已激活GPIO数量

    // 统一事件队列
    gpio_event_data_t event_queue[GPIO_HUB_EVENT_QUEUE_SIZE];
    volatile uint32_t queue_write_index;    // 写入索引

    // 广播控制
    volatile uint32_t broadcast_sequence;   // 广播序列号

    // 统计信息
    uint64_t         total_event_count;     // 总事件数

    // 客户端管理
    volatile int     client_count;          // 客户端数量
} gpio_event_hub_shm_t;

// =============================================================================
// GPIO事件中心客户端数据结构（v2.0新增）
// =============================================================================

/**
 * GPIO事件中心订阅客户端结构体
 * 【功能】连接到事件中心，接收所有或指定GPIO的事件
 */
typedef struct {
    // 【IPC连接】
    ai_shared_memory_t    shm;              // 共享内存连接
    gpio_event_hub_shm_t *shm_ptr;          // 共享内存数据指针

    // 【订阅配置】
    int                   subscribed_gpios[GPIO_HUB_MAX_GPIO];  // 订阅的GPIO列表
    int                   subscribed_count;  // 订阅数量（0表示订阅全部）
    int                   subscribe_all;     // 是否订阅全部GPIO

    // 【事件追踪】
    uint32_t              last_sequence;     // 最后处理的事件序列号

    // 【回调函数】
    gpio_event_callback_t event_callback;    // 用户事件回调
    void                 *user_data;         // 用户数据

    // 【客户端通知服务】
    ai_unix_socket_server_t notify_server;   // 通知Socket服务器
    int                   notify_server_started;
    char                  notify_socket_path[108];

    // 【状态】
    volatile int          is_listening;      // 是否已订阅
    int                   is_connected;      // 连接状态
} gpio_event_hub_client_t;

// =============================================================================
// GPIO事件中心客户端API（v2.0新增）
// =============================================================================

/**
 * 创建GPIO事件中心客户端
 * 【功能】初始化客户端结构体
 * 【参数】client - 客户端描述符指针
 * 【返回】成功返回0，失败返回-1
 */
int ai_gpio_hub_client_create(gpio_event_hub_client_t *client);

/**
 * 连接到GPIO事件中心
 * 【功能】打开共享内存连接
 * 【参数】client - 客户端描述符指针
 * 【返回】成功返回0，失败返回-1
 */
int ai_gpio_hub_client_connect(gpio_event_hub_client_t *client);

/**
 * 订阅指定GPIO列表的事件
 * 【功能】订阅指定GPIO列表的事件，当这些GPIO状态变化时触发回调
 * 【参数】client - 客户端描述符指针
 *         gpio_list - GPIO编号数组
 *         gpio_count - 数组大小
 *         callback - 事件回调函数
 *         user_data - 用户数据指针
 * 【返回】成功返回0，失败返回-1
 */
int ai_gpio_hub_client_subscribe_gpios(
    gpio_event_hub_client_t *client,
    const int *gpio_list,
    int gpio_count,
    gpio_event_callback_t callback,
    void *user_data
);

/**
 * 订阅所有GPIO事件
 * 【功能】订阅事件中心管理的所有GPIO事件
 * 【参数】client - 客户端描述符指针
 *         callback - 事件回调函数
 *         user_data - 用户数据指针
 * 【返回】成功返回0，失败返回-1
 */
int ai_gpio_hub_client_subscribe_all(
    gpio_event_hub_client_t *client,
    gpio_event_callback_t callback,
    void *user_data
);

/**
 * 获取指定GPIO的状态
 * 【功能】从共享内存读取GPIO状态
 * 【参数】client - 客户端描述符指针
 *         gpio_number - GPIO编号
 * 【返回】GPIO状态（0=释放，1=按下），失败返回-1
 */
int ai_gpio_hub_client_get_gpio_state(
    gpio_event_hub_client_t *client,
    int gpio_number
);

/**
 * 获取所有活跃GPIO的列表
 * 【功能】获取事件中心当前监控的所有GPIO编号
 * 【参数】client - 客户端描述符指针
 *         gpio_list - 输出GPIO编号数组
 *         max_count - 数组最大容量
 * 【返回】活跃GPIO数量（可能小于max_count）
 */
int ai_gpio_hub_client_get_active_gpios(
    gpio_event_hub_client_t *client,
    int *gpio_list,
    int max_count
);

/**
 * 取消订阅
 * 【功能】停止接收GPIO事件
 * 【参数】client - 客户端描述符指针
 */
void ai_gpio_hub_client_unsubscribe(gpio_event_hub_client_t *client);

/**
 * 断开与事件中心的连接
 * 【功能】关闭共享内存连接
 * 【参数】client - 客户端描述符指针
 */
void ai_gpio_hub_client_disconnect(gpio_event_hub_client_t *client);

/**
 * 销毁客户端并清理资源
 * 【功能】释放所有资源
 * 【参数】client - 客户端描述符指针
 */
void ai_gpio_hub_client_destroy(gpio_event_hub_client_t *client);

/**
 * 检查事件中心服务是否可用
 * 【功能】检查事件中心是否正在运行
 * 【参数】client - 客户端描述符指针
 * 【返回】服务可用返回1，不可用返回0
 */
int ai_gpio_hub_client_is_service_alive(gpio_event_hub_client_t *client);

#ifdef __cplusplus
}
#endif

#endif // ai_gpio_H
