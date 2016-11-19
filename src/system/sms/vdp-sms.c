#include "system/sms/all.h"

#define VDP_ADD_CYCLES(vdp, v) (vdp)->H.timestamp += ((v)*2)
#define VDP_TIMESTAMP_CAP (((struct SMS *)state)->z80.H.timestamp_end)
#define VDP_FRAME_DATA (((struct SMSGlobal *)H)->frame_data)

#define VDPNAME(n) sms_vdp_##n

#include "video/tms9918/core.c"

