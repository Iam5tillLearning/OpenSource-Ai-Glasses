# Makefile for the public AI Glass SDK distribution
# The public package ships prebuilt SDK libraries and builds examples against them.

CLIENT_LIB = lib/libai_glass_sdk.a
CLIENT_SO = lib/libai_glass_sdk.so

EXAMPLE_DIRS = examples/audio_play_example \
               examples/gpio_example \
               examples/camera_capture_example \
               examples/disable_aicore_physical_actions_example \
               examples/record_audio_example \
               examples/bluetooth_demo/ble_demo/glasses \
               examples/bluetooth_demo/classic_bt_demo/glasses/sdk_spp_demo \
               examples/bluetooth_demo/combo_camera_spp_demo/glasses \
               examples/http_example \
               examples/websocket_example \
               examples/text_event_example \
               examples/media_resource_control

all: verify-libs examples

verify-libs:
	@test -f "$(CLIENT_LIB)" || { echo "ERROR: missing prebuilt $(CLIENT_LIB)"; exit 1; }
	@test -f "$(CLIENT_SO)" || { echo "ERROR: missing prebuilt $(CLIENT_SO)"; exit 1; }
	@echo "✅ Prebuilt SDK libraries are available"

examples: verify-libs
	@echo "🔨 Building all public SDK examples..."
	@for dir in $(EXAMPLE_DIRS); do \
		if [ -f $$dir/Makefile ]; then \
			echo "📂 Building $$dir..."; \
			$(MAKE) -C $$dir || exit 1; \
		else \
			echo "⚠️  Skipping $$dir (no Makefile)"; \
		fi; \
	done
	@echo "✅ All public SDK examples built successfully"

clean:
	@echo "🧹 Cleaning public SDK example outputs..."
	@for dir in $(EXAMPLE_DIRS); do \
		if [ -f $$dir/Makefile ]; then \
			$(MAKE) -C $$dir clean || exit 1; \
		fi; \
	done
	@echo "✅ Public SDK example outputs cleaned; prebuilt libraries preserved"

install: verify-libs
	@echo "📦 Installing AI Glass SDK..."
	mkdir -p /usr/local/include /usr/local/lib
	cp include/*.h /usr/local/include/
	cp $(CLIENT_LIB) $(CLIENT_SO) /usr/local/lib/
	ldconfig

uninstall:
	@for header in include/*.h; do rm -f "/usr/local/include/$$(basename $$header)"; done
	rm -f /usr/local/lib/libai_glass_sdk.a /usr/local/lib/libai_glass_sdk.so
	ldconfig

help:
	@echo "AI Glass SDK public distribution targets:"
	@echo "  make           - verify prebuilt SDK libraries and build all examples"
	@echo "  make clean     - remove example outputs without deleting SDK libraries"
	@echo "  make install   - install public headers and prebuilt SDK libraries"
	@echo "  make uninstall - remove installed public SDK files"

.PHONY: all verify-libs examples clean install uninstall help
