# Makefile for GPIO Example
# 编译GPIO示例程序（使用GPIO客户端SDK）

DEV_ENV_ROOT := ../../../../..
TOOLCHAIN_BIN := $(DEV_ENV_ROOT)/tools/linux/toolchain/arm-rockchip831-linux-uclibcgnueabihf/bin

# 编译器和工具链 (使用ARM交叉编译工具链)
ifeq ($(CROSS_COMPILE),)
ifneq ($(wildcard $(TOOLCHAIN_BIN)/arm-rockchip831-linux-uclibcgnueabihf-gcc),)
CROSS_COMPILE = $(TOOLCHAIN_BIN)/arm-rockchip831-linux-uclibcgnueabihf-
else
CROSS_COMPILE = arm-rockchip831-linux-uclibcgnueabihf-
endif
endif

CC = $(CROSS_COMPILE)gcc

# 编译选项
CFLAGS = -Wall -g -O2
CFLAGS += -std=gnu99
CFLAGS += -D_GNU_SOURCE
CFLAGS += -DLINUX
CFLAGS += -DOS_LINUX

# SDK路径（当前在ai_glass_sdk/examples/gpio_example/）
SDK_DIR = ../..

# 头文件路径（使用SDK）
INCLUDES = -I$(SDK_DIR)/include

# 库文件
SDK_STATIC_LIB = $(SDK_DIR)/lib/libai_glass_sdk.a
LIBS = $(SDK_STATIC_LIB) -lpthread -lrt -lm

# 源文件（使用SDK预编译静态库）
EXAMPLE_SOURCE = gpio_example.c

# 目标文件
BUILD_DIR = ../build
TARGET = $(BUILD_DIR)/gpio_example

# 默认目标
all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $@

# 编译示例程序（使用SDK）
$(TARGET): $(EXAMPLE_SOURCE) | $(BUILD_DIR)
	@echo "编译GPIO示例程序（使用SDK）..."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(EXAMPLE_SOURCE) $(LIBS)
	@echo "✅ 编译完成: $(TARGET)"
	@echo ""
	@echo "SDK路径: $(SDK_DIR)"
	@echo ""
	@echo "运行方法:"
	@echo "  ./$(TARGET)"
	@echo ""
	@echo "注意事项:"
	@echo "  1. 确保ai-core服务端已启动"
	@echo "  2. 服务端需启用GPIO功能（--enable-gpio）"
	@echo ""

# 清理
clean:
	@echo "清理编译产物..."
	rm -f $(TARGET)
	@echo "✅ 清理完成"

# 运行示例
run: $(TARGET)
	@echo "运行GPIO示例..."
	./$(TARGET)

# 帮助信息
help:
	@echo "GPIO示例程序 Makefile"
	@echo ""
	@echo "目标:"
	@echo "  all   - 编译示例程序（默认）"
	@echo "  clean - 清理编译产物"
	@echo "  run   - 编译并运行示例程序"
	@echo "  help  - 显示此帮助信息"
	@echo ""
	@echo "使用示例:"
	@echo "  make              # 编译"
	@echo "  make run          # 编译并运行"
	@echo "  make clean        # 清理"

.PHONY: all clean run help
