#!/bin/sh
wla-z80 -o main.o -v main.S && \
wlalink -r -v linkfile cputimer.sms && \
true

