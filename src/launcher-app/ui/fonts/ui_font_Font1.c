/*******************************************************************************
 * Size: 16 px
 * Bpp: 1
 * Opts: --bpp 1 --size 16 --font /Users/wenmou/Desktop/7-26GUI/assets/AlibabaPuHuiTi-3-55-Regular.ttf -o /Users/wenmou/Desktop/7-26GUI/assets/ui_font_Font1.c --format lvgl -r 0x20-0x7f --symbols 月日年星期一二三四五六天 --no-compress --no-prefilter
 ******************************************************************************/

#include "../ui.h"

#ifndef UI_FONT_FONT1
#define UI_FONT_FONT1 1
#endif

#if UI_FONT_FONT1

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0x55, 0x55, 0x3c,

    /* U+0022 "\"" */
    0xdd, 0xdd, 0x50,

    /* U+0023 "#" */
    0x11, 0x8, 0x84, 0xdf, 0xf2, 0x21, 0x10, 0x89,
    0xff, 0x64, 0x22, 0x11, 0x0,

    /* U+0024 "$" */
    0x10, 0x21, 0xfc, 0x89, 0x12, 0x3c, 0x1e, 0x16,
    0x24, 0x48, 0x9f, 0xc2, 0x4, 0x8,

    /* U+0025 "%" */
    0x71, 0x91, 0x22, 0x2c, 0x45, 0x8, 0xe0, 0xeb,
    0x83, 0x88, 0x51, 0x1a, 0x22, 0x44, 0xc7, 0x0,

    /* U+0026 "&" */
    0x1c, 0x8, 0x82, 0x20, 0x98, 0x3c, 0xe, 0x24,
    0xca, 0x14, 0x83, 0x30, 0xc7, 0xdc,

    /* U+0027 "'" */
    0xff, 0x40,

    /* U+0028 "(" */
    0x26, 0x44, 0x88, 0x88, 0x88, 0x84, 0x46, 0x20,

    /* U+0029 ")" */
    0x46, 0x22, 0x11, 0x11, 0x11, 0x12, 0x26, 0x40,

    /* U+002A "*" */
    0x10, 0x47, 0xcc, 0x28, 0x20,

    /* U+002B "+" */
    0x10, 0x10, 0x10, 0x10, 0xff, 0x10, 0x10, 0x10,

    /* U+002C "," */
    0x24, 0xa4,

    /* U+002D "-" */
    0xf8,

    /* U+002E "." */
    0xf0,

    /* U+002F "/" */
    0x4, 0x20, 0x86, 0x10, 0x43, 0x8, 0x21, 0x84,
    0x10, 0xc0,

    /* U+0030 "0" */
    0x38, 0x8a, 0xc, 0x18, 0x30, 0x60, 0xc1, 0x82,
    0x88, 0xe0,

    /* U+0031 "1" */
    0x7d, 0x11, 0x11, 0x11, 0x11, 0x10,

    /* U+0032 "2" */
    0x7c, 0xc, 0x8, 0x10, 0x20, 0x83, 0xc, 0x30,
    0xc3, 0xf8,

    /* U+0033 "3" */
    0x7c, 0x4, 0x8, 0x33, 0x80, 0xc0, 0x81, 0x2,
    0xb, 0xe0,

    /* U+0034 "4" */
    0xc, 0xc, 0x14, 0x34, 0x24, 0x44, 0xc4, 0xff,
    0x4, 0x4, 0x4,

    /* U+0035 "5" */
    0x7c, 0x81, 0x2, 0x7, 0xc0, 0xc0, 0x81, 0x2,
    0xb, 0xe0,

    /* U+0036 "6" */
    0x1c, 0x61, 0x4, 0xb, 0xd8, 0xe0, 0xc1, 0x82,
    0x8d, 0xf0,

    /* U+0037 "7" */
    0xfe, 0x4, 0x10, 0x20, 0xc1, 0x6, 0x8, 0x30,
    0x41, 0x80,

    /* U+0038 "8" */
    0x3c, 0x85, 0xa, 0x13, 0xc8, 0xe0, 0xc1, 0x83,
    0x8d, 0xf0,

    /* U+0039 "9" */
    0x38, 0x8a, 0xc, 0x18, 0x38, 0xde, 0x81, 0x4,
    0x31, 0xc0,

    /* U+003A ":" */
    0xf0, 0x3, 0xc0,

    /* U+003B ";" */
    0x6c, 0x0, 0x1, 0x2b, 0x0,

    /* U+003C "<" */
    0x3, 0xe, 0x78, 0xc0, 0xc0, 0x70, 0x1e, 0x3,

    /* U+003D "=" */
    0xff, 0x0, 0x0, 0x0, 0xff,

    /* U+003E ">" */
    0x80, 0xf0, 0x1c, 0x7, 0x3, 0x1c, 0x70, 0xc0,

    /* U+003F "?" */
    0xf8, 0x10, 0x41, 0xc, 0x63, 0xc, 0x0, 0xc3,
    0x0,

    /* U+0040 "@" */
    0xf, 0x83, 0x6, 0x60, 0x24, 0x79, 0x89, 0x99,
    0x19, 0x91, 0x99, 0x11, 0x93, 0xa8, 0xee, 0x40,
    0x2, 0x0, 0x1f, 0xc0,

    /* U+0041 "A" */
    0xc, 0x1, 0x40, 0x68, 0x9, 0x81, 0x10, 0x62,
    0xf, 0xe3, 0x4, 0x40, 0xc8, 0xb, 0x1, 0x0,

    /* U+0042 "B" */
    0xfd, 0xe, 0xc, 0x18, 0x7f, 0xa1, 0xc1, 0x83,
    0xf, 0xf0,

    /* U+0043 "C" */
    0x3f, 0x60, 0xc0, 0x80, 0x80, 0x80, 0x80, 0x80,
    0xc0, 0x40, 0x3f,

    /* U+0044 "D" */
    0xfc, 0x86, 0x83, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x82, 0x86, 0xfc,

    /* U+0045 "E" */
    0xfe, 0x8, 0x20, 0x83, 0xe8, 0x20, 0x82, 0xf,
    0xc0,

    /* U+0046 "F" */
    0xfe, 0x8, 0x20, 0x83, 0xe8, 0x20, 0x82, 0x8,
    0x0,

    /* U+0047 "G" */
    0x3f, 0x60, 0xc0, 0x80, 0x80, 0x87, 0x81, 0x81,
    0xc1, 0x61, 0x3f,

    /* U+0048 "H" */
    0x81, 0x81, 0x81, 0x81, 0xff, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x81,

    /* U+0049 "I" */
    0xff, 0xe0,

    /* U+004A "J" */
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1e,

    /* U+004B "K" */
    0x86, 0x8c, 0x88, 0x98, 0xb0, 0xe0, 0xb0, 0x98,
    0x88, 0x84, 0x86,

    /* U+004C "L" */
    0x82, 0x8, 0x20, 0x82, 0x8, 0x20, 0x82, 0xf,
    0xc0,

    /* U+004D "M" */
    0xc0, 0xf8, 0x7e, 0x16, 0x85, 0xb3, 0x64, 0x99,
    0xe6, 0x31, 0x8c, 0x60, 0x18, 0x4,

    /* U+004E "N" */
    0xc0, 0xf0, 0x6c, 0x36, 0x19, 0x8c, 0x66, 0x33,
    0xd, 0x83, 0xc0, 0xe0, 0x60,

    /* U+004F "O" */
    0x3e, 0x20, 0xa0, 0x30, 0x18, 0xc, 0x6, 0x3,
    0x1, 0x80, 0xa0, 0x8f, 0x80,

    /* U+0050 "P" */
    0xfd, 0xe, 0xc, 0x18, 0x30, 0xff, 0x40, 0x81,
    0x2, 0x0,

    /* U+0051 "Q" */
    0x3e, 0x20, 0xa0, 0x30, 0x18, 0xc, 0x6, 0x3,
    0x1, 0x80, 0xa0, 0x8f, 0x80, 0xc0, 0x30,

    /* U+0052 "R" */
    0xfc, 0x86, 0x82, 0x82, 0x86, 0xfc, 0x98, 0x8c,
    0x86, 0x82, 0x83,

    /* U+0053 "S" */
    0x7f, 0x2, 0x4, 0xe, 0x7, 0x81, 0x81, 0x2,
    0xf, 0xf0,

    /* U+0054 "T" */
    0xff, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x10,

    /* U+0055 "U" */
    0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x81, 0xc3, 0x7c,

    /* U+0056 "V" */
    0xc0, 0x90, 0x26, 0x19, 0x84, 0x21, 0xc, 0xc3,
    0x20, 0x48, 0x1e, 0x7, 0x0, 0xc0,

    /* U+0057 "W" */
    0x40, 0xd, 0xc, 0x34, 0x30, 0x98, 0xe2, 0x66,
    0x98, 0x92, 0x62, 0x4d, 0xd, 0x14, 0x3c, 0x50,
    0x61, 0xc1, 0x86, 0x0,

    /* U+0058 "X" */
    0x60, 0x88, 0x63, 0x30, 0x48, 0x1e, 0x3, 0x1,
    0xe0, 0x48, 0x33, 0x18, 0x64, 0x8,

    /* U+0059 "Y" */
    0xc1, 0xb0, 0x88, 0x86, 0xc1, 0x40, 0x60, 0x20,
    0x10, 0x8, 0x4, 0x2, 0x0,

    /* U+005A "Z" */
    0xff, 0x3, 0x6, 0xc, 0x8, 0x18, 0x30, 0x20,
    0x40, 0xc0, 0xff,

    /* U+005B "[" */
    0xf2, 0x49, 0x24, 0x92, 0x49, 0x38,

    /* U+005C "\\" */
    0x40, 0x40, 0x60, 0x20, 0x30, 0x10, 0x18, 0x8,
    0x8, 0x4, 0x4, 0x6, 0x2, 0x3,

    /* U+005D "]" */
    0xe4, 0x92, 0x49, 0x24, 0x92, 0x78,

    /* U+005E "^" */
    0x38, 0x51, 0xa2, 0x64, 0x58, 0xc0,

    /* U+005F "_" */
    0xff,

    /* U+0060 "`" */
    0xc9, 0x80,

    /* U+0061 "a" */
    0x3c, 0xc, 0x8, 0x17, 0xf0, 0x60, 0xc3, 0x7a,

    /* U+0062 "b" */
    0x80, 0x80, 0x80, 0xbc, 0xc2, 0x81, 0x81, 0x81,
    0x81, 0x81, 0xc2, 0xbc,

    /* U+0063 "c" */
    0x3e, 0x82, 0x4, 0x8, 0x10, 0x20, 0x20, 0x3e,

    /* U+0064 "d" */
    0x1, 0x1, 0x1, 0x3d, 0x43, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x43, 0x3d,

    /* U+0065 "e" */
    0x3c, 0x8e, 0xc, 0x1f, 0xf0, 0x20, 0x21, 0x3e,

    /* U+0066 "f" */
    0x1c, 0x82, 0x3e, 0x20, 0x82, 0x8, 0x20, 0x82,
    0x8,

    /* U+0067 "g" */
    0x3d, 0x43, 0xc1, 0x81, 0x81, 0x81, 0xc1, 0x43,
    0x3d, 0x1, 0x2, 0x7c,

    /* U+0068 "h" */
    0x81, 0x2, 0x5, 0xec, 0x70, 0x60, 0xc1, 0x83,
    0x6, 0xc, 0x10,

    /* U+0069 "i" */
    0x9f, 0xf0,

    /* U+006A "j" */
    0x10, 0x1, 0x11, 0x11, 0x11, 0x11, 0x11, 0xe0,

    /* U+006B "k" */
    0x82, 0x8, 0x23, 0x9a, 0x4a, 0x38, 0xa2, 0x49,
    0xa3,

    /* U+006C "l" */
    0xaa, 0xaa, 0xab,

    /* U+006D "m" */
    0xfb, 0xd8, 0xc6, 0x10, 0xc2, 0x18, 0x43, 0x8,
    0x61, 0xc, 0x21, 0x84, 0x20,

    /* U+006E "n" */
    0xbd, 0x86, 0xc, 0x18, 0x30, 0x60, 0xc1, 0x82,

    /* U+006F "o" */
    0x3c, 0x42, 0x81, 0x81, 0x81, 0x81, 0x81, 0x42,
    0x3c,

    /* U+0070 "p" */
    0xbc, 0xc2, 0x81, 0x81, 0x81, 0x81, 0x81, 0xc2,
    0xbc, 0x80, 0x80, 0x80,

    /* U+0071 "q" */
    0x3d, 0x43, 0x81, 0x81, 0x81, 0x81, 0x81, 0x43,
    0x3d, 0x1, 0x1, 0x1,

    /* U+0072 "r" */
    0xbc, 0x88, 0x88, 0x88, 0x80,

    /* U+0073 "s" */
    0x7c, 0x21, 0x87, 0x4, 0x21, 0xf0,

    /* U+0074 "t" */
    0x42, 0x3e, 0x84, 0x21, 0x8, 0x42, 0xe,

    /* U+0075 "u" */
    0x83, 0x6, 0xc, 0x18, 0x30, 0x60, 0xe3, 0x7a,

    /* U+0076 "v" */
    0xc1, 0x43, 0x42, 0x62, 0x26, 0x24, 0x34, 0x1c,
    0x18,

    /* U+0077 "w" */
    0xc3, 0x1a, 0x38, 0xd1, 0x44, 0xca, 0x22, 0xdb,
    0x14, 0x50, 0xa2, 0x87, 0x1c, 0x18, 0x60,

    /* U+0078 "x" */
    0x43, 0x66, 0x24, 0x1c, 0x18, 0x3c, 0x24, 0x62,
    0x43,

    /* U+0079 "y" */
    0xc1, 0x43, 0x42, 0x62, 0x26, 0x34, 0x14, 0x18,
    0x18, 0x18, 0x10, 0x60,

    /* U+007A "z" */
    0xfc, 0x18, 0x60, 0x83, 0x4, 0x10, 0x60, 0xfe,

    /* U+007B "{" */
    0x29, 0x24, 0x94, 0x49, 0x24, 0x88,

    /* U+007C "|" */
    0xff, 0xfe,

    /* U+007D "}" */
    0x84, 0x44, 0x44, 0x43, 0x44, 0x44, 0x44, 0x80,

    /* U+007E "~" */
    0x71, 0x49, 0xc6,

    /* U+4E00 "一" */
    0xff, 0xfc,

    /* U+4E09 "三" */
    0x7f, 0xf8, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0xff, 0xc0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0xf, 0xff, 0xc0,

    /* U+4E8C "二" */
    0x7f, 0xf8, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0xf, 0xff, 0xc0,

    /* U+4E94 "五" */
    0x7f, 0xf8, 0x10, 0x0, 0x40, 0x3, 0x0, 0xc,
    0x1, 0xff, 0x80, 0x82, 0x2, 0x8, 0x8, 0x20,
    0x60, 0x81, 0x82, 0x3f, 0xff,

    /* U+516D "六" */
    0x2, 0x0, 0xc, 0x0, 0x10, 0x3f, 0xff, 0x0,
    0x0, 0x0, 0x0, 0xcc, 0x2, 0x18, 0x18, 0x20,
    0xc0, 0xc2, 0x1, 0x98, 0x2, 0xc0, 0xc, 0x0,
    0x0,

    /* U+56DB "四" */
    0xff, 0xf8, 0x91, 0x89, 0x18, 0x91, 0x89, 0x19,
    0x91, 0xb1, 0xfa, 0x1, 0x80, 0x18, 0x1, 0xff,
    0xf8, 0x1, 0x80, 0x10,

    /* U+5929 "天" */
    0x3f, 0xf8, 0x2, 0x0, 0x4, 0x0, 0x8, 0x0,
    0x10, 0xf, 0xff, 0x80, 0x40, 0x1, 0xc0, 0x2,
    0x80, 0xc, 0x80, 0x31, 0x81, 0xc0, 0xc6, 0x0,
    0xc0, 0x0, 0x0,

    /* U+5E74 "年" */
    0x18, 0x0, 0x40, 0x3, 0xff, 0x88, 0x40, 0x41,
    0x3, 0x4, 0x3, 0xff, 0x88, 0x40, 0x21, 0x0,
    0x84, 0xf, 0xff, 0xc0, 0x40, 0x1, 0x0, 0x4,
    0x0, 0x10, 0x0,

    /* U+65E5 "日" */
    0xff, 0xe0, 0x18, 0x6, 0x1, 0x80, 0x60, 0x1f,
    0xfe, 0x1, 0x80, 0x60, 0x18, 0x7, 0xff, 0x80,
    0x60, 0x10,

    /* U+661F "星" */
    0x3f, 0xf0, 0x80, 0x43, 0xff, 0x8, 0x4, 0x3f,
    0xf0, 0x0, 0x3, 0x10, 0x1f, 0xfe, 0xc1, 0x0,
    0xff, 0xe0, 0x10, 0x0, 0x40, 0xff, 0xfc,

    /* U+6708 "月" */
    0x3f, 0xe4, 0x4, 0x80, 0x90, 0x13, 0xfe, 0x40,
    0x48, 0x9, 0x1, 0x3f, 0xe4, 0x5, 0x80, 0xa0,
    0x1c, 0xf, 0x1, 0x80,

    /* U+671F "期" */
    0x22, 0x7b, 0xfa, 0x48, 0x92, 0x44, 0x93, 0xe7,
    0x91, 0x24, 0xf9, 0x24, 0x49, 0x22, 0x7f, 0xfc,
    0x41, 0x22, 0x4d, 0x14, 0x38, 0xa0, 0x88
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 66, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 92, .box_w = 2, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 4, .adv_w = 104, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 7, .adv_w = 154, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 20, .adv_w = 147, .box_w = 7, .box_h = 16, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 34, .adv_w = 210, .box_w = 11, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 50, .adv_w = 164, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 64, .adv_w = 65, .box_w = 2, .box_h = 5, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 66, .adv_w = 92, .box_w = 4, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 74, .adv_w = 92, .box_w = 4, .box_h = 15, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 82, .adv_w = 110, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 87, .adv_w = 141, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 95, .adv_w = 77, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 97, .adv_w = 113, .box_w = 5, .box_h = 1, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 98, .adv_w = 77, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 99, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 109, .adv_w = 147, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 119, .adv_w = 147, .box_w = 4, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 125, .adv_w = 147, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 135, .adv_w = 147, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 145, .adv_w = 147, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 156, .adv_w = 147, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 166, .adv_w = 147, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 176, .adv_w = 147, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 186, .adv_w = 147, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 196, .adv_w = 147, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 206, .adv_w = 97, .box_w = 2, .box_h = 9, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 209, .adv_w = 97, .box_w = 3, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 214, .adv_w = 141, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 222, .adv_w = 141, .box_w = 8, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 227, .adv_w = 141, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 235, .adv_w = 110, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 244, .adv_w = 220, .box_w = 12, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 264, .adv_w = 171, .box_w = 11, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 280, .adv_w = 159, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 290, .adv_w = 153, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 301, .adv_w = 179, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 312, .adv_w = 142, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 321, .adv_w = 135, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 330, .adv_w = 175, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 341, .adv_w = 181, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 352, .adv_w = 70, .box_w = 1, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 354, .adv_w = 69, .box_w = 4, .box_h = 14, .ofs_x = -2, .ofs_y = -3},
    {.bitmap_index = 361, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 372, .adv_w = 130, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 381, .adv_w = 212, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 395, .adv_w = 189, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 408, .adv_w = 190, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 421, .adv_w = 153, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 431, .adv_w = 190, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 446, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 457, .adv_w = 145, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 467, .adv_w = 135, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 478, .adv_w = 179, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 489, .adv_w = 158, .box_w = 10, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 503, .adv_w = 230, .box_w = 14, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 523, .adv_w = 160, .box_w = 10, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 537, .adv_w = 152, .box_w = 9, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 550, .adv_w = 160, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 561, .adv_w = 75, .box_w = 3, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 567, .adv_w = 131, .box_w = 8, .box_h = 14, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 581, .adv_w = 75, .box_w = 3, .box_h = 15, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 587, .adv_w = 141, .box_w = 7, .box_h = 6, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 593, .adv_w = 128, .box_w = 8, .box_h = 1, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 594, .adv_w = 89, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 10},
    {.bitmap_index = 596, .adv_w = 157, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 604, .adv_w = 163, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 616, .adv_w = 127, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 624, .adv_w = 163, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 636, .adv_w = 148, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 644, .adv_w = 89, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 653, .adv_w = 163, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 665, .adv_w = 158, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 676, .adv_w = 69, .box_w = 1, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 678, .adv_w = 69, .box_w = 4, .box_h = 15, .ofs_x = -2, .ofs_y = -3},
    {.bitmap_index = 686, .adv_w = 133, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 695, .adv_w = 73, .box_w = 2, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 698, .adv_w = 236, .box_w = 11, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 711, .adv_w = 158, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 719, .adv_w = 158, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 728, .adv_w = 163, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 740, .adv_w = 163, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 752, .adv_w = 97, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 757, .adv_w = 123, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 763, .adv_w = 86, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 770, .adv_w = 157, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 778, .adv_w = 135, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 787, .adv_w = 212, .box_w = 13, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 802, .adv_w = 132, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 811, .adv_w = 135, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 823, .adv_w = 130, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 831, .adv_w = 77, .box_w = 3, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 837, .adv_w = 47, .box_w = 1, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 839, .adv_w = 77, .box_w = 4, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 847, .adv_w = 141, .box_w = 8, .box_h = 3, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 850, .adv_w = 252, .box_w = 14, .box_h = 1, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 852, .adv_w = 252, .box_w = 14, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 872, .adv_w = 252, .box_w = 14, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 892, .adv_w = 252, .box_w = 14, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 913, .adv_w = 252, .box_w = 14, .box_h = 14, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 938, .adv_w = 252, .box_w = 12, .box_h = 13, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 958, .adv_w = 252, .box_w = 15, .box_h = 14, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 985, .adv_w = 252, .box_w = 14, .box_h = 15, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1012, .adv_w = 252, .box_w = 10, .box_h = 14, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 1030, .adv_w = 252, .box_w = 14, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1053, .adv_w = 252, .box_w = 11, .box_h = 14, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 1073, .adv_w = 252, .box_w = 13, .box_h = 14, .ofs_x = 1, .ofs_y = -2}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_1[] = {
    0x0, 0x9, 0x8c, 0x94, 0x36d, 0x8db, 0xb29, 0x1074,
    0x17e5, 0x181f, 0x1908, 0x191f
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 19968, .range_length = 6432, .glyph_id_start = 96,
        .unicode_list = unicode_list_1, .glyph_id_ofs_list = NULL, .list_length = 12, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};

/*-----------------
 *    KERNING
 *----------------*/


/*Pair left and right glyphs for kerning*/
static const uint8_t kern_pair_glyph_ids[] =
{
    3, 3,
    3, 8,
    3, 13,
    3, 15,
    8, 3,
    8, 8,
    8, 13,
    8, 15,
    9, 75,
    13, 3,
    13, 8,
    13, 18,
    13, 24,
    13, 26,
    15, 3,
    15, 8,
    15, 18,
    15, 24,
    15, 26,
    16, 16,
    18, 13,
    18, 15,
    18, 27,
    18, 28,
    24, 13,
    24, 15,
    24, 27,
    24, 28,
    27, 18,
    27, 24,
    28, 18,
    28, 24,
    34, 3,
    34, 8,
    34, 36,
    34, 40,
    34, 48,
    34, 50,
    34, 53,
    34, 54,
    34, 55,
    34, 56,
    34, 58,
    34, 71,
    34, 77,
    34, 85,
    34, 87,
    34, 90,
    35, 36,
    35, 40,
    35, 48,
    35, 50,
    35, 53,
    35, 55,
    35, 57,
    35, 58,
    37, 13,
    37, 15,
    37, 34,
    37, 53,
    37, 55,
    37, 56,
    37, 57,
    37, 58,
    37, 59,
    37, 66,
    38, 75,
    39, 13,
    39, 15,
    39, 34,
    39, 66,
    39, 73,
    39, 76,
    40, 55,
    40, 58,
    43, 43,
    44, 34,
    44, 36,
    44, 40,
    44, 48,
    44, 50,
    44, 53,
    44, 54,
    44, 55,
    44, 56,
    44, 58,
    44, 68,
    44, 69,
    44, 70,
    44, 72,
    44, 77,
    44, 80,
    44, 82,
    44, 84,
    44, 85,
    44, 86,
    44, 87,
    44, 88,
    44, 90,
    45, 3,
    45, 8,
    45, 34,
    45, 36,
    45, 40,
    45, 48,
    45, 50,
    45, 53,
    45, 54,
    45, 55,
    45, 56,
    45, 58,
    45, 87,
    45, 88,
    45, 90,
    48, 13,
    48, 15,
    48, 34,
    48, 53,
    48, 55,
    48, 56,
    48, 57,
    48, 58,
    48, 59,
    48, 66,
    49, 13,
    49, 15,
    49, 34,
    49, 53,
    49, 55,
    49, 56,
    49, 57,
    49, 58,
    49, 59,
    49, 66,
    50, 13,
    50, 15,
    50, 34,
    50, 43,
    50, 53,
    50, 55,
    50, 56,
    50, 57,
    50, 58,
    50, 59,
    50, 66,
    51, 36,
    51, 40,
    51, 48,
    51, 50,
    51, 53,
    51, 54,
    51, 55,
    51, 56,
    51, 57,
    51, 58,
    51, 68,
    51, 69,
    51, 70,
    51, 72,
    51, 80,
    51, 82,
    51, 85,
    51, 86,
    51, 87,
    51, 89,
    51, 90,
    52, 13,
    52, 15,
    52, 52,
    52, 53,
    52, 55,
    52, 56,
    52, 58,
    52, 85,
    52, 87,
    52, 88,
    52, 89,
    52, 90,
    53, 3,
    53, 8,
    53, 13,
    53, 15,
    53, 34,
    53, 36,
    53, 40,
    53, 48,
    53, 50,
    53, 52,
    53, 66,
    53, 68,
    53, 69,
    53, 70,
    53, 72,
    53, 74,
    53, 75,
    53, 78,
    53, 79,
    53, 80,
    53, 81,
    53, 82,
    53, 83,
    53, 84,
    53, 85,
    53, 86,
    53, 87,
    53, 88,
    53, 89,
    53, 90,
    53, 91,
    54, 34,
    54, 57,
    54, 89,
    55, 13,
    55, 15,
    55, 27,
    55, 28,
    55, 34,
    55, 36,
    55, 40,
    55, 48,
    55, 50,
    55, 52,
    55, 66,
    55, 68,
    55, 69,
    55, 70,
    55, 72,
    55, 73,
    55, 76,
    55, 80,
    55, 82,
    55, 84,
    55, 87,
    55, 90,
    56, 13,
    56, 15,
    56, 27,
    56, 28,
    56, 34,
    56, 36,
    56, 40,
    56, 48,
    56, 50,
    56, 66,
    56, 68,
    56, 69,
    56, 70,
    56, 72,
    56, 80,
    56, 82,
    56, 84,
    57, 34,
    57, 36,
    57, 40,
    57, 48,
    57, 50,
    57, 53,
    57, 54,
    57, 55,
    57, 56,
    57, 58,
    57, 68,
    57, 69,
    57, 70,
    57, 72,
    57, 77,
    57, 80,
    57, 82,
    57, 84,
    57, 85,
    57, 86,
    57, 87,
    57, 88,
    57, 90,
    58, 3,
    58, 8,
    58, 13,
    58, 15,
    58, 27,
    58, 28,
    58, 34,
    58, 36,
    58, 40,
    58, 48,
    58, 50,
    58, 52,
    58, 66,
    58, 68,
    58, 69,
    58, 70,
    58, 71,
    58, 72,
    58, 73,
    58, 74,
    58, 76,
    58, 78,
    58, 79,
    58, 80,
    58, 81,
    58, 82,
    58, 83,
    58, 84,
    58, 85,
    58, 86,
    58, 87,
    58, 89,
    58, 90,
    58, 91,
    59, 36,
    59, 40,
    59, 48,
    59, 50,
    60, 75,
    66, 53,
    66, 55,
    66, 58,
    66, 87,
    66, 88,
    66, 90,
    67, 13,
    67, 15,
    67, 53,
    67, 55,
    67, 56,
    67, 57,
    67, 58,
    67, 87,
    67, 89,
    67, 90,
    70, 13,
    70, 15,
    70, 53,
    70, 58,
    70, 75,
    71, 3,
    71, 8,
    71, 10,
    71, 11,
    71, 13,
    71, 15,
    71, 32,
    71, 53,
    71, 55,
    71, 56,
    71, 57,
    71, 58,
    71, 62,
    71, 66,
    71, 68,
    71, 69,
    71, 70,
    71, 71,
    71, 72,
    71, 73,
    71, 74,
    71, 75,
    71, 76,
    71, 77,
    71, 80,
    71, 82,
    71, 84,
    71, 85,
    71, 94,
    73, 53,
    73, 58,
    73, 87,
    73, 90,
    74, 3,
    74, 8,
    74, 10,
    74, 32,
    74, 53,
    74, 62,
    74, 94,
    75, 53,
    76, 53,
    76, 54,
    76, 56,
    76, 66,
    76, 68,
    76, 69,
    76, 70,
    76, 72,
    76, 77,
    76, 80,
    76, 82,
    76, 86,
    77, 71,
    77, 87,
    77, 88,
    77, 90,
    78, 53,
    78, 58,
    78, 87,
    78, 90,
    79, 53,
    79, 58,
    79, 87,
    79, 90,
    80, 13,
    80, 15,
    80, 53,
    80, 55,
    80, 56,
    80, 57,
    80, 58,
    80, 87,
    80, 89,
    80, 90,
    81, 13,
    81, 15,
    81, 53,
    81, 55,
    81, 56,
    81, 57,
    81, 58,
    81, 87,
    81, 89,
    81, 90,
    82, 53,
    82, 58,
    82, 75,
    83, 3,
    83, 8,
    83, 13,
    83, 15,
    83, 66,
    83, 68,
    83, 69,
    83, 70,
    83, 71,
    83, 80,
    83, 82,
    83, 85,
    84, 53,
    84, 55,
    84, 56,
    84, 57,
    84, 58,
    84, 87,
    84, 89,
    84, 90,
    85, 71,
    86, 53,
    86, 58,
    87, 13,
    87, 15,
    87, 53,
    87, 57,
    87, 66,
    87, 68,
    87, 69,
    87, 70,
    87, 80,
    87, 82,
    88, 13,
    88, 15,
    88, 53,
    88, 57,
    88, 66,
    89, 53,
    89, 54,
    89, 56,
    89, 66,
    89, 68,
    89, 69,
    89, 70,
    89, 72,
    89, 77,
    89, 80,
    89, 82,
    89, 86,
    90, 13,
    90, 15,
    90, 53,
    90, 57,
    90, 66,
    90, 68,
    90, 69,
    90, 70,
    90, 80,
    90, 82,
    91, 53,
    92, 73,
    92, 75,
    92, 76
};

/* Kerning between the respective left and right glyphs
 * 4.4 format which needs to scaled with `kern_scale`*/
static const int8_t kern_pair_values[] =
{
    -14, -15, -46, -46, -14, -15, -46, -46,
    15, -41, -41, -36, -10, -10, -41, -41,
    -36, -10, -10, -31, -25, -25, -25, -25,
    -41, -41, -25, -25, -20, -15, -20, -15,
    -26, -26, -5, -5, -5, -5, -20, -3,
    -18, -10, -21, -5, -3, -5, -5, -5,
    -3, -3, -3, -3, -10, -5, -5, -5,
    -15, -15, -5, -9, -8, -5, -5, -10,
    -5, -10, 5, -41, -41, -10, -15, 0,
    0, -3, -5, 2, -5, -5, -5, -5,
    -5, -5, -10, -10, -10, -10, -10, -10,
    -10, -10, -10, -10, -10, -5, -3, -10,
    -15, -10, -15, -25, -25, 5, -15, -15,
    -15, -15, -20, -5, -20, -10, -26, -10,
    -5, -10, -15, -15, -5, -9, -8, -5,
    -5, -10, -5, -10, -51, -51, -15, -7,
    -3, -3, -13, -5, -10, -15, -15, -15,
    -5, 2, -9, -8, -5, -5, -10, -5,
    -10, -4, -4, -4, -4, -10, -3, -5,
    -3, -7, -10, -5, -5, -5, -5, -5,
    -5, -5, -3, -3, 5, -3, -5, -5,
    0, 0, -5, -3, -5, 0, -5, -3,
    0, -5, 1, 1, -39, -39, -20, -5,
    -5, -5, -5, 0, -20, -15, -15, -15,
    -15, -5, -5, -15, -15, -15, -15, -15,
    -15, -15, 5, -20, -10, -5, -10, -10,
    -15, -3, -5, 0, -36, -36, -10, -10,
    -18, -8, -8, -8, -8, -3, -10, -5,
    -5, -5, -5, 0, 0, -5, -5, -5,
    0, 0, -20, -20, -10, -10, -10, -5,
    -5, -5, -5, -10, -5, -5, -5, -5,
    -5, -5, -5, -5, -5, -5, -5, -5,
    -5, -10, -10, -10, -10, -10, -10, -10,
    -10, -10, -10, -10, -5, -3, -10, -15,
    -10, -15, 0, 0, -36, -36, -15, -15,
    -21, -10, -10, -10, -10, 0, -26, -15,
    -15, -15, 0, -15, 0, -5, 0, -10,
    -10, -15, -10, -15, -10, -16, 0, -10,
    -10, -10, -10, -10, -5, -5, -5, -5,
    20, -15, -5, -16, -3, -3, -3, -10,
    -10, -15, -5, -5, -10, -15, -3, -5,
    -3, -5, -5, -15, -10, 5, 1, 1,
    0, 16, 0, 0, 10, 10, 5, 5,
    5, 0, 5, -7, -5, -5, -5, 0,
    -10, -5, -5, -5, -5, -5, -5, -5,
    -5, 0, 1, -15, -10, -3, -3, 1,
    1, 1, 1, -5, 10, 6, -5, -10,
    0, -5, -5, -5, -5, -5, -5, -5,
    -5, -5, -3, -5, -5, -3, -5, -15,
    -10, -3, -3, -15, -10, -3, -3, -10,
    -10, -15, -5, -5, -10, -15, -3, -5,
    -3, -10, -10, -15, -5, -5, -10, -15,
    -3, -5, -3, -5, -5, 5, 1, 1,
    -23, -23, -13, -5, -5, -5, 0, -5,
    -5, 5, -15, -5, -5, -5, -15, -5,
    -3, -5, 0, -10, -5, -23, -23, -10,
    -10, -9, -3, -3, -3, -3, -3, -13,
    -13, -5, -10, -8, -10, 0, -5, -5,
    -5, -5, -5, -5, -5, -5, -5, -3,
    -23, -23, -10, -10, -9, -3, -3, -3,
    -3, -3, -15, 1, 20, 1
};

/*Collect the kern pair's data in one place*/
static const lv_font_fmt_txt_kern_pair_t kern_pairs =
{
    .glyph_ids = kern_pair_glyph_ids,
    .values = kern_pair_values,
    .pair_cnt = 486,
    .glyph_ids_size = 0
};

/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = &kern_pairs,
    .kern_scale = 16,
    .cmap_num = 2,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t ui_font_Font1 = {
#else
lv_font_t ui_font_Font1 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 16,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if UI_FONT_FONT1*/

