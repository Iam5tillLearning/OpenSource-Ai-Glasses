# Makefile for query AI-Core physical actions sample

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
CFLAGS = -Wall -g -O2 -std=gnu99

SDK_DIR = ../..
INCLUDES = -I$(SDK_DIR)/include
SDK_STATIC_LIB = $(SDK_DIR)/lib/libai_glass_sdk.a
LIBS = $(SDK_STATIC_LIB) -lpthread -lrt -lm

BUILD_DIR = ../build
TARGET = $(BUILD_DIR)/query_aicore_physical_actions_example
SRC = query_aicore_physical_actions_example.c

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $@

$(TARGET): $(SRC) | $(BUILD_DIR)
	@echo "Building query_aicore_physical_actions_example..."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(SRC) $(LIBS)
	@echo "OK: $(TARGET)"

clean:
	rm -f $(TARGET)

help:
	@echo "make            # build"
	@echo "make clean      # clean output"
	@echo "run command:"
	@echo "  ./$(TARGET)"

.PHONY: all clean help
