CC=gcc
FLAGS=--std=c99 -Wall --pedantic
LIBS=-lX11 -lXinerama

all: exn.c config.h
	${CC} -o exn exn.c ${FLAGS} ${LIBS}

install:
	cp exn /usr/bin/exn
