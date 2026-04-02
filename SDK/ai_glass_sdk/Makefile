# Makefile for AI Glass SDK
# 编译AI-Core客户端SDK和示例程序

# 工具链路径 (使用绝对路径，参考 display-service)
DEV_ENV_ROOT := ../../..
TOOLCHAIN_PATH := $(DEV_ENV_ROOT)/tools/linux/toolchain/arm-rockchip831-linux-uclibcgnueabihf/bin

CC = $(TOOLCHAIN_PATH)/arm-rockchip831-linux-uclibcgnueabihf-gcc
AR = $(TOOLCHAIN_PATH)/arm-rockchip831-linux-uclibcgnueabihf-ar
STRIP = $(TOOLCHAIN_PATH)/arm-rockchip831-linux-uclibcgnueabihf-strip

# 编译选项
CFLAGS = -Wall -g -O2
CFLAGS += -std=gnu99
CFLAGS += -D_GNU_SOURCE
CFLAGS += -fPIC
CFLAGS += -I../../../media/rockit/out/include

# 库文件
LIBS = -lrt -lpthread

# 源文件
CLIENT_SOURCES = src/ai_camera.c src/ai_audio.c \
                 src/ai_gpio.c src/ai_ipc.c src/ai_log.c \
                 src/ai_display.c src/ai_text_event.c \
                 src/http_client.c src/ws_client.c \
                 third_party/mongoose/mongoose.c
HEADERS = include/ai_camera.h include/ai_audio.h \
          include/ai_gpio.h include/ai_ipc.h include/ai_log.h \
          include/ai_display.h include/ai_text_event.h \
          include/http_client.h include/ws_client.h

# 目标对象文件
OBJS = lib/ai_camera.o lib/ai_audio.o \
       lib/ai_gpio.o lib/ai_ipc.o lib/ai_log.o \
       lib/ai_display.o lib/ai_text_event.o \
       lib/http_client.o lib/ws_client.o lib/mongoose.o

# 目标库文件
CLIENT_LIB = lib/libai_glass_sdk.a
CLIENT_SO = lib/libai_glass_sdk.so

# 子目录（examples下的所有示例程序）
EXAMPLE_DIRS = examples/audio_play_example \
               examples/gpio_example \
               examples/camera_capture_example \
               examples/disable_physical_interaction_example \
               examples/record_audio_example \
               examples/http_example \
               examples/websocket_example \
               examples/text_event_example \
               examples/media_resource_control

# 默认目标（编译库和所有examples）
all: lib $(CLIENT_LIB) $(CLIENT_SO) examples

# 编译所有示例程序
examples: $(CLIENT_LIB) $(CLIENT_SO)
	@echo "🔨 Building all examples..."
	@for dir in $(EXAMPLE_DIRS); do \
		if [ -f $$dir/Makefile ]; then \
			echo "📂 Building $$dir..."; \
			$(MAKE) -C $$dir || exit 1; \
		else \
			echo "⚠️  Skipping $$dir (no Makefile)"; \
		fi \
	done
	@echo "✅ All examples built successfully"

# 创建构建目录
lib:
	mkdir -p lib

# 编译静态库
$(CLIENT_LIB): $(OBJS)
	@echo "📚 Building static library..."
	$(AR) rcs $@ $^
	@echo "✅ Static library built: $(CLIENT_LIB)"

# 编译动态库
$(CLIENT_SO): $(OBJS)
	@echo "📚 Building shared library..."
	$(CC) -shared -o $@ $^ $(LIBS)
	@echo "✅ Shared library built: $(CLIENT_SO)"

# 编译各个模块的对象文件
lib/ai_camera.o: src/ai_camera.c include/ai_camera.h
	@echo "🔨 Compiling camera client module..."
	$(CC) $(CFLAGS) -Iinclude -c src/ai_camera.c -o $@

lib/ai_audio.o: src/ai_audio.c include/ai_audio.h
	@echo "🔨 Compiling audio client module..."
	$(CC) $(CFLAGS) -Iinclude -c src/ai_audio.c -o $@

lib/ai_gpio.o: src/ai_gpio.c include/ai_gpio.h include/ai_ipc.h
	@echo "🔨 Compiling GPIO event client module..."
	$(CC) $(CFLAGS) -Iinclude -c src/ai_gpio.c -o $@

lib/ai_ipc.o: src/ai_ipc.c include/ai_ipc.h
	@echo "🔨 Compiling IPC module..."
	$(CC) $(CFLAGS) -Iinclude -c src/ai_ipc.c -o $@

lib/ai_log.o: src/ai_log.c include/ai_log.h
	@echo "🔨 Compiling log module..."
	$(CC) $(CFLAGS) -Iinclude -c src/ai_log.c -o $@

lib/ai_display.o: src/ai_display.c include/ai_display.h
	@echo "🔨 Compiling display client module..."
	$(CC) $(CFLAGS) -Iinclude -c src/ai_display.c -o $@

lib/ai_text_event.o: src/ai_text_event.c include/ai_text_event.h include/ai_ipc.h
	@echo "🔨 Compiling text event client module..."
	$(CC) $(CFLAGS) -Iinclude -c src/ai_text_event.c -o $@

lib/http_client.o: src/http_client.c include/http_client.h third_party/mongoose/mongoose.h
	@echo "🔨 Compiling HTTP client module..."
	$(CC) $(CFLAGS) -Iinclude -Ithird_party/mongoose -c src/http_client.c -o $@

lib/ws_client.o: src/ws_client.c include/ws_client.h third_party/mongoose/mongoose.h
	@echo "🔨 Compiling WebSocket client module..."
	$(CC) $(CFLAGS) -Iinclude -Ithird_party/mongoose -c src/ws_client.c -o $@

lib/mongoose.o: third_party/mongoose/mongoose.c third_party/mongoose/mongoose.h
	@echo "🔨 Compiling mongoose library (with mbedTLS)..."
	$(CC) $(CFLAGS) -Ithird_party/mbedtls/include -DMG_ENABLE_PACKED_FS=0 -DMG_ENABLE_SSI=0 -DMG_ENABLE_DIRECTORY_LISTING=0 -DMG_ENABLE_LOG=0 -DMG_TLS=1 -c third_party/mongoose/mongoose.c -o $@

# 清理编译产物（包括所有子目录）
clean:
	@echo "🧹 Cleaning build artifacts..."
	rm -rf lib/
	@for dir in $(EXAMPLE_DIRS); do \
		if [ -f $$dir/Makefile ]; then \
			echo "🧹 Cleaning $$dir..."; \
			$(MAKE) -C $$dir clean; \
		fi \
	done
	@echo "✅ Clean completed"

# 安装头文件和库
install: $(CLIENT_LIB) $(CLIENT_SO)
	@echo "📦 Installing AI Glass SDK..."
	mkdir -p /usr/local/include
	mkdir -p /usr/local/lib
	cp include/*.h /usr/local/include/
	cp $(CLIENT_LIB) /usr/local/lib/
	cp $(CLIENT_SO) /usr/local/lib/
	ldconfig
	@echo "✅ Installation completed"

# 卸载
uninstall:
	@echo "🗑️ Uninstalling AI Glass SDK..."
	rm -f /usr/local/include/ai_camera.h
	rm -f /usr/local/include/ai_audio.h
	rm -f /usr/local/include/ai_gpio.h
	rm -f /usr/local/include/ai_ipc.h
	rm -f /usr/local/include/ai_log.h
	rm -f /usr/local/include/http_client.h
	rm -f /usr/local/include/ws_client.h
	rm -f /usr/local/lib/libai_glass_sdk.a
	rm -f /usr/local/lib/libai_glass_sdk.so
	ldconfig
	@echo "✅ Uninstallation completed"

# 发布版本 (优化并去除调试信息)
release: CFLAGS += -O3 -DNDEBUG
release: clean all
	$(STRIP) $(CLIENT_SO)
	@echo "✅ Release build completed"

# 显示帮助信息
help:
	@echo "AI Glass SDK Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build libraries and all examples"
	@echo "  examples  - Build only examples (requires libraries)"
	@echo "  clean     - Clean build artifacts (including examples)"
	@echo "  install   - Install headers and libraries"
	@echo "  uninstall - Remove installed files"
	@echo "  release   - Build optimized release version"
	@echo "  help      - Show this help"
	@echo ""
	@echo "Usage examples:"
	@echo "  make              # Build libraries and examples"
	@echo "  make examples     # Build only examples"
	@echo "  make clean        # Clean all"
	@echo "  make install      # Install"
	@echo "  make release      # Release build"
	@echo ""
	@echo "Example directories:"
	@for dir in $(EXAMPLE_DIRS); do echo "  - $$dir"; done

# 声明伪目标
.PHONY: all clean install uninstall release help examples

# 编译信息
info:
	@echo "Build Configuration:"
	@echo "  Compiler: $(CC)"
	@echo "  Flags: $(CFLAGS)"
	@echo "  Libraries: $(LIBS)"
	@echo "  Targets: $(CLIENT_LIB) $(CLIENT_SO)"
