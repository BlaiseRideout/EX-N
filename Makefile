CC=gcc
FLAGS=--std=c99 -Wall --pedantic
LIBS=

all:
	${CC} -o exn exn.c ${FLAGS} ${LIBS}
