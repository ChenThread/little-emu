#!/bin/sh
gcc -std=gnu99 -O2 -g -shared -fPIC -o libittlesms-dedi.so \
	-Isrc \
	-Isrc/system/sms \
	\
	src/system/sms/*.c \
	\
	-DDEDI -lm -Wall && \
gcc -std=gnu99 -O2 -g -o dedi-lsms -Isrc src/main.c -L. -Wl,-rpath,. -littlesms-dedi \
	-DDEDI -ldl -lm -Wall && \
gcc -std=gnu99 -O2 -g -shared -fPIC -o lbots/server-net.so -Isrc bots/net.c \
	-DSERVER -DDEDI -lm -Wall && \
true

