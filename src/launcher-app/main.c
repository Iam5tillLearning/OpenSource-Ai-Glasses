#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "lvgl/lvgl.h"
#include "../../SDK/ai_glass_sdk/include/ai_ble.h"
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
static ai_ble_client_t *ble_client = NULL;

// GPIO 事件客户端 (v2.0 Hub 架构)
gpio_event_hub_client_t gpio_hub_client;

// BLE 文本显示层
static lv_obj_t *ble_text_overlay = NULL;
static lv_obj_t *ble_text_label = NULL;
static volatile int ble_text_visible = 0;
static volatile int ble_text_pending = 0;
static char pending_ble_text[AI_BLE_MAX_DATA_LEN + 1] = {0};

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

static void hide_ble_text_overlay(void) {
    if (!ble_text_overlay) {
        return;
    }

    lv_obj_add_flag(ble_text_overlay, LV_OBJ_FLAG_HIDDEN);
    ble_text_visible = 0;
}

static void show_ble_text_overlay(const char *text) {
    if (!ble_text_overlay || !ble_text_label) {
        return;
    }

    lv_label_set_text(ble_text_label, text ? text : "");
    lv_obj_clear_flag(ble_text_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(ble_text_overlay);
    ble_text_visible = 1;
    printf("[Launcher][BLE] UI updated: %s\n", text ? text : "");
}

static void init_ble_text_overlay(void) {
    if (!ui_Screen1 || ble_text_overlay) {
        return;
    }

    ble_text_overlay = lv_obj_create(ui_Screen1);
    lv_obj_set_size(ble_text_overlay, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(ble_text_overlay, 0, 0);
    lv_obj_set_style_bg_color(ble_text_overlay, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ble_text_overlay, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ble_text_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ble_text_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ble_text_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ble_text_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(ble_text_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ble_text_overlay, LV_OBJ_FLAG_HIDDEN);

    ble_text_label = lv_label_create(ble_text_overlay);
    lv_obj_set_width(ble_text_label, SCREEN_WIDTH - 80);
    lv_obj_center(ble_text_label);
    lv_label_set_long_mode(ble_text_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(ble_text_label, "");
    lv_obj_set_style_text_font(ble_text_label, &ui_font_alibaba_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ble_text_label, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ble_text_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ble_text_label, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void on_ble_display_text(const char *datatype, const char *data, void *user_data) {
    (void)user_data;

    pthread_mutex_lock(&ui_mutex);
    strncpy(pending_ble_text, data ? data : "", AI_BLE_MAX_DATA_LEN);
    pending_ble_text[AI_BLE_MAX_DATA_LEN] = '\0';
    ble_text_pending = 1;
    pthread_mutex_unlock(&ui_mutex);

    printf("[Launcher][BLE] recv %s => %s\n", datatype, pending_ble_text);
}

static int init_ble_client(void) {
    ble_client = ai_ble_client_create();
    if (!ble_client) {
        printf("[Launcher][BLE] ERROR: Failed to create client\n");
        return -1;
    }

    if (ai_ble_client_start(ble_client) != 0) {
        printf("[Launcher][BLE] ERROR: Failed to start client\n");
        ai_ble_client_destroy(ble_client);
        ble_client = NULL;
        return -1;
    }

    if (ai_ble_register_datatype(ble_client, "display.text", on_ble_display_text, NULL) != 0) {
        printf("[Launcher][BLE] ERROR: Failed to register datatype display.text\n");
        ai_ble_client_destroy(ble_client);
        ble_client = NULL;
        return -1;
    }

    printf("[Launcher][BLE] Registered datatype: display.text\n");
    return 0;
}

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

    if (ble_text_visible) {
        hide_ble_text_overlay();
        pthread_mutex_unlock(&ui_mutex);
        printf("[Launcher] Page key dismissed BLE text overlay\n");
        return;
    }
    
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

    if (ble_text_visible) {
        hide_ble_text_overlay();
        pthread_mutex_unlock(&ui_mutex);
        printf("[Launcher] Confirm key dismissed BLE text overlay\n");
        return;
    }
    
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

// ==================== GPIO 事件回调 (统一回调) ====================

// v2.0 GPIO Hub 统一回调
void gpio_hub_callback(gpio_event_t event_type, int gpio_number, void *user_data) {
    printf("[Launcher] GPIO%d Event received (type=%d)\n", gpio_number, event_type);
    fflush(stdout);

    if (event_type == GPIO_EVENT_PRESS) {
        pthread_mutex_lock(&ui_mutex);
        pending_gpio_event = gpio_number;  // 直接使用 GPIO 编号
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
    // v2.0 GPIO Hub: 使用单一客户端订阅多个 GPIO
    ai_gpio_hub_client_create(&gpio_hub_client);
    
    if (ai_gpio_hub_client_connect(&gpio_hub_client) == 0) {
        int gpios[] = {GPIO_PAGE, GPIO_CONFIRM};  // GPIO 0 和 75
        if (ai_gpio_hub_client_subscribe_gpios(&gpio_hub_client, gpios, 2, 
                                                gpio_hub_callback, NULL) == 0) {
            printf("[Launcher] GPIO Hub connected (GPIO %d, %d)\n", GPIO_PAGE, GPIO_CONFIRM);
        } else {
            printf("[Launcher] Failed to subscribe to GPIOs\n");
        }
    } else {
        printf("[Launcher] Failed to connect GPIO Hub\n");
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
    ai_gpio_hub_client_destroy(&gpio_hub_client);
    if (ble_client) {
        ai_ble_client_destroy(ble_client);
        ble_client = NULL;
    }
    
    ai_display_disconnect(disp_client);
    pthread_mutex_destroy(&ui_mutex);
    printf("[Launcher] Cleanup complete\n");
}

// 打印帮助信息
void print_usage(const char *prog_name) {
    printf("Usage: %s [OPTIONS]\n", prog_name);
    printf("Options:\n");
    printf("  -h, --help                 Show this help message\n");
    printf("  --powersave_timeout <sec>  Set screen power save timeout in seconds (0 to disable)\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s --powersave_timeout 60  (Set timeout to 60s)\n", prog_name);
    printf("  %s --powersave_timeout 0   (Disable power save)\n", prog_name);
}

// ==================== 主函数 ====================
int main(int argc, char **argv) {
    setbuf(stdout, NULL); // 禁用 stdout 缓冲，确保立即输出
    
    // 解析命令行参数
    int power_save_timeout = -1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--powersave_timeout") == 0 && i + 1 < argc) {
            power_save_timeout = atoi(argv[i+1]);
            i++; // Skip value
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    printf("Launcher App Starting...\n");

    if (init_display_client() != 0) return -1;
    
    // 如果指定了超时参数，则设置
    if (power_save_timeout >= 0) {
        ai_display_set_power_save_timeout(disp_client, power_save_timeout);
        printf("[Launcher] Configured power save timeout: %d seconds\n", power_save_timeout);
    }

    init_gpio_clients();
    if (init_lvgl() != 0) return -1;

    ui_init();
    init_ble_text_overlay();
    switch_to_state(STATE_HOME);
    if (init_ble_client() != 0) {
        printf("[Launcher][BLE] WARNING: BLE text subscription disabled\n");
    }

    printf("[Launcher] Entering main loop\n");
    
    while(1) {
        if (ble_text_pending) {
            char text[AI_BLE_MAX_DATA_LEN + 1];

            pthread_mutex_lock(&ui_mutex);
            strncpy(text, pending_ble_text, AI_BLE_MAX_DATA_LEN + 1);
            ble_text_pending = 0;
            pthread_mutex_unlock(&ui_mutex);

            show_ble_text_overlay(text);
        }

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
