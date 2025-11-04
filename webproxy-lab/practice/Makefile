CC = gcc
CFLAGS = -O0 -Wall -I . -g

TARGETS = echoclient echoserver
COMMON_OBJS = csapp.o
CLIENT_OBJS = echoclient.o
SERVER_OBJS = echoserver.o echo.o

.PHONY: all clean

all: $(TARGETS)

echoclient: $(CLIENT_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

echoserver: $(SERVER_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c csapp.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGETS) *.o
