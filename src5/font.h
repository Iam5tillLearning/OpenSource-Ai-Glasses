#include "lvgl/lvgl.h"
#include <jbd013_api.h>
#include <hal_driver.h>

int clr_char(void);
//int display_string(const char* text);
int display_string_at(int x, int y, const char* text);
uint32_t custom_tick_get(void);