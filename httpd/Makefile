CC=gcc
CFLAGS=-std=c99 -Werror -Wall -Wextra -Wvla -pedantic
SRCS=src/main.c src/config/config.c src/logger/logger.c src/server/server.c src/utils/string/string.c src/daemon/daemon.c \
	 src/http/request_parser.c src/http/response_generator.c src/utils/file/file.c
OBJS=$(SRCS:.c=.o)
TARGET=httpd

TEST_SOURCES := $(wildcard tests/unit_tests/*.c)
TEST_SUPPORT := src/utils/string/string.c src/http/request_parser.c src/http/response_generator.c src/config/config.c
TEST_BIN_DIR := tests
TEST_BINS := $(patsubst tests/unit_tests/%.c,$(TEST_BIN_DIR)/%,$(TEST_SOURCES))

httpd: $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDLIBS)

debug: CFLAGS += -g -fsanitize=address
debug: LDFLAGS += -fsanitize=address
debug: $(OBJS)
	$(CC) $^ -o httpd $(LDFLAGS) $(LDLIBS)


check: CFLAGS += -fsanitize=address -g
check: LDLIBS += -lcriterion
check: LDFLAGS += -fsanitize=address
check: $(TEST_BINS)
	@echo "Running $(TEST_BINS)"
	@for t in $(TEST_BINS); do \
		./$$t || true; \
	done

# Pattern rule: build a test binary for each test source file
$(TEST_BIN_DIR)/%: tests/unit_tests/%.c $(TEST_SUPPORT)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS) -lcriterion

clean:
	$(RM) $(OBJS) $(TARGET) $(TEST_BINS)
