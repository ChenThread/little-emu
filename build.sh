#!/bin/sh
clang -O2 -g -shared -fPIC -o libittlesms-dedi.so \
	\
	src/psg.c \
	src/sms.c \
	src/vdp.c \
	src/z80.c \
	\
	-DDEDI -lm -Wall && \
clang -O2 -g -o dedi-lsms src/main.c -L. -Wl,-rpath,. -littlesms-dedi \
	-DDEDI -ldl -lm -Wall && \
clang -O2 -g -shared -fPIC -o libittlesms.so \
	\
	src/psg.c \
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
clang -O2 -g -shared -fPIC -o lbots/tas.so -Isrc bots/tas.c \
	`sdl2-config --cflags --libs` -lm -Wall && \
clang -O2 -g -shared -fPIC -o lbots/corrupt.so -Isrc bots/corrupt.c \
	`sdl2-config --cflags --libs` -lm -Wall && \
clang -O2 -g -shared -fPIC -o lbots/net.so -Isrc bots/net.c \
	`sdl2-config --cflags --libs` -lm -Wall && \
true

