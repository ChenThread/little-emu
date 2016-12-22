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

// TODO: get the VDP in place
// (for now we're copy-pasting from the VDP we need to extend)
#if (USE_NTSC) != 0
#define SCANLINES 262
#define FRAME_START_Y 43
#define FRAME_BORDER_TOP 27
#define FRAME_BORDER_BOTTOM 24
#define FRAME_WAIT (1000000/60)
#else
#define SCANLINES 313
#define FRAME_START_Y 70
#define FRAME_BORDER_TOP 54
#define FRAME_BORDER_BOTTOM 48
#define FRAME_WAIT (1000000/50)
#endif

//define PSG_OUT_BUF_LEN (1<<24)

#include "littleemu.h"
#include "cpu/m68k/all.h"

struct MD
{
	struct EmuState H;
	uint8_t ram[65536];
	uint8_t zram[8192];
	struct M68K m68k;
	//struct VDP vdp;
	//struct PSG psg;
	uint8_t joy[4];
	bool z80_busreq;
	bool z80_busack;
} __attribute__((__packed__));

struct MDGlobal
{
	struct EmuGlobal H;
	struct MD current;

	struct EmuRamHead ram_heads[3];
	struct EmuRomHead rom_heads[2];

	// MD
	// currently not supporting SF II
	uint8_t rom[4*1024*1024];
	size_t rom_len;

	// VDP
	// TODO!
	//uint8_t frame_data[SCANLINES][342];

	// PSG
	// TODO!
	//int32_t outhpf_charge;
};

// md.c
void md_init(struct MDGlobal *G, struct MD *md);
void md_copy(struct MD *dest, struct MD *src);
void md_run(struct MDGlobal *G, struct MD *md, uint64_t timestamp);
//void md_run_frame(struct MDGlobal *G, struct MD *md);
extern void (*md_hook_poll_input)(struct MDGlobal *G, struct MD *md, int controller, uint64_t timestamp);

// m68k.c
void md_m68k_reset(struct M68K *m68k);
void md_m68k_init(struct EmuGlobal *H, struct M68K *m68k);
void md_m68k_irq(struct M68K *m68k, struct EmuGlobal *H, struct EmuState *state);
void md_m68k_run(struct M68K *m68k, struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp);

