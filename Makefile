CC = gcc

BUILD_PATH = build

CFLAGS  = -O3 -Wall -Ilibirecovery -Isrc
LDFLAGS = -lirecovery-1.0 -lusb-1.0 -lpthread

SOURCES = \
	src/main.c \
	src/batch.c \
	src/utils.c \
	src/libbootkit/log.c \
	src/libbootkit/boot.c \
	src/libbootkit/ops.c \
	src/libbootkit/dfu.c \
	src/libbootkit/protocol.c

RESULT = $(BUILD_PATH)/checkm8_bootkit

DIR_HELPER = mkdir -p $(@D)

.PHONY: all clean

all: $(RESULT)
	@echo "%%%%% done building"

$(RESULT): $(SOURCES)
	@echo "\tbuilding checkm8_bootkit for Linux"
	@$(DIR_HELPER)
	@$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_PATH)/%.o: %.c
	@echo "\tbuilding C: $<"
	@$(DIR_HELPER)
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf $(BUILD_PATH)
	@echo "%%%%% done cleaning"
