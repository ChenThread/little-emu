#!/bin/sh
CC="gcc -std=gnu99 -O2"
if [ "$1" != "" ]; then
	CC="$1"
fi

${CC} -g -shared -fPIC -o libittle-emu-dedi.so \
	-Isrc \
	-Isrc/system/sms \
	\
	src/system/sms/*.c \
	src/core.c \
	\
	-DDEDI -lm -Wall && \
${CC} -g -o dedi-lemu -Isrc src/main.c -L. -Wl,-rpath,. -little-emu-dedi \
	-DDEDI -ldl -lm -Wall && \
${CC} -g -shared -fPIC -o lbots/server-net.so -Isrc bots/net.c \
	-DSERVER -DDEDI -lm -Wall && \
true

