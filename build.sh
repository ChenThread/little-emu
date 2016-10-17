#!/bin/sh
clang -O2 -g -shared -fPIC -o libittlesms.so \
	\
	src/sms.c \
	src/vdp.c \
	src/z80.c \
	\
	`sdl2-config --cflags --libs` -lm -Wall && \
clang -O2 -g -o lsms src/main.c -L. -Wl,-rpath,. -littlesms \
	`sdl2-config --cflags --libs` -lm -Wall && \
clang -O2 -g -shared -fPIC -o lbots/s1.so -Isrc bots/s1.c \
	`sdl2-config --cflags --libs` -lm -Wall && \
clang -O2 -g -shared -fPIC -o lbots/s2.so -Isrc bots/s2.c \
	`sdl2-config --cflags --libs` -lm -Wall && \
true

