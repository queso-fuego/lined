.POSIX:
CC ::= gcc
CFLAGS ::= -std=c23 -Wall -Wextra -Wpedantic -g3

ifeq ($(OS), Windows_NT)
CP ::= copy /y
RM ::= del /q
else
CP ::= cp -f
RM ::= rm -f
endif

all: lined

lined: lined.c
	$(CC) $(CFLAGS) -o lined lined.c
	$(CP) lined.c lined_copy.c

clean:
	$(RM) lined
