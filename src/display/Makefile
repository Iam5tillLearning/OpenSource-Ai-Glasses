# ARCH=arm

# CC = /home/yangfan/ZNYJ/rv1106b_rv1103b_linux_ipc_v1.0.0_20241016/tools/linux/toolchain/arm-rockchip831-linux-uclibcgnueabihf/bin/arm-rockchip831-linux-uclibcgnueabihf-gcc
# LVGL_DIR_NAME   ?= lvgl
# LVGL_DIR        ?= .

# WARNINGS        := -Wall -Wshadow -Wundef -Wmissing-prototypes -Wno-discarded-qualifiers -Wall -Wextra -Wno-unused-function -Wno-error=strict-prototypes -Wpointer-arith \
#                     -fno-strict-aliasing -Wno-error=cpp -Wuninitialized -Wmaybe-uninitialized -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess \
#                     -Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wformat-security -Wno-ignored-qualifiers -Wno-error=pedantic \
#                     -Wno-sign-compare -Wno-error=missing-prototypes -Wdouble-promotion -Wclobbered -Wdeprecated -Wempty-body -Wtype-limits -Wshift-negative-value -Wstack-usage=2048 \
#                     -Wno-unused-value -Wno-unused-parameter -Wno-missing-field-initializers -Wuninitialized -Wmaybe-uninitialized -Wall -Wextra -Wno-unused-parameter \
#                     -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wpointer-arith -Wno-cast-qual -Wmissing-prototypes \
#                     -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wno-discarded-qualifiers -Wformat-security -Wno-ignored-qualifiers -Wno-sign-compare -std=c99
# CFLAGS          ?= -O0 -g0 -I. -I$(LVGL_DIR)/ -I$(UI_DIR)  $(WARNINGS) 
# #CFLAGS          ?= -O0 -g0 -I. -I$(LVGL_DIR)/ -I$(UI_DIR)  -I./$(AUDIO_DIR) $(WARNINGS) 
# LDFLAGS         := -L/home/kkk/luckfox-pico/sysdrv/source/kernel/lib -lm 
# #LDFLAGS         := -L/home/kkk/luckfox-pico/sysdrv/source/kernel/lib -lm -lrkmedia
# BIN             = app2
# BUILD_DIR       = ./build
# BUILD_OBJ_DIR   = $(BUILD_DIR)/obj
# BUILD_BIN_DIR   = $(BUILD_DIR)/bin

# #Collect the files to compile
# MAINSRC = $(wildcard ./*.c)

# # UI 相关的文件路径
# UI_DIR = ./ui
# UI_SRC = $(wildcard $(UI_DIR)/*.c)
# UI_OBJ = $(UI_SRC:.c=.o)

# # 多媒体相关的路径
# # AUDIO_DIR = ./audio
# # AUDIO_SRC = $(wildcard $(AUDIO_DIR)/*.c)  # 包含audio.c
# # AUDIO_OBJ = $(AUDIO_SRC:.c=.o)

# include $(LVGL_DIR)/lvgl/lvgl.mk
# #include $(LVGL_DIR)/lv_drivers/lv_drivers.mk
# CSRCS += $(LVGL_DIR)/lvgl/src/extra/themes/default/lv_theme_default.c
# CSRCS += $(LVGL_DIR)/lvgl/src/widgets/lv_keyboard.c
# CSRCS += $(LVGL_DIR)/lvgl/src/widgets/lv_spinbox.c
# CSRCS += $(LVGL_DIR)/lvgl/src/font/lv_customer_font_simhei_20.c
# CSRCS += $(LVGL_DIR)/lvgl/src/font/lv_font_montserrat_24.c


# OBJEXT          ?= .o

# AOBJS           = $(ASRCS:.S=$(OBJEXT))
# COBJS           = $(CSRCS:.c=$(OBJEXT))

# MAINOBJ         = $(MAINSRC:.c=$(OBJEXT))

# SRCS            = $(ASRCS) $(CSRCS) $(MAINSRC) $(UI_SRC)
# OBJS            = $(AOBJS) $(COBJS) $(MAINOBJ) $(UI_OBJ)
# TARGET          = $(addprefix $(BUILD_OBJ_DIR)/, $(patsubst ./%, %, $(OBJS)))

# ## MAINOBJ -> OBJFILES

# all: default

# $(BUILD_OBJ_DIR)/%.o: %.c
# 	@mkdir -p $(dir $@)
# 	@$(CC)  $(CFLAGS) -c $< -o $@
# 	@echo "CC $<"

# $(BUILD_OBJ_DIR)/ui/%.o: $(UI_DIR)/%.c
# 	@mkdir -p $(dir $@)
# 	@$(CC)  $(CFLAGS) -c $< -o $@
# 	@echo "CC $<"
    
# default: $(TARGET)
# 	@mkdir -p $(dir $(BUILD_BIN_DIR)/)
# 	$(CC) -o $(BUILD_BIN_DIR)/$(BIN) $(TARGET) $(LDFLAGS)

# clean: 
# 	rm -rf $(BUILD_DIR)



# 项目根目录相对路径 (相对于 Makefile 所在目录)
# 假设 OpenSource-Ai-Glasses 项目放在 aiglass_dev_env 目录下
DEV_ENV_ROOT    := ../../..

# 交叉编译工具链设置
TOOLCHAIN_PREFIX := $(DEV_ENV_ROOT)/tools/linux/toolchain/arm-rockchip831-linux-uclibcgnueabihf/bin/arm-rockchip831-linux-uclibcgnueabihf-
CC = $(TOOLCHAIN_PREFIX)gcc

LVGL_DIR_NAME   ?= lvgl
LVGL_DIR        ?= ../../third_party

WARNINGS        := -Wall -Wshadow -Wundef -Wmissing-prototypes -Wno-discarded-qualifiers -Wall -Wextra -Wno-unused-function -Wno-error=strict-prototypes -Wpointer-arith \
                    -fno-strict-aliasing -Wno-error=cpp -Wuninitialized -Wmaybe-uninitialized -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess \
                    -Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wformat-security -Wno-ignored-qualifiers -Wno-error=pedantic \
                    -Wno-sign-compare -Wno-error=missing-prototypes -Wdouble-promotion -Wclobbered -Wdeprecated -Wempty-body -Wtype-limits -Wshift-negative-value -Wstack-usage=2048 \
                    -Wno-unused-value -Wno-unused-parameter -Wno-missing-field-initializers -Wuninitialized -Wmaybe-uninitialized -Wall -Wextra -Wno-unused-parameter \
                    -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wpointer-arith -Wno-cast-qual -Wmissing-prototypes \
                    -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wno-discarded-qualifiers -Wformat-security -Wno-ignored-qualifiers -Wno-sign-compare -std=c99

# 在CFLAGS中添加预定义宏来消除警告
CFLAGS          ?= -O0 -g0 -I. -I$(LVGL_DIR)/ -I$(UI_DIR)  $(WARNINGS) \
                    -DLV_LOG_TRACE_INDEV=0 \
                    -DLV_LOG_LEVEL=0 \
                    -DLV_LOG_TRACE_EVENT=0 \
                    -DLV_LOG_TRACE_DISP_REFR=0 \
                    -DLV_LOG_TRACE_ANIM=0 \
                    -DLV_LOG_TRACE_MEM=0 \
                    -DLV_LOG_TRACE_TIMER=0

# SDK 库路径
SDK_LIB_PATH    := $(DEV_ENV_ROOT)/sysdrv/source/kernel/lib
LDFLAGS         := -L$(SDK_LIB_PATH) -L. -lm -lpthread -ldl -lbat

BIN             = display
BUILD_DIR       = ./build
BUILD_OBJ_DIR   = $(BUILD_DIR)/obj
BUILD_BIN_DIR   = $(BUILD_DIR)/bin

MAINSRC = $(wildcard ./*.c)

UI_DIR = ./ui
UI_SRC = $(shell find $(UI_DIR) -type f -name '*.c')
UI_OBJ = $(UI_SRC:.c=.o)

include $(LVGL_DIR)/lvgl/lvgl.mk

# lvgl.mk 已经包含了所有 LVGL 源文件，不需要再额外添加
# 如果需要中文字体支持，需要自行生成字体文件并添加到 CSRCS
# 可以使用 LVGL 字体转换工具：https://lvgl.io/tools/fontconverter

OBJEXT          ?= .o
AOBJS           = $(ASRCS:.S=$(OBJEXT))
COBJS           = $(CSRCS:.c=$(OBJEXT))
MAINOBJ         = $(MAINSRC:.c=$(OBJEXT))
SRCS            = $(ASRCS) $(CSRCS) $(MAINSRC) $(UI_SRC)
OBJS            = $(AOBJS) $(COBJS) $(MAINOBJ) $(UI_OBJ)
TARGET          = $(addprefix $(BUILD_OBJ_DIR)/, $(patsubst ./%, %, $(OBJS)))

all: default

# 通用规则
$(BUILD_OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@$(CC)  $(CFLAGS) -c $< -o $@
	@echo "CC $<"

# UI规则（支持子目录）
$(BUILD_OBJ_DIR)/ui/%.o: $(UI_DIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC)  $(CFLAGS) -c $< -o $@
	@echo "CC $<"

# 处理所有extra目录下的文件
$(BUILD_OBJ_DIR)/lvgl/src/extra/%.o: $(LVGL_DIR)/lvgl/src/extra/%.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "CC $(subst $(LVGL_DIR)/,,$<)"

default: $(TARGET)
	@mkdir -p $(dir $(BUILD_BIN_DIR)/)
	$(CC) -o $(BUILD_BIN_DIR)/$(BIN) $(TARGET) $(LDFLAGS)

clean: 
	rm -rf $(BUILD_DIR)