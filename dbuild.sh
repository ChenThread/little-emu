#!/bin/sh
gcc -std=gnu99 -O2 -g -shared -fPIC -o libittlesms-dedi.so \
	\
	src/psg.c \
	src/sms.c \
	src/vdp.c \
	src/z80.c \
	\
	-DDEDI -lm -Wall && \
gcc -std=gnu99 -O2 -g -o dedi-lsms src/main.c -L. -Wl,-rpath,. -littlesms-dedi \
	-DDEDI -ldl -lm -Wall && \
gcc -std=gnu99 -O2 -g -shared -fPIC -o lbots/server-net.so -Isrc bots/net.c \
	-DSERVER -lm -Wall && \
true

