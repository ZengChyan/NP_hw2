SHELL = /bin/bash
CC = gcc
CFLAGS = -lpthread 
SRC = $(wildcard *.c)
EXE = $(patsubst %.c, %, $(SRC))

all: ${EXE}

%:	%.c
	${CC} ${CFLAGS} $@.c -o $@
clean:
	rm ${EXE}
