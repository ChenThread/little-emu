#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

#ifdef DEDI
#include <dlfcn.h>
#else
#include <SDL.h>
#endif

#ifndef USE_NTSC
#define USE_NTSC 0
#endif

#include "littleemu.h"
#include "video/psx/all.h"
//include "audio/psx/all.h"
#include "cpu/psx/all.h"

enum {
	PSX_JOY_MODE_UNENGAGED = 0,
	PSX_JOY_MODE_WAITING,
	PSX_JOY_MODE_PAD_CMD,  // 0x01 0x--
	PSX_JOY_MODE_BUTTON_0, // 0x42 0x5A
	PSX_JOY_MODE_BUTTON_1, // 0x-- 0x41
	PSX_JOY_MODE_BUTTON_2, // 0x-- 0xLL
	PSX_JOY_MODE_BUTTON_3, // 0x-- 0xHH
};

struct PSXJoy
{
	uint32_t val;
	uint32_t pad_ctrl_mode;
	uint16_t buttons;
	uint8_t mode;
	// TODO: inputs which aren't the digital gamepad
	// (this requires a lot of tracking)
} __attribute__((__packed__));

struct PSX
{
	struct EmuState H;
	uint32_t ram[2048<<8];
	uint32_t scratch[1024>>2];
	struct MIPS mips;
	struct GPU gpu;
	//struct SPU spu;
	struct PSXJoy joy[2];
} __attribute__((__packed__));

struct PSXGlobal
{
	struct EmuGlobal H;
	struct PSX current;

	struct EmuRamHead ram_heads[3];
	struct EmuRomHead rom_heads[2];

	// PSX
	// not actually a ROM, this is just an exe bootstrap
	uint8_t rom[2048<<10];
	size_t rom_len;

	// GPU
	uint32_t frame_data[SCANLINES][PIXELS_WIDE];

	// SPU
	// TODO!
};

// psx.c
void psx_init(struct PSXGlobal *G, struct PSX *psx);
void psx_copy(struct PSX *dest, struct PSX *src);
void psx_run(struct PSXGlobal *G, struct PSX *psx, uint64_t timestamp);
//void psx_run_frame(struct PSXGlobal *G, struct PSX *psx);
extern void (*psx_hook_poll_input)(struct PSXGlobal *G, struct PSX *psx, int controller, uint64_t timestamp);

#if 0
// spu.c
void psx_spu_pop_16bit_stereo(int16_t *buf, size_t len);
void psx_spu_run(struct SPU *spu, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp);
void psx_spu_init(struct EmuGlobal *G, struct SPU *spu);
uint16_t psx_spu_read16(struct SPU *spu, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp, uint32_t addr);
uint32_t psx_spu_read32(struct SPU *spu, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp, uint32_t addr);
void psx_spu_write16(struct SPU *spu, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp, uint32_t addr, uint16_t val);
void psx_spu_write32(struct SPU *spu, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp, uint32_t addr, uint32_t val);
#endif

// gpu.c
void psx_gpu_run(struct GPU *gpu, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp);
void psx_gpu_init(struct EmuGlobal *G, struct GPU *gpu);
uint32_t psx_gpu_read_gp0(struct GPU *gpu, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp);
uint32_t psx_gpu_read_gp1(struct GPU *gpu, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp);
void psx_gpu_write_gp0(struct GPU *gpu, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp, uint32_t val);
void psx_gpu_write_gp1(struct GPU *gpu, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp, uint32_t val);

// psx.c
void psx_mips_reset(struct MIPS *mips);
void psx_mips_init(struct EmuGlobal *H, struct MIPS *mips);
void psx_mips_run(struct MIPS *mips, struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp);

