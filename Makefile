CC=gcc
FLAGS=--std=c99 -Wall --pedantic -O2
LIBS=-lX11 -lXinerama

all: exn.c config.h
	${CC} -o exn exn.c ${FLAGS} ${LIBS}

install:
	cp exn /usr/bin/exn

config.h:
	cp config.def.h config.h
