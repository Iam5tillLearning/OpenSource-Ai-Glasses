# Makefile for Text Event Example
# 工具链路径 (使用绝对路径)
DEV_ENV_ROOT := ../../../../..
TOOLCHAIN_PATH := $(DEV_ENV_ROOT)/tools/linux/toolchain/arm-rockchip831-linux-uclibcgnueabihf/bin

CC = $(TOOLCHAIN_PATH)/arm-rockchip831-linux-uclibcgnueabihf-gcc
CFLAGS = -Wall -g -I../../include

# 使用静态库并链接 mbedTLS
SDK_LIB = ../../lib/libai_glass_sdk.a
MBEDTLS_LIBS = ../../third_party/mbedtls/library/libmbedtls.a \
               ../../third_party/mbedtls/library/libmbedx509.a \
               ../../third_party/mbedtls/library/libmbedcrypto.a
LDFLAGS = $(SDK_LIB) $(MBEDTLS_LIBS) -lpthread -lrt

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
