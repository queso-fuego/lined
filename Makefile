.POSIX:
CC ::= gcc
CFLAGS ::= -std=c23 -Wall -Wextra -Wpedantic

all: lined
	cp lined.c lined_copy.c

clean:
	$(RM) lined
