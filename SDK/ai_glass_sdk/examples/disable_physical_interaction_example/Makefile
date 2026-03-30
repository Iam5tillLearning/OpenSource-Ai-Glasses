# Makefile for disable physical interaction sample

ifeq ($(CROSS_COMPILE),)
CROSS_COMPILE = arm-rockchip831-linux-uclibcgnueabihf-
endif

CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -g -O2 -std=gnu99

SDK_DIR = ../..
INCLUDES = -I$(SDK_DIR)/include
SDK_STATIC_LIB = $(SDK_DIR)/lib/libai_glass_sdk.a
LIBS = $(SDK_STATIC_LIB) -lpthread -lrt -lm

BUILD_DIR = ../build
TARGET = $(BUILD_DIR)/disable_physical_interaction_example
SRC = disable_physical_interaction_example.c

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $@

$(TARGET): $(SRC) | $(BUILD_DIR)
	@echo "Building disable_physical_interaction_example..."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(SRC) $(LIBS)
	@echo "OK: $(TARGET)"

clean:
	rm -f $(TARGET)

help:
	@echo "make            # build"
	@echo "make clean      # clean output"
	@echo "run command:"
	@echo "  ./$(TARGET)"

.PHONY: all clean help
