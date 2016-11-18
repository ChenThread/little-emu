#include <stdint.h>

#ifndef USE_NTSC
#define USE_NTSC 0
#endif

#define TIME_IN_ORDER(t0, t1) (((t0) - (t1)) > ((t1) - (t0)))

#define PSG_OUT_BUF_LEN (1<<24)

#include "cpu/z80/all.h"
