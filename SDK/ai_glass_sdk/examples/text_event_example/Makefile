# Makefile for Text Event Example
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
CFLAGS = -Wall -g -I../../include

# 使用SDK预编译静态库
SDK_LIB = ../../lib/libai_glass_sdk.a
LDFLAGS = $(SDK_LIB) -lpthread -lrt -lm

BUILD_DIR = ../build
TARGET = $(BUILD_DIR)/text_event_client
SRCS = main.c
OBJS = $(SRCS:.c=.o)

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
