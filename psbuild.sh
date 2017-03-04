#!/bin/sh
if [ "$1" = "" ]; then
	echo "ERROR: Please provide a pack-in game!"
	exit 1
fi

CC="gcc -std=gnu99 -O2"
if [ "$2" != "" ]; then
	CC="$2"
fi

${CC} -g -o slsms \
	-Isrc \
	-DSTATIC_VER \
	-DUSE_GLOBAL_ROM \
	-DROM_IS_PROVIDED \
	-DROM_BUFFER_SIZE=512*1024 \
	-DSMS_ROM_SIZE=512*1024 \
	-DPSG_OUT_BUF_LEN=65536 \
	"-DPROVIDED_ROM=\"$1\"" \
	\
	src/main.c \
	\
	src/system/sms/*.c \
	src/core.c \
	\
	provided_rom.S \
	\
	`sdl2-config --cflags --libs` -Wall && \
true

