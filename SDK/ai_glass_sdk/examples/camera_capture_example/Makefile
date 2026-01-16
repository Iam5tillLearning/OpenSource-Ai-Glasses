# Makefile for Camera Capture Example
# 摄像头捕获示例程序

# 编译器和工具链
ifeq ($(CROSS_COMPILE),)
CROSS_COMPILE = arm-rockchip831-linux-uclibcgnueabihf-
endif

CC = $(CROSS_COMPILE)gcc

# 编译选项
CFLAGS = -Wall -g -O2
CFLAGS += -std=gnu99

# SDK路径（当前在ai_glass_sdk/examples/camera_capture_example/）
SDK_DIR = ../..

# 头文件路径
INCLUDES = -I$(SDK_DIR)/include

# 库文件
LIBS = -lrt

# 源文件
EXAMPLE_SOURCE = camera_capture_example.c
SDK_SOURCES = $(SDK_DIR)/src/ai_camera.c

# 目标文件
BUILD_DIR = ../build
TARGET = $(BUILD_DIR)/camera_capture_example

# 默认目标
all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $@

# 编译程序
$(TARGET): $(EXAMPLE_SOURCE) $(SDK_SOURCES) | $(BUILD_DIR)
	@echo "编译摄像头捕获示例程序..."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(EXAMPLE_SOURCE) $(SDK_SOURCES) $(LIBS)
	@echo "✅ 编译完成: $(TARGET)"
	@echo ""
	@echo "运行方法:"
	@echo "  ./$(TARGET) [save_path]"
	@echo ""
	@echo "注意事项:"
	@echo "  1. 确保ai-core服务端已启动"
	@echo "  2. save_path为可选参数，默认保存到/tmp"

# 清理
clean:
	@echo "清理编译产物..."
	rm -f $(TARGET)
	@echo "✅ 清理完成"

# 帮助信息
help:
	@echo "摄像头捕获示例程序 Makefile"
	@echo ""
	@echo "目标:"
	@echo "  all   - 编译程序（默认）"
	@echo "  clean - 清理编译产物"
	@echo "  help  - 显示此帮助信息"

.PHONY: all clean help
