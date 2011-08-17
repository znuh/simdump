CC = gcc
LDFLAGS := $(shell pkg-config --libs libpcsclite)
CFLAGS := $(shell pkg-config --cflags libpcsclite)


all:
	$(CC) -Wall -g $(CFLAGS) -o dump main.c rs232_if.c terminal.c terminal_phoenix.c terminal_pcsc.c utils.c card.c $(LDFLAGS)

clean:
	rm -f dump *~

