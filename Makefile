CC := gcc
CFLAGS := -g -Wall
TARGET := pipe fifo socketpair uds tcp

all: $(TARGET)

pipe: pipe.c
	$(CC) $(CFLAGS) -o $@ $<

fifo: fifo.c
	$(CC) $(CFLAGS) -o $@ $<

socketpair: socketpair.c
	$(CC) $(CFLAGS) -o $@ $<

uds: uds.c
	$(CC) $(CFLAGS) -o $@ $<

tcp: tcp.c
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: test
test:
	sudo ./run_tests.sh
