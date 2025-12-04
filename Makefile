# Project configuration
PROJECT_DIR := server
TARGET := http-server
CC := gcc
CFLAGS := -std=c99 -Werror -Wall -Wextra -Wvla -pedantic

# Source files
SRC_DIR := $(PROJECT_DIR)/src
SRCS := $(wildcard $(SRC_DIR)/*.c $(SRC_DIR)/*/*.c $(SRC_DIR)/*/*/*.c)
OBJS := $(SRCS:.c=.o)

# Test configuration
TEST_DIR := $(PROJECT_DIR)/tests
TEST_UNIT_DIR := $(TEST_DIR)/unit_tests
TEST_SOURCES := $(wildcard $(TEST_UNIT_DIR)/*.c)
TEST_SUPPORT := $(SRC_DIR)/utils/string/string.c $(SRC_DIR)/http/request_parser.c \
                $(SRC_DIR)/http/response_generator.c $(SRC_DIR)/config/config.c
TEST_BINS := $(patsubst $(TEST_UNIT_DIR)/%.c,$(TEST_DIR)/%,$(TEST_SOURCES))

# Targets
.PHONY: all debug check clean

all: $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDLIBS) $(LDFLAGS)

debug: CFLAGS += -g -fsanitize=address
debug: LDFLAGS += -fsanitize=address
debug: all

check: CFLAGS += -fsanitize=address -g
check: LDLIBS += -lcriterion
check: LDFLAGS += -fsanitize=address
check: $(TEST_BINS)
	@for t in $(TEST_BINS); do ./$$t || true; done

$(TEST_DIR)/%: $(TEST_UNIT_DIR)/%.c $(TEST_SUPPORT)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS) -lcriterion

clean:
	$(RM) $(OBJS) $(TARGET) $(TEST_BINS)
