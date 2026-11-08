CC     = gcc
CFLAGS = -Wall -Wextra -Iinclude
LIBS   = -lsqlite3

SERVER_SRC = src/server/main.c src/server/net.c src/server/log.c \
             src/scheduler/scheduler.c src/scheduler/cron_parse.c \
             src/executor/executor.c \
             src/db/db.c src/notify/notify.c

all: schedulerd

schedulerd: $(SERVER_SRC) include/*.h
	$(CC) $(CFLAGS) -o schedulerd $(SERVER_SRC) $(LIBS)

clean:
	rm -f schedulerd

.PHONY: all clean
