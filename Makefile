
all:
	gcc -Wall -g -o dump main.c rs232_if.c terminal.c terminal_phoenix.c utils.c card.c

clean:
	rm -f dump *~

