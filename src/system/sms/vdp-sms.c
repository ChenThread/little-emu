#include "common.h"

#define VDP_ADD_CYCLES(vdp, v) (vdp)->timestamp += ((v)*2)

#include "video/tms9918/core.c"

