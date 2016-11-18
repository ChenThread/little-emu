#!/bin/sh
gcc -std=gnu99 -O2 -g -shared -fPIC -o libittle-emu-dedi.so \
	-Isrc \
	-Isrc/system/sms \
	\
	src/system/sms/*.c \
	\
	-DDEDI -lm -Wall && \
gcc -std=gnu99 -O2 -g -o dedi-lemu -Isrc src/main.c -L. -Wl,-rpath,. -little-emu-dedi \
	-DDEDI -ldl -lm -Wall && \
gcc -std=gnu99 -O2 -g -shared -fPIC -o lbots/server-net.so -Isrc bots/net.c \
	-DSERVER -DDEDI -lm -Wall && \
true

