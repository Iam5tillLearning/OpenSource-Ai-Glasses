DEV_ENV_ROOT := ../../..
TOOLCHAIN_PATH := $(DEV_ENV_ROOT)/tools/linux/toolchain/arm-rockchip831-linux-uclibcgnueabihf/bin
CC = $(TOOLCHAIN_PATH)/arm-rockchip831-linux-uclibcgnueabihf-gcc

CFLAGS = -Wall -O2 -I. -I../../SDK/ai_glass_sdk/include -I../../third_party/cJSON -D_GNU_SOURCE
SDK_LIB = ../../SDK/ai_glass_sdk/lib
LDFLAGS = $(SDK_LIB)/libai_glass_sdk.a -lpthread -lrt

# Build Directories
BUILD_DIR = build
OBJ_DIR   = $(BUILD_DIR)/obj
BIN_DIR   = $(BUILD_DIR)/bin

TARGET = display-service
TARGET_BIN = $(BIN_DIR)/$(TARGET)

# Sources
SRCS = main.c hal_driver.c jbd013_api.c config_utils.c
OBJS = $(addprefix $(OBJ_DIR)/, $(SRCS:.c=.o))

# cJSON
CJSON_DIR = ../../third_party/cJSON
CJSON_OBJ = $(OBJ_DIR)/cJSON.o
OBJS += $(CJSON_OBJ)

.PHONY: all clean

all: $(TARGET_BIN)

$(TARGET_BIN): $(OBJS)
	@mkdir -p $(dir $@)
	@echo "Linking $@"
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(CJSON_OBJ): $(CJSON_DIR)/cJSON.c
	@mkdir -p $(dir $@)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) *.o
