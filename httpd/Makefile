CC=gcc
CFLAGS=-std=c99 -Werror -Wall -Wextra -Wvla -pedantic
SRCS=src/config/config.c src/logger/logger.c src/server/server.c src/utils/string/string.c src/daemon/daemon.c
OBJS=$(SRCS:.c=.o)
TARGET=httpd

httpd: $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

debug: CFLAGS += -g -fsanitize=address
debug: LDLIBS += -fsanitize=address
debug: tests/server_main.o $(OBJS)
	$(CC) $^ -o httpd $(LDLIBS)

clean:
	$(RM) $(OBJS) $(TARGET) tests/server_main.o
