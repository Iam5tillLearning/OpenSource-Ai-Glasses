DEV_ENV_ROOT := ../../..
TOOLCHAIN_PATH := $(DEV_ENV_ROOT)/tools/linux/toolchain/arm-rockchip831-linux-uclibcgnueabihf/bin
CC = $(TOOLCHAIN_PATH)/arm-rockchip831-linux-uclibcgnueabihf-gcc

# SDK Paths
SDK_DIR = ../../SDK/ai_glass_sdk
SDK_INC = $(SDK_DIR)/include
SDK_LIB = $(SDK_DIR)/lib

# LVGL Paths
LVGL_DIR_NAME ?= lvgl
LVGL_DIR      ?= ../../third_party
LVGL_PATH     = $(LVGL_DIR)/lvgl

# Build Directories
BUILD_DIR = build
OBJ_DIR   = $(BUILD_DIR)/obj
BIN_DIR   = $(BUILD_DIR)/bin

TARGET = launcher-app
TARGET_BIN = $(BIN_DIR)/$(TARGET)

CFLAGS = -Wall -O2 -I. -I$(SDK_INC) -I$(LVGL_DIR) -Iui -D_GNU_SOURCE
# LVGL Config
CFLAGS += -DLV_CONF_INCLUDE_SIMPLE

# Linker Flags
# 静态链接 SDK, 依然链接 pthread, rt, m
LDFLAGS = $(SDK_LIB)/libai_glass_sdk.a -lpthread -lrt -lm

# Include LVGL makefile
# This defines CSRCS (containing paths like ../../third_party/lvgl/src/core/lv_obj.c)
include $(LVGL_PATH)/lvgl.mk

# Sources
MAIN_SRC = main.c
UI_DIR = ui
UI_SRC = $(shell find $(UI_DIR) -type f -name '*.c')

# Object Files definition
# 1. Main Object
MAIN_OBJ = $(OBJ_DIR)/main.o

# 2. UI Objects (ui/foo.c -> build/obj/ui/foo.o)
UI_OBJ = $(patsubst %.c, $(OBJ_DIR)/%.o, $(UI_SRC))

# 3. LVGL Objects
# Map ../../third_party/lvgl/src/foo.c -> build/obj/lvgl/src/foo.o
# CSRCS paths start with $(LVGL_DIR)/ (e.g., ../../third_party/)
# 3. LVGL Objects
# LVGL mk files add filenames (e.g. lv_disp.c) or relative paths to CSRCS.
# We prepend build/obj/lvgl/ to ensure they go to the build dir.
# $(CSRCS:.c=.o) turns lv_disp.c -> lv_disp.o
# addprefix prepends $(OBJ_DIR)/lvgl/ -> build/obj/lvgl/lv_disp.o
LVGL_OBJ = $(addprefix $(OBJ_DIR)/lvgl/, $(CSRCS:.c=.o))

ALL_OBJS = $(MAIN_OBJ) $(UI_OBJ) $(LVGL_OBJ)

.PHONY: all clean

all: $(TARGET_BIN)

debug_vars:
	@echo "CSRCS: $(CSRCS)"
	@echo "LVGL_OBJ: $(LVGL_OBJ)"

$(TARGET_BIN): $(ALL_OBJS)
	@mkdir -p $(dir $@)
	@echo "Linking $@"
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Rule for main.c
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Rule for libraries (LVGL)
# Matches build/obj/lvgl/foo.o -> foo.c using VPATH resolution
$(OBJ_DIR)/lvgl/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Fallback rule for local objects (like UI) if not caught above
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) *.o ui/*.o

