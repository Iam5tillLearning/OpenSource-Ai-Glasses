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
STRIP = $(CROSS_COMPILE)strip
DEBUG ?= 0
ifeq ($(DEBUG),1)
STRIP_OUTPUTS ?= 0
else
STRIP_OUTPUTS ?= 1
endif

CFLAGS = -Wall -O2 -I../../include
ifeq ($(DEBUG),1)
CFLAGS += -DDEBUG -g
else
CFLAGS += -DNDEBUG
endif

# 使用SDK静态库
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
	@if [ "$(STRIP_OUTPUTS)" != "0" ]; then $(STRIP) --strip-unneeded $(TARGET); fi

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
