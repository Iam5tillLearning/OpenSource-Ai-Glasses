SET(SOURCES screens/ui_Screen1.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    images/ui_img_weixiao_png.c
    images/ui_img_799793925.c
    images/ui_img_lanya_png.c
    fonts/ui_font_Font1.c
    fonts/ui_font_Font2.c)

add_library(ui ${SOURCES})
