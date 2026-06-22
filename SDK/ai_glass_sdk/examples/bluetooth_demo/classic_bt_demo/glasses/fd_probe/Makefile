REPO_ROOT := $(abspath ../../../../../../../..)
TOOLCHAIN := $(REPO_ROOT)/tools/linux/toolchain/arm-rockchip831-linux-uclibcgnueabihf/bin
CC := $(TOOLCHAIN)/arm-rockchip831-linux-uclibcgnueabihf-gcc
STRIP := $(TOOLCHAIN)/arm-rockchip831-linux-uclibcgnueabihf-strip
DEBUG ?= 0
ifeq ($(DEBUG),1)
STRIP_OUTPUTS ?= 0
else
STRIP_OUTPUTS ?= 1
endif

BUILD_DIR := build
TARGET := $(BUILD_DIR)/spp_fd_probe

CFLAGS := -Wall -Wextra -O2
ifeq ($(DEBUG),1)
CFLAGS += -DDEBUG -g
else
CFLAGS += -DNDEBUG
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): spp_fd_probe.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<
	@if [ "$(STRIP_OUTPUTS)" != "0" ]; then $(STRIP) --strip-unneeded $@; fi

clean:
	rm -rf $(BUILD_DIR)
