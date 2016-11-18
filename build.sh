#!/bin/sh
./dbuild.sh && \
gcc -std=gnu99 -O2 -g -shared -fPIC -o libittlesms-dedi.so \
	-Isrc \
	-Isrc/system/sms \
	\
	src/system/sms/*.c \
	\
	-DDEDI -lm -Wall && \
gcc -std=gnu99 -O2 -g -o dedi-lsms -Isrc src/main.c -L. -Wl,-rpath,. -littlesms-dedi \
	-DDEDI -ldl -lm -Wall && \
gcc -std=gnu99 -O2 -g -shared -fPIC -o libittlesms.so \
	-Isrc \
	-Isrc/system/sms \
	\
	src/system/sms/*.c \
	\
	`sdl2-config --cflags --libs` -lm -Wall && \
gcc -std=gnu99 -O2 -g -o lsms -Isrc src/main.c -L. -Wl,-rpath,. -littlesms \
	`sdl2-config --cflags --libs` -lm -Wall && \
gcc -std=gnu99 -O2 -g -shared -fPIC -o lbots/s1.so -Isrc bots/s1.c \
	`sdl2-config --cflags --libs` -lm -Wall && \
gcc -std=gnu99 -O2 -g -shared -fPIC -o lbots/s2.so -Isrc bots/s2.c \
	`sdl2-config --cflags --libs` -lm -Wall && \
gcc -std=gnu99 -O2 -g -shared -fPIC -o lbots/tas.so -Isrc bots/tas.c \
	`sdl2-config --cflags --libs` -lm -Wall && \
gcc -std=gnu99 -O2 -g -shared -fPIC -o lbots/corrupt.so -Isrc bots/corrupt.c \
	`sdl2-config --cflags --libs` -lm -Wall && \
gcc -std=gnu99 -O2 -g -shared -fPIC -o lbots/net.so -Isrc bots/net.c \
	`sdl2-config --cflags --libs` -lm -Wall && \
true

