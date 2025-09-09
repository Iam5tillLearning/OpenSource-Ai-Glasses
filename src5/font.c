#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <font.h>
#include "lv_font_montserrat_48.c"

#define Font lv_font_montserrat_48

int x_clr, y_clr = 0;
int x_write, y_write = 0;

// 定义字符区域结构体
typedef struct {
    int top;        // 上边界
    int bottom;     // 下边界
    int left;       // 左边界
    int right;      // 右边界
} area;

// 字符区域指针
size_t size = 0;
area* areas = NULL;

// 向数组中添加新的字符区域
int add_area(area new_area) {
    // 扩展数组大小
    area* new_array = realloc(areas, (size + 1) * sizeof(area));
    if (new_array == NULL) {
        return -1; // 内存分配失败
    }
    areas = new_array; // 更新指针

    // 添加新区域
    areas[size] = new_area;
    size++;

    return 0;
}

// 查找并删除满足条件的区域
int find_remove_area(int x, int y) {
    for (size_t i = 0; i < size; ++i) {
        if (areas[i].top < x && x < areas[i].bottom && 
            areas[i].left < y && y < areas[i].right) {
            
            // 计算缓冲区大小（确保对齐）
            int width = areas[i].right - areas[i].left;
            int buf_size = (width + 1) / 2;  // 4bpp 打包格式
            uint8_t *pBuf = malloc(buf_size);
            if (!pBuf) return -1;
            memset(pBuf, 0, buf_size);

            // 逐行清除（可调整 sync 策略）
            int height = areas[i].bottom - areas[i].top;
            for (int j = 0; j < height; j++) {
                bool sync = (j == height - 1);  // 或直接 true 强制每行同步
                display_image_sync(areas[i].top + j, areas[i].left, pBuf, buf_size, sync);
            }
            free(pBuf);

            // 安全删除区域
            area *old_areas = areas;  // 备份指针
            for (size_t j = i; j < size - 1; ++j) {
                areas[j] = areas[j + 1];
            }
            size--;

            // 安全 realloc
            area *tmp = realloc(old_areas, size * sizeof(area));
            if (!tmp && size > 0) {
                free(old_areas);  // 避免内存泄漏
                return -1;
            }
            areas = tmp;

            return 0;  // 成功删除
        }
    }
    return -1;  // 未找到
}

// 清除一个字符区域
int clr_char(void) {
    for (int i = 0; i < 640 * 480; i++) {
        if (find_remove_area(x_clr, y_clr) == 0) {
            return 0;
        }
        else {
            y_clr += 1;
            if (y_clr >= 640) {
                x_clr += 1;
                y_clr = 0;
                if (x_clr >= 480) {
                    x_clr = 0;
                }
            }
        }
    }

    return -1;
}

// 打印 area 数组中的所有值
void print_areas() {
    for (size_t i = 0; i < size; i++) {
        printf("Area %zu: top = %d, bottom = %d, left = %d, right = %d\n",
            i, areas[i].top, areas[i].bottom, areas[i].left, areas[i].right);
    }
}

// 将UTF-8编码转为Unicode码
uint32_t utf8_to_unicode(const char** str) {
    uint32_t unichar = 0;
    if (**str & 0x80) {
        // 多字节字符
        if (**str & 0x40) {
            // 3字节字符
            unichar = (**str & 0x0F) << 12;
            (*str)++;
            unichar |= (**str & 0x3F) << 6;
            (*str)++;
            unichar |= (**str & 0x3F);
            (*str)++;
        }
        else {
            // 2字节字符
            unichar = (**str & 0x1F) << 6;
            (*str)++;
            unichar |= (**str & 0x3F);
            (*str)++;
        }
    }
    else {
        // 单字节字符
        unichar = **str;
        (*str)++;
    }
    return unichar;
}

// 检查unicode是否在字符集中
bool check_chars(uint32_t unicode, const char* chars) {
    const char* p = chars;
    while (*p) {
        if (*p == unicode) {
            return true;
        }
        p++;
    }
    return false;
}

// 将字符的位图数据写入屏幕
/**
 * @brief 在指定位置写入一个字符（优化版）
 * @param x 起始X坐标
 * @param y 起始Y坐标
 * @param font 使用的字体
 * @param c 要显示的Unicode字符
 * @param adv_x 字符的水平步进值
 * @param adv_y 字符的垂直步进值
 * @param last_char 是否是字符串中的最后一个字符
 * @return 成功返回0，失败返回-1
 */
int write_char(int x, int y, const lv_font_t* font, uint32_t c, int adv_x, int adv_y, bool last_char) {
    lv_font_glyph_dsc_t g;
    const uint8_t* bmp = lv_font_get_glyph_bitmap(font, c);

    // 特殊字符垂直位置调整
    const char* chars1 = "abcdefhiklmnorstuvwxz,。.．_";
    const char* chars2 = "gjpqy";
    const char* chars3 = "^\"'~";

    if (lv_font_get_glyph_dsc(font, &g, c, '\0')) {
        if (bmp == NULL) {
            fprintf(stderr, "Null Font!\n");
            return -1;
        }

        int width = g.box_w;
        int height = g.box_h;

        // 计算字符绘制起始位置
        int row = x + ((adv_y - height) / 2);
        int col = y + ((adv_x - width) / 2);

        // 特殊字符垂直位置调整
        if (check_chars(c, chars1)) {
            row = x + adv_y * 0.85 - height;
        }
        if (check_chars(c, chars2)) {
            row = x + adv_y - height;
        }
        if (check_chars(c, chars3)) {
            row = x + adv_y * 0.15;
        }

        // 预分配缓冲区
        uint8_t pBuf[(width + 1) / 2 + 1];  // 足够容纳所有情况
        memset(pBuf, 0, sizeof(pBuf));

        if (width % 2 == 0) {    // 宽度为偶数
            for (int row_add = 0; row_add < height; row_add++) {
                memcpy(pBuf, bmp + row_add * width / 2, width / 2);
                display_image_sync(row + row_add, col, pBuf, width / 2, 
                            (row_add == height-1) && last_char);
            }
        } else {                 // 宽度为奇数
            for (int row_add = 0, j = 0; row_add < height; row_add += 2, j += width) {
                // 处理奇数行
                memcpy(pBuf, bmp + j, (width + 1) / 2);
                pBuf[(width - 1) / 2] &= 0xF0;
                display_image_sync(row + row_add, col, pBuf, (width + 1) / 2, false);

                // 处理偶数行（如果有）
                if (row_add + 1 < height) {
                    uint8_t last_pixel = bmp[j + ((width - 1) / 2)] & 0x0F;
                    memset(pBuf, 0, sizeof(pBuf));
                    
                    // 复制剩余数据
                    if ((width - 1) / 2 > 0) {
                        memcpy(pBuf + 1, bmp + j + ((width + 1) / 2), (width - 1) / 2);
                    }
                    
                    // 移位处理
                    for (int k = 0; k < (width - 1) / 2; ++k) {
                        pBuf[k + 1] = (pBuf[k + 1] << 4) & 0xF0;
                        pBuf[k] |= pBuf[k + 1] >> 4;
                    }
                    pBuf[0] |= (last_pixel << 4);
                    
                    // 只在最后一行和最后一个字符时同步
                    bool sync = (row_add + 1 == height - 1) && last_char;
                    display_image_sync(row + row_add + 1, col, pBuf, (width + 1) / 2, sync);
                }
            }
        }
    }
    return 0;
}
/*//旧版本
int write_char(int x, int y, const lv_font_t* font, uint32_t c, int adv_x, int adv_y) {
    lv_font_glyph_dsc_t g;
    const uint8_t* bmp = lv_font_get_glyph_bitmap(font, c);     // 获取位图数据

    // 对特定字符优化
    const char* chars1 = "abcdefhiklmnorstuvwxz,，.。、_";
    const char* chars2 = "gjpqy";
    const char* chars3 = "^\"”'’~";

    if (lv_font_get_glyph_dsc(font, &g, c, '\0')) {
        if (bmp == NULL) {
            fprintf(stderr, "Null Font!\n");
            return -1;
        }
        // 获取字符的实际宽高
        int width = g.box_w;
        int height = g.box_h;

        // 计算行和列起始
        int row = x + ((adv_y - height) / 2);
        int col = y + ((adv_x - width) / 2);

        // 对特定字符优化
        if (check_chars(c, chars1)) {
            row = x + adv_y * 0.85 - height;
        }
        if (check_chars(c, chars2)) {
            row = x + adv_y - height;
        }
        if (check_chars(c, chars3)) {
            row = x + adv_y * 0.15;
        }

        if (width % 2 == 0) {    // 宽度为偶数个像素
            uint8_t pBuf[width / 2];
            for (int row_add = 0; row_add < height; row_add++) {
                for (int i = 0; i < width / 2; i++) {
                    pBuf[i] = bmp[row_add * width / 2 + i];
                }
                display_image(row + row_add, col, pBuf, width / 2);
            }
        }
        else {                   // 宽度为奇数个像素
            uint8_t pBuf[(width + 1) / 2];
            for (int row_add = 0, j = 0; row_add < height; row_add += 2, j += width) {
                for (int i = 0; i < (width + 1) / 2; i++) {
                    pBuf[i] = bmp[j + i];
                }
                pBuf[(width - 1) / 2] &= 0xF0;                              // 末像素清零
                display_image(row + row_add, col, pBuf, (width + 1) / 2);

                //写下一行像素
                if (row_add + 1 < height) {
                    uint8_t new_Buf[((width + 1) / 2)+1];
                    uint8_t last_pixel = bmp[j + ((width - 1) / 2)] & 0x0F; // 记录行末像素
                    memset(pBuf, 0, sizeof(pBuf));
                    memset(new_Buf, 0, sizeof(new_Buf));
                    for (int i = 0; i < (width - 1) / 2; i++) {
                        pBuf[i] = bmp[j + ((width + 1) / 2) + i];
                    }
                    // 将数组元素右移4位
                    for (int k = 0; k < (width - 1) / 2; ++k) {
                        new_Buf[k + 1] = (pBuf[k] << 4) & 0xF0; // 当前高4位移到下个低4位
                        new_Buf[k] |= pBuf[k] >> 4;             // 当前低4位与前个高4位合并
                    }
                    new_Buf[0] |= (last_pixel << 4);            // 把行末像素移到高4位
                    display_image(row + row_add+1, col, new_Buf, (width + 1) / 2);
                }
            }
        }
    }

    return 0;
}*/

// 显示字符串
/*int display_string(const char* text) {
    const lv_font_t* font = &Font;   // 选择字体
    const char* p = text;
    int adv_y = lv_font_get_line_height(font);                // 获取字符的建议高度
    const char* chars1 = "j";
    // 使行数为整数
    while (480 % adv_y != 0) {
        adv_y--;
    }

    while (*p) {
        uint32_t unicode = utf8_to_unicode(&p);
        int adv_x = lv_font_get_glyph_width(font, unicode, 0);// 获取字符的建议宽度

        // 对特定字符优化
        if (check_chars(unicode, chars1)) {
            adv_x *= 1.5;
        }

        // 清除字符区域
        for (int n = 0; n < adv_x + adv_y; n++) {
            find_remove_area(x_write + (adv_y / 2), y_write + n);
        }

        // 添加字符区域
        area new_area1 = { x_write, x_write + adv_y, y_write, y_write + adv_x };
        if (add_area(new_area1) != 0) {
            perror("Failed to add area");
            return -1;
        }

        // 自动换行
        if (y_write + adv_x >= 640) {
            x_write += adv_y;
            y_write = 0;
            if (x_write + adv_y >= 480) {
                x_write = 0;
            }
        }

        if (write_char(x_write, y_write, font, unicode, adv_x, adv_y) == 0) {
            y_write += adv_x;
            // 自动换行
            if (y_write + adv_x >= 640) {
                x_write += adv_y;
                y_write = 0;
                if (x_write + adv_y >= 480) {
                    x_write = 0;
                }
            }
        }
    }

    return 0;
}*/

//新增指定位置显示
int display_string_at(int x, int y, const char* text) {
    const lv_font_t* font = &Font;
    const char* p = text;
    int adv_y = lv_font_get_line_height(font);

    // 使 adv_y 能整除 480
    while (480 % adv_y != 0) {
        adv_y--;
    }

    int cur_x = x;
    int cur_y = y;
    int char_count = strlen(text);
    int current_char = 0;

    while (*p) {
        current_char++;
        uint32_t unicode = utf8_to_unicode(&p);
        int adv_x = lv_font_get_glyph_width(font, unicode, 0);

        // 处理特殊字符宽度
        const char* chars1 = "j";
        if (check_chars(unicode, chars1)) {
            adv_x *= 1.5;
        }

        // 清除原区域 - 可以优化为批量清除
        for (int n = 0; n < adv_x + adv_y; n++) {
            find_remove_area(cur_x + (adv_y / 2), cur_y + n);
        }

        // 添加新区域
        area new_area1 = { cur_x, cur_x + adv_y, cur_y, cur_y + adv_x };
        if (add_area(new_area1) != 0) {
            perror("Failed to add area");
            return -1;
        }

        // 只在最后一个字符时同步
        bool is_last_char = (current_char == char_count);
        if (write_char(cur_x, cur_y, font, unicode, adv_x, adv_y, is_last_char) == 0) {
            cur_y += adv_x;
            // 自动换行
            if (cur_y + adv_x >= 640) {
                cur_x += adv_y;
                cur_y = y;
                if (cur_x + adv_y >= 480) {
                    cur_x = x;
                }
                // 换行时同步一次
                send_cmd(SPI_SYNC);
                usleep(1 * 1000);
            }
        }
    }

    return 0;
}
/*
int display_string_at(int x, int y, const char* text) {
    const lv_font_t* font = &Font;
    const char* p = text;
    int adv_y = lv_font_get_line_height(font);

    // 使 adv_y 能整除 480
    while (480 % adv_y != 0) {
        adv_y--;
    }

    int cur_x = x;
    int cur_y = y;

    while (*p) {
        uint32_t unicode = utf8_to_unicode(&p);
        int adv_x = lv_font_get_glyph_width(font, unicode, 0);

        // 处理特殊字符宽度
        const char* chars1 = "j";
        if (check_chars(unicode, chars1)) {
            adv_x *= 1.5;
        }

        // 清除原区域
        for (int n = 0; n < adv_x + adv_y; n++) {
            find_remove_area(cur_x + (adv_y / 2), cur_y + n);
        }

        // 添加新区域
        area new_area1 = { cur_x, cur_x + adv_y, cur_y, cur_y + adv_x };
        if (add_area(new_area1) != 0) {
            perror("Failed to add area");
            return -1;
        }

        // 自动换行
        if (cur_y + adv_x >= 640) {
            cur_x += adv_y;
            cur_y = y;
            if (cur_x + adv_y >= 480) {
                cur_x = x;
            }
        }

        if (write_char(cur_x, cur_y, font, unicode, adv_x, adv_y) == 0) {
            cur_y += adv_x;
            // 自动换行
            if (cur_y + adv_x >= 640) {
                cur_x += adv_y;
                cur_y = y;
                if (cur_x + adv_y >= 480) {
                    cur_x = x;
                }
            }
        }
    }

    return 0;
}*/

//系统时钟函数
uint32_t custom_tick_get(void) {
    static uint64_t start_ms = 0;
    if (start_ms == 0) {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = (tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000;
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t now_ms;
    now_ms = (tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000;

    uint32_t time_ms = now_ms - start_ms;
    return time_ms;
}