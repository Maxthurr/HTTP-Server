CC=gcc
CFLAGS=-std=c99 -Werror -Wall -Wextra -Wvla -pedantic
SRCS=src/main.c src/config/config.c src/logger/logger.c src/server/server.c src/utils/string/string.c src/daemon/daemon.c
OBJS=$(SRCS:.c=.o)
TARGET=httpd

# Unit test settings
TEST_SRCS=tests/unit_tests/http_parser_tests.c src/utils/string/string.c src/http/parser.c src/config/config.c
TEST_TARGET=tests/http_parser_tests

httpd: $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

debug: CFLAGS += -g -fsanitize=address
debug: LDFLAGS += -fsanitize=address
debug: $(OBJS)
	$(CC) $^ -o httpd $(LDFLAGS)


check: CFLAGS += -fsanitize=address -g
check: LDLIBS += -lcriterion
check: LDFLAGS += -fsanitize=address
check: $(TEST_TARGET)
	./$(TEST_TARGET) || true

$(TEST_TARGET): $(TEST_SRCS)
	$(CC) $(CFLAGS) $^ -o $(TEST_TARGET) $(LDLIBS) -lcriterion

clean:
	$(RM) $(OBJS) $(TARGET) $(TEST_TARGET)
