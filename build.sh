#!/bin/sh
clang -O2 -g -o lsms \
	\
	src/sms.c \
	src/vdp.c \
	src/z80.c \
	\
	src/main.c \
	\
	`sdl2-config --cflags --libs` -lm -Wall && \
true

