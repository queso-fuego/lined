.POSIX:
CC ::= gcc
CFLAGS ::= -std=c23 -Wall -Wextra -Wpedantic

all: lined

lined: lined.c
	$(CC) $(CFLAGS) -o lined lined.c
	cp lined.c lined_copy.c

clean:
	$(RM) lined
