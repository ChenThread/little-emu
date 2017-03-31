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

// TODO get video working
#if USE_NTSC
#define SCANLINES 263
#define VCLKS_WIDE 3413
#else
#define SCANLINES 314
#define VCLKS_WIDE 3406
#endif

#define PIXELS_STEP (4)
#define PIXELS_WIDE ((VCLKS_WIDE)/(PIXELS_STEP))
#define FRAME_WAIT ((int)((44100*0x300*11)/(7*VCLKS_WIDE*SCANLINES)))

#include "littleemu.h"
//include "video/psx/all.h"
//include "audio/psx/all.h"
#include "cpu/psx/all.h"

struct PSXJoy
{
	uint8_t mode;
	uint8_t typ;
	uint16_t buttons;
	// TODO: inputs which aren't the digital gamepad
	// (this requires a lot of tracking)
} __attribute__((__packed__));

struct PSX
{
	struct EmuState H;
	uint32_t ram[2048<<8];
	uint32_t scratch[1024>>2];
	//uint16_t vram[1024<<9];
	//uint16_t spuram[512<<9];
	struct MIPS mips;
	//struct GPU gpu;
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
	// TODO!
	uint32_t frame_data[SCANLINES][VCLKS_WIDE];

	// SPU
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

// gpu.c
void psx_gpu_run(struct GPU *vdp, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp);
void psx_gpu_init(struct EmuGlobal *G, struct GPU *vdp);
#endif

// psx.c
void psx_mips_reset(struct MIPS *mips);
void psx_mips_init(struct EmuGlobal *H, struct MIPS *mips);
void psx_mips_run(struct MIPS *mips, struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp);

