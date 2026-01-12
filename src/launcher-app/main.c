#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "lvgl/lvgl.h"
#include "../../SDK/ai_glass_sdk/include/ai_display.h"
#include "../../SDK/ai_glass_sdk/include/ai_gpio.h"
#include "ui/ui.h"

// ==================== 屏幕规格 ====================
// 使用 SDK 定义的宏
#define SCREEN_WIDTH  AI_DISPLAY_WIDTH
#define SCREEN_HEIGHT AI_DISPLAY_HEIGHT

// ==================== GPIO 定义 ====================
#define GPIO_PAGE     0    // 翻页键
#define GPIO_CONFIRM  75   // 确认键

// ==================== 全局客户端句柄 ====================
ai_display_client_t *disp_client = NULL;
uint8_t *shm_buf = NULL;

// GPIO 事件客户端 (改为使用两个独立的客户端)
gpio_event_client_t gpio_client_page;    // 负责 GPIO 0
gpio_event_client_t gpio_client_confirm; // 负责 GPIO 75

// ==================== 菜单状态管理 ====================
// 主菜单项枚举
// 简化后的首页菜单项枚举
typedef enum {
    HOME_ITEM_CAMERA = 0,     // 拍照
    HOME_ITEM_RECORD,         // 录像
    HOME_ITEM_MORE,           // 更多
    HOME_ITEM_COUNT
} home_menu_item_t;

// 当前界面状态
typedef enum {
    STATE_HOME,               // 首页 (拍照/录像/更多)
    STATE_SUB_MENU,           // 子菜单 (更多里面)
    STATE_TELEPROMPTER,       // 提词器 (保留逻辑，暂无入口)
} ui_state_t;

static volatile ui_state_t current_state = STATE_HOME;
static volatile int current_menu_index = 0;

// 线程安全标志
static pthread_mutex_t ui_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile int ui_update_pending = 0;
static volatile int pending_gpio_event = -1;

// ==================== UI 更新函数 ====================
// 切换首页菜单高亮显示
void update_home_item_highlight(int index) {
    // 所有文字都保持白色且完全不透明
    if (ui_CameraText) {
        lv_obj_set_style_text_opa(ui_CameraText, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(ui_CameraText, lv_color_white(), 0);
    }
    if (ui_VideoText) {
        lv_obj_set_style_text_opa(ui_VideoText, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(ui_VideoText, lv_color_white(), 0);
        lv_obj_clear_flag(ui_VideoText, LV_OBJ_FLAG_HIDDEN);
    }
    if (ui_MoreText) {
        lv_obj_set_style_text_opa(ui_MoreText, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(ui_MoreText, lv_color_white(), 0);
    }
    
    // 移动选中框到对应位置
    if (ui_SelectionRect) {
        switch (index) {
            case HOME_ITEM_CAMERA:
                lv_obj_set_pos(ui_SelectionRect, 5, 110);    // Camera 列
                break;
            case HOME_ITEM_RECORD:
                lv_obj_set_pos(ui_SelectionRect, 233, 110);  // Record 列
                break;
            case HOME_ITEM_MORE:
                lv_obj_set_pos(ui_SelectionRect, 461, 110);  // More 列
                break;
        }
    }
}

// 切换到指定UI布局用于首页Item
void switch_to_home_item_ui(int index) {
    // 首页现在全部都在 ui_VideoContainer 里 (Camera/Record/More)
    // 所以只需要显示这一个容器
    if (ui_VideoContainer) lv_obj_clear_flag(ui_VideoContainer, LV_OBJ_FLAG_HIDDEN);
    
    update_home_item_highlight(index);
}


void switch_to_state(ui_state_t new_state) {
    if (ui_Navgation) lv_obj_add_flag(ui_Navgation, LV_OBJ_FLAG_HIDDEN);
    if (ui_VideoContainer) lv_obj_add_flag(ui_VideoContainer, LV_OBJ_FLAG_HIDDEN);
    if (ui_subMenu) lv_obj_add_flag(ui_subMenu, LV_OBJ_FLAG_HIDDEN);
    
    switch (new_state) {
        case STATE_HOME:
            if (ui_VideoContainer) lv_obj_clear_flag(ui_VideoContainer, LV_OBJ_FLAG_HIDDEN); // Default to VideoContainer (Camera/Record)
            current_menu_index = 0; // Default to Camera
            switch_to_home_item_ui(current_menu_index);
            break;
            
        case STATE_SUB_MENU:
            if (ui_subMenu) lv_obj_clear_flag(ui_subMenu, LV_OBJ_FLAG_HIDDEN);
            break;
            
        case STATE_TELEPROMPTER:
            // [DELETED] Teleprompter functionality removed
            break;
    }
    
    current_state = new_state;
    printf("[Launcher] State changed to: %d\n", new_state);
}

// ==================== GPIO 事件处理 (核心逻辑) ====================
// 处理翻页键 (GPIO 0)
// 处理翻页键 (GPIO 0)
void handle_page_key(void) {
    pthread_mutex_lock(&ui_mutex);
    
    switch (current_state) {
        case STATE_HOME:
            current_menu_index = (current_menu_index + 1) % HOME_ITEM_COUNT;
            switch_to_home_item_ui(current_menu_index);
            break;
            
        default:
            break;
    }
    
    pthread_mutex_unlock(&ui_mutex);
    printf("[Launcher] Page key processed: menu_index=%d\n", current_menu_index);
}

// 处理确认键 (GPIO 75)
void handle_confirm_key(void) {
    pthread_mutex_lock(&ui_mutex);
    
    switch (current_state) {
        case STATE_HOME:
            if (current_menu_index == HOME_ITEM_CAMERA) {
                printf("[Launcher] ACTION: Camera\n");
            } else if (current_menu_index == HOME_ITEM_RECORD) {
                printf("[Launcher] ACTION: Record\n");
            } else if (current_menu_index == HOME_ITEM_MORE) {
                switch_to_state(STATE_SUB_MENU);
            }
            break;
            
        case STATE_TELEPROMPTER:
            printf("[Launcher] ACTION: Teleprompter Next Page\n");
            break;
            
        case STATE_SUB_MENU:
            switch_to_state(STATE_HOME);
            break;
    }
    
    pthread_mutex_unlock(&ui_mutex);
}

// ==================== GPIO 事件回调 (各自分开) ====================

// GPIO 0 回调 (翻页)
void gpio_page_callback(gpio_event_t event_type, int gpio_number, void *user_data) {
    printf("[Launcher] GPIO%d Event received (type=%d)\n", gpio_number, event_type);
    fflush(stdout);

    if (event_type == GPIO_EVENT_PRESS) {
        pthread_mutex_lock(&ui_mutex);
        pending_gpio_event = GPIO_PAGE;
        ui_update_pending = 1;
        pthread_mutex_unlock(&ui_mutex);
    }
}

// GPIO 75 回调 (确认)
void gpio_confirm_callback(gpio_event_t event_type, int gpio_number, void *user_data) {
    printf("[Launcher] GPIO%d Event received (type=%d)\n", gpio_number, event_type);
    fflush(stdout);

    if (event_type == GPIO_EVENT_PRESS) {
        pthread_mutex_lock(&ui_mutex);
        pending_gpio_event = GPIO_CONFIRM;
        ui_update_pending = 1;
        pthread_mutex_unlock(&ui_mutex);
    }
}

// ==================== LVGL Flush Callback ====================
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p) {
    if (!shm_buf) {
        lv_disp_flush_ready(disp_drv);
        return;
    }

    int32_t w = area->x2 - area->x1 + 1;
    int32_t h = area->y2 - area->y1 + 1;
    
    // Copy data to SHM (simplified for brevity, assume ai_display handles it or do manual copy)
    for(int y = 0; y < h; y++) {
        int shm_y = area->y1 + y;
        int shm_x_start = area->x1;
        
        uint8_t * dst_row_ptr = shm_buf + (shm_y * 320) + (shm_x_start / 2);
        const lv_color_t * src_row_ptr = color_p + (y * w);
        
        for (int i = 0; i < w; i += 2) {
             uint8_t p1 = (src_row_ptr[i].full & 0xF0);
             uint8_t p2 = (i + 1 < w) ? ((src_row_ptr[i+1].full & 0xF0) >> 4) : 0;
             dst_row_ptr[i/2] = p1 | p2;
        }
    }
    
    // Commit ONLY when the last part of the frame is flushed to avoid flooding the server/driver
    if (lv_disp_flush_is_last(disp_drv)) {
        ai_display_commit_frame(disp_client, 0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    }

    lv_disp_flush_ready(disp_drv);
}

// ==================== 初始化函数 ====================
int init_display_client(void) {
    disp_client = ai_display_init();
    if (!disp_client) {
        printf("[Launcher] ERROR: Failed to init display client\n");
        return -1;
    }
    
    int ret = ai_display_connect(disp_client);
    if (ret != AI_DISPLAY_SUCCESS) {
        printf("[Launcher] ERROR: Failed to connect to display service: %s\n", ai_display_get_error_string(ret));
        return -1;
    }
    
    shm_buf = ai_display_get_framebuffer(disp_client);
    if (!shm_buf) {
        printf("[Launcher] ERROR: Failed to get framebuffer\n");
        return -1;
    }
    
    ai_display_request_focus(disp_client);
    printf("[Launcher] Display client initialized\n");
    return 0;
}

int init_gpio_clients(void) {
    // 1. 初始化并连接 GPIO 0 (翻页)
    if (ai_gpio_event_client_create(&gpio_client_page) == 0) {
        if (ai_gpio_event_client_connect_gpio(&gpio_client_page, GPIO_PAGE) == 0) {
            ai_gpio_event_client_subscribe(&gpio_client_page, gpio_page_callback, NULL);
            printf("[Launcher] Connected to GPIO %d (Page)\n", GPIO_PAGE);
        } else {
            printf("[Launcher] Failed to connect to GPIO %d\n", GPIO_PAGE);
        }
    }

    // 2. 初始化并连接 GPIO 75 (确认)
    if (ai_gpio_event_client_create(&gpio_client_confirm) == 0) {
        if (ai_gpio_event_client_connect_gpio(&gpio_client_confirm, GPIO_CONFIRM) == 0) {
            ai_gpio_event_client_subscribe(&gpio_client_confirm, gpio_confirm_callback, NULL);
            printf("[Launcher] Connected to GPIO %d (Confirm)\n", GPIO_CONFIRM);
        } else {
            printf("[Launcher] Failed to connect to GPIO %d\n", GPIO_CONFIRM);
        }
    }
    
    return 0;
}

int init_lvgl(void) {
    lv_init();
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf1[SCREEN_WIDTH * 10];
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, SCREEN_WIDTH * 10);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    return 0;
}

// ==================== 清理函数 ====================
void cleanup(void) {
    ai_gpio_event_client_unsubscribe(&gpio_client_page);
    ai_gpio_event_client_destroy(&gpio_client_page);
    
    ai_gpio_event_client_unsubscribe(&gpio_client_confirm);
    ai_gpio_event_client_destroy(&gpio_client_confirm);
    
    ai_display_disconnect(disp_client);
    pthread_mutex_destroy(&ui_mutex);
    printf("[Launcher] Cleanup complete\n");
}

// ==================== 主函数 ====================
int main(void) {
    setbuf(stdout, NULL); // 禁用 stdout 缓冲，确保立即输出
    printf("Launcher App Starting...\n");

    if (init_display_client() != 0) return -1;
    init_gpio_clients();
    if (init_lvgl() != 0) return -1;

    ui_init();
    switch_to_state(STATE_HOME);

    printf("[Launcher] Entering main loop\n");
    
    while(1) {
        if (ui_update_pending) {
            pthread_mutex_lock(&ui_mutex);
            int gpio = pending_gpio_event;
            pending_gpio_event = -1;
            ui_update_pending = 0;
            pthread_mutex_unlock(&ui_mutex);
            
            if (gpio == GPIO_PAGE) {
                handle_page_key();
            } else if (gpio == GPIO_CONFIRM) {
                handle_confirm_key();
            }
        }
        
        lv_tick_inc(5); // Tell LVGL that 5ms has passed
        lv_timer_handler();
        usleep(5000);
    }

    cleanup();
    return 0;
}
