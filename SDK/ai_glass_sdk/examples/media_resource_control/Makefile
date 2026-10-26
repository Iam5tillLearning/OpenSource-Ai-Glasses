# Makefile for Media Resource Control Example
# 媒体资源切换控制台示例

DEV_ENV_ROOT := ../../../../..
TOOLCHAIN_BIN := $(DEV_ENV_ROOT)/tools/linux/toolchain/arm-rockchip831-linux-uclibcgnueabihf/bin

ifeq ($(CROSS_COMPILE),)
ifneq ($(wildcard $(TOOLCHAIN_BIN)/arm-rockchip831-linux-uclibcgnueabihf-gcc),)
CROSS_COMPILE = $(TOOLCHAIN_BIN)/arm-rockchip831-linux-uclibcgnueabihf-
else
CROSS_COMPILE = arm-rockchip831-linux-uclibcgnueabihf-
endif
endif

CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip
DEBUG ?= 0
ifeq ($(DEBUG),1)
STRIP_OUTPUTS ?= 0
else
STRIP_OUTPUTS ?= 1
endif

CFLAGS = -Wall -O2
CFLAGS += -std=gnu99
ifeq ($(DEBUG),1)
CFLAGS += -DDEBUG -g
else
CFLAGS += -DNDEBUG
endif

SDK_DIR = ../..
INCLUDES = -I$(SDK_DIR)/include
SDK_STATIC_LIB = $(SDK_DIR)/lib/libai_glass_sdk.a

EXAMPLE_SOURCE = media_resource_control.c
BUILD_DIR = ../build
TARGET = $(BUILD_DIR)/media_resource_control

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $@

$(TARGET): $(EXAMPLE_SOURCE) | $(BUILD_DIR)
	@echo "🔨 编译媒体资源切换控制台..."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(EXAMPLE_SOURCE) $(SDK_STATIC_LIB) -lpthread -lrt -lm
	@if [ "$(STRIP_OUTPUTS)" != "0" ]; then $(STRIP) --strip-unneeded $@; fi
	@echo "✅ 编译完成: $(TARGET)"

clean:
	@echo "🧹 清理编译产物..."
	rm -f $(TARGET)
	@echo "✅ 清理完成"

help:
	@echo "媒体资源切换控制台 Makefile"
	@echo ""
	@echo "目标:"
	@echo "  all   - 编译程序（默认）"
	@echo "  clean - 清理编译产物"
	@echo "  help  - 显示帮助信息"
	@echo ""
	@echo "运行示例:"
	@echo "  ./media_resource_control"
	@echo "  ./media_resource_control /tmp/ai-core_audio_ctrl"
	@echo ""
	@echo "核心指令:"
	@echo "  cam_on / cam_off / aud_on / aud_off"

.PHONY: all clean help
