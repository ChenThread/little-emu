#!/bin/sh
CC="gcc -std=gnu99 -O2"
if [ "$1" != "" ]; then
	CC="$1"
fi

./dbuild.sh "${CC}" && \
${CC} -g -shared -fPIC -o libittle-emu.so \
	-Isrc \
	\
	src/system/sms/*.c \
	src/core.c \
	\
	`sdl2-config --cflags --libs` -lm -Wall && \
${CC} -g -shared -fPIC -o libittle-emu-md.so \
	-Isrc \
	\
	src/system/md/*.c \
	src/core.c \
	\
	`sdl2-config --cflags --libs` -lm -Wall && \
${CC} -g -o lmd -Isrc src/main.c -L. -Wl,-rpath,. -little-emu-md \
	`sdl2-config --cflags --libs` -lm -Wall && \
${CC} -g -o lemu -Isrc src/main.c -L. -Wl,-rpath,. -little-emu \
	`sdl2-config --cflags --libs` -lm -Wall && \
${CC} -g -shared -fPIC -o lbots/s1.so -Isrc bots/s1.c \
	`sdl2-config --cflags --libs` -lm -Wall && \
${CC} -g -shared -fPIC -o lbots/s2.so -Isrc bots/s2.c \
	`sdl2-config --cflags --libs` -lm -Wall && \
${CC} -g -shared -fPIC -o lbots/tas.so -Isrc bots/tas.c \
	`sdl2-config --cflags --libs` -lm -Wall && \
${CC} -g -shared -fPIC -o lbots/corrupt.so -Isrc bots/corrupt.c \
	`sdl2-config --cflags --libs` -lm -Wall && \
${CC} -g -shared -fPIC -o lbots/net.so -Isrc bots/net.c \
	`sdl2-config --cflags --libs` -lm -Wall && \
true

