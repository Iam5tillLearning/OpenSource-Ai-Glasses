/**
 * =============================================================================
 * AI Glass SDK - 显示服务客户端API
 * =============================================================================
 *
 * @file ai_display.h
 * @brief AI Glass SDK 显示服务客户端接口头文件
 * @version 1.0
 * @date 2026-01-12
 *
 * 【模块概述】
 * 本模块提供与ai_display_service显示服务进行通信的客户端API。
 * 通过共享内存和Unix Socket实现高性能的帧缓冲提交和显示控制。
 *
 * 【核心特性】
 * - 双缓冲槽位设计，支持主应用和overlay叠加层
 * - 基于共享内存的零拷贝帧缓冲传输
 * - Unix Socket命令通道实现可靠的控制通信
 * - 支持焦点请求，实现多客户端显示优先级管理
 *
 * 【适用场景】
 * - 需要向AI Glass屏幕输出UI内容的应用程序
 * - 多进程显示场景下的焦点切换
 * - 需要低延迟显示更新的实时应用
 *
 * 作者：AI Media Service Team
 */

#ifndef AI_DISPLAY_H
#define AI_DISPLAY_H

#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// 常量定义
// =============================================================================

/**
 * 共享内存配置
 */
#define AI_DISPLAY_SHM_NAME     "/ai_display_shm"
#define AI_DISPLAY_SHM_SIZE     (512 * 1024)  // 512KB

/**
 * 显示参数配置
 */
#define AI_DISPLAY_WIDTH        640           // 显示宽度（像素）
#define AI_DISPLAY_HEIGHT       480           // 显示高度（像素）
#define AI_DISPLAY_BPP          4             // 每像素位数
#define AI_DISPLAY_FRAME_SIZE   (AI_DISPLAY_WIDTH * AI_DISPLAY_HEIGHT / 2) // 153600 bytes

// =============================================================================
// 错误码定义
// =============================================================================

#define AI_DISPLAY_SUCCESS           0    // 成功
#define AI_DISPLAY_ERROR_INIT       -1    // 初始化失败
#define AI_DISPLAY_ERROR_CONNECT    -2    // 连接失败
#define AI_DISPLAY_ERROR_SHM        -3    // 共享内存操作失败
#define AI_DISPLAY_ERROR_SOCKET     -4    // Socket操作失败
#define AI_DISPLAY_ERROR_SEND       -5    // 发送失败
#define AI_DISPLAY_ERROR_PARAM      -6    // 参数错误
#define AI_DISPLAY_ERROR_NOT_READY  -7    // 客户端未就绪

// =============================================================================
// IPC 数据结构
// =============================================================================

/**
 * 共享内存布局结构体
 * 包含魔数校验、活动客户端PID和双帧缓冲槽位
 */
typedef struct {
    uint32_t magic;                  // 0x44495350 = "DISP"
    volatile int active_client_pid;  // 当前在前台显示的进程 PID

    // 两个帧缓冲槽位
    // slot 0: 用于主应用全屏渲染
    uint8_t framebuffer_slot_0[AI_DISPLAY_FRAME_SIZE];
    // slot 1: 用于overlay叠加层或通知
    uint8_t framebuffer_slot_1[AI_DISPLAY_FRAME_SIZE];

} ai_display_shm_t;

/**
 * Socket 通信消息类型枚举
 */
typedef enum {
    AI_DISPLAY_MSG_COMMIT = 1,              // 提交帧更新
    AI_DISPLAY_MSG_REQUEST_FOCUS = 2,       // 请求显示焦点
    AI_DISPLAY_MSG_SET_POWER_SAVE_TIMEOUT = 3  // 设置息屏超时时间
} ai_display_msg_type_t;

/**
 * Socket 通信消息结构体
 */
typedef struct {
    int type;           // ai_display_msg_type_t
    int pid;            // 客户端 PID
    int slot_index;     // 使用的缓冲区槽位 (0 or 1)
    int x, y, w, h;     // 更新区域 (保留字段，暂未使用)
} ai_display_msg_t;

// =============================================================================
// 客户端句柄定义
// =============================================================================

/**
 * 显示客户端句柄（不透明类型）
 */
typedef struct ai_display_client_ctx ai_display_client_t;

// =============================================================================
// 核心 API
// =============================================================================

/**
 * 初始化显示客户端
 * 【功能】创建并初始化客户端上下文结构体
 * 【返回】成功返回客户端句柄，失败返回NULL
 * 【注意】调用此函数后需要调用 ai_display_connect() 连接到服务
 */
ai_display_client_t* ai_display_init(void);

/**
 * 连接到显示服务
 * 【功能】打开共享内存并建立Unix Socket连接
 * 【参数】client - 客户端句柄（由 ai_display_init 返回）
 * 【返回】成功返回 AI_DISPLAY_SUCCESS，失败返回相应错误码
 * 【注意】连接前服务端必须已启动
 */
int ai_display_connect(ai_display_client_t *client);

/**
 * 断开与显示服务的连接
 * 【功能】关闭Socket连接和共享内存映射，但不销毁客户端句柄
 * 【参数】client - 客户端句柄
 * 【注意】断开后可以重新调用 ai_display_connect() 重连
 */
void ai_display_disconnect(ai_display_client_t *client);

/**
 * 获取帧缓冲指针（Slot 0）
 * 【功能】获取主帧缓冲区的内存指针
 * 【参数】client - 客户端句柄
 * 【返回】成功返回帧缓冲指针，失败返回NULL
 * 【注意】大多数全屏应用应使用此函数
 */
uint8_t* ai_display_get_framebuffer(ai_display_client_t *client);

/**
 * 获取指定Slot的帧缓冲指针
 * 【功能】获取指定槽位的帧缓冲区内存指针
 * 【参数】client - 客户端句柄
 *         slot - 槽位索引（0=主缓冲，1=overlay缓冲）
 * 【返回】成功返回帧缓冲指针，失败返回NULL
 */
uint8_t* ai_display_get_framebuffer_slot(ai_display_client_t *client, int slot);

/**
 * 提交帧更新
 * 【功能】通知服务端刷新指定槽位的帧缓冲到屏幕
 * 【参数】client - 客户端句柄
 *         slot_index - 槽位索引（0 或 1）
 *         x, y, w, h - 更新区域（保留参数，当前未使用）
 * 【返回】成功返回 AI_DISPLAY_SUCCESS，失败返回相应错误码
 */
int ai_display_commit_frame(ai_display_client_t *client, int slot_index, int x, int y, int w, int h);

/**
 * 请求显示焦点
 * 【功能】请求将当前客户端设为前台显示
 * 【参数】client - 客户端句柄
 * 【返回】成功返回 AI_DISPLAY_SUCCESS，失败返回相应错误码
 * 【注意】多客户端场景下使用，实现显示优先级切换
 */
int ai_display_request_focus(ai_display_client_t *client);

/**
 * 清理显示客户端资源
 * 【功能】断开连接并释放客户端句柄
 * 【参数】client - 客户端句柄
 * 【注意】调用后句柄将无效，不可再使用
 */
void ai_display_cleanup(ai_display_client_t *client);

/**
 * 设置息屏超时时间
 * 【功能】设置屏幕自动息屏的超时时间
 * 【参数】client - 客户端句柄
 *         timeout_sec - 超时秒数，0表示关闭自动息屏
 * 【返回】成功返回 AI_DISPLAY_SUCCESS，失败返回相应错误码
 * 【注意】设置后立即生效；设为0时会立即唤醒屏幕（如果已息屏）
 */
int ai_display_set_power_save_timeout(ai_display_client_t *client, int timeout_sec);

// =============================================================================
// 辅助函数
// =============================================================================

/**
 * 检查客户端是否已连接
 * 【功能】查询当前客户端的连接状态
 * 【参数】client - 客户端句柄
 * 【返回】已连接返回1，未连接或参数无效返回0
 */
int ai_display_is_connected(ai_display_client_t *client);

/**
 * 获取错误信息字符串
 * 【功能】将错误码转换为可读的错误描述字符串
 * 【参数】error_code - 错误码
 * 【返回】错误描述字符串
 */
const char* ai_display_get_error_string(int error_code);

#ifdef __cplusplus
}
#endif

#endif // AI_DISPLAY_H
