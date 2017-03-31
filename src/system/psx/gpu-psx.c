#include "system/psx/all.h"

#define GPU_ADD_CYCLES(gpu, v) (gpu)->H.timestamp += ((v)*7)
#define GPU_TIMESTAMP_CAP (((struct PSX *)state)->mips.H.timestamp_end)
#define GPU_FRAME_DATA (((struct PSXGlobal *)H)->frame_data)

#define GPUNAME(n) psx_gpu_##n

#include "video/psx/core.c"

