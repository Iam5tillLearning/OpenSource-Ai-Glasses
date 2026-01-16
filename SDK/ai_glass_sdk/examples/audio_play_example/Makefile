# Makefile for Audio Play Example
# 音频播放示例程序（使用AI Audio Client SDK）

# 编译器和工具链
ifeq ($(CROSS_COMPILE),)
CROSS_COMPILE = arm-rockchip831-linux-uclibcgnueabihf-
endif

CC = $(CROSS_COMPILE)gcc

# 编译选项
CFLAGS = -Wall -g -O2
CFLAGS += -std=gnu99

# SDK路径（当前在ai_glass_sdk/examples/audio_play_example/）
SDK_DIR = ../..

# 头文件路径（使用SDK）
INCLUDES = -I$(SDK_DIR)/include

# SDK源文件
SDK_SOURCES = $(SDK_DIR)/src/ai_audio.c

# 示例程序源文件
EXAMPLE_SOURCE = audio_play_example.c

# 目标文件
BUILD_DIR = ../build
TARGET = $(BUILD_DIR)/audio_play_example

# 默认目标
all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $@

# 编译程序（链接SDK源文件）
$(TARGET): $(EXAMPLE_SOURCE) $(SDK_SOURCES) | $(BUILD_DIR)
	@echo "🔨 编译音频播放示例程序..."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(EXAMPLE_SOURCE) $(SDK_SOURCES) -lpthread -lrt
	@echo "✅ 编译完成: $(TARGET)"

# 清理
clean:
	@echo "🧹 清理编译产物..."
	rm -f $(TARGET)
	@echo "✅ 清理完成"

# 帮助信息
help:
	@echo "音频播放示例 Makefile"
	@echo ""
	@echo "目标:"
	@echo "  all   - 编译程序（默认）"
	@echo "  clean - 清理编译产物"
	@echo "  help  - 显示此帮助信息"
	@echo ""
	@echo "使用方法:"
	@echo "  make              # 编译"
	@echo "  make clean        # 清理"
	@echo ""
	@echo "运行示例:"
	@echo "  ./audio_play_example -f /path/to/audio.pcm"
	@echo "  ./audio_play_example -f /path/to/audio.pcm -v 80 -F"
	@echo "  ./audio_play_example -S  # 停止播放"

.PHONY: all clean help
