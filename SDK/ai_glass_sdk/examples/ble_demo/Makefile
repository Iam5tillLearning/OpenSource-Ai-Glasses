# Makefile for BLE Demo

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
CFLAGS = -Wall -Wextra -g -O2 -std=gnu99

SDK_DIR = ../..
INCLUDES = -I$(SDK_DIR)/include
SDK_STATIC_LIB = $(SDK_DIR)/lib/libai_glass_sdk.a

BUILD_DIR = ../build
TARGET = $(BUILD_DIR)/ble_demo
SOURCE = ble_demo.c

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $@

$(TARGET): $(SOURCE) $(SDK_STATIC_LIB) | $(BUILD_DIR)
	@echo "Building BLE demo..."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(SOURCE) $(SDK_STATIC_LIB) -lpthread -lrt -lm
	@echo "Built: $(TARGET)"

clean:
	rm -f $(TARGET)

help:
	@echo "BLE demo Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all   - Build glasses-side BLE demo"
	@echo "  clean - Remove glasses-side build output"
	@echo ""
	@echo "Android demo:"
	@echo "  cd android && bash build_android.sh"

.PHONY: all clean help
