# Makefile for WebSocket Example

# 输出目录
BUILD_DIR = ../build
DEV_ENV_ROOT := ../../../../..
TOOLCHAIN_BIN := $(DEV_ENV_ROOT)/tools/linux/toolchain/arm-rockchip831-linux-uclibcgnueabihf/bin

# 交叉编译工具链
ifeq ($(CROSS_COMPILE),)
ifneq ($(wildcard $(TOOLCHAIN_BIN)/arm-rockchip831-linux-uclibcgnueabihf-gcc),)
CROSS_COMPILE = $(TOOLCHAIN_BIN)/arm-rockchip831-linux-uclibcgnueabihf-
else
CROSS_COMPILE = arm-rockchip831-linux-uclibcgnueabihf-
endif
endif

CC = $(CROSS_COMPILE)gcc

# 编译选项
CFLAGS = -Wall -O2 -std=gnu99
CFLAGS += -DMG_ENABLE_PACKED_FS=0
CFLAGS += -DMG_ENABLE_SSI=0
CFLAGS += -DMG_ENABLE_DIRECTORY_LISTING=0
CFLAGS += -DMG_ENABLE_LOG=1
# 使用mbedTLS（支持WSS连接）
CFLAGS += -DMG_TLS=1
# 启用详细调试日志（0=ERROR, 1=ERROR, 2=INFO, 3=DEBUG, 4=VERBOSE）
CFLAGS += -DMG_LOG_LEVEL=4

# 头文件路径
INCLUDES = -I../../include -I../../third_party/mongoose -I../../third_party/mbedtls/include

# 源文件
WS_CLIENT_SRC = ../../src/ws_client.c
MONGOOSE_SRC = ../../third_party/mongoose/mongoose.c

# mbedTLS静态库
MBEDTLS_LIBS = ../../third_party/mbedtls/library/libmbedtls.a \
               ../../third_party/mbedtls/library/libmbedx509.a \
               ../../third_party/mbedtls/library/libmbedcrypto.a

# 链接库
LIBS = -lpthread -lm

# 目标程序
TARGET = $(BUILD_DIR)/ws_example

all: $(BUILD_DIR) $(TARGET)

# 创建构建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 编译WebSocket示例
$(TARGET): ws_example.c $(WS_CLIENT_SRC) $(MONGOOSE_SRC) | $(BUILD_DIR)
	@echo "编译 WebSocket 示例 (with mbedTLS)..."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(WS_CLIENT_SRC) $(MONGOOSE_SRC) $(MBEDTLS_LIBS) $(LIBS)
	@echo "✅ 编译完成: $@"

clean:
	@echo "清理 WebSocket 示例程序..."
	rm -f $(TARGET)
	@echo "✅ 清理完成"

.PHONY: all clean
