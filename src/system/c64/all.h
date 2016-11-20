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

#include "littleemu.h"

#include "video/vicii/all.h"
#include "cpu/6502/all.h"

struct C64
{
	struct EmuState H;
	uint8_t ram[65536];
	uint8_t cpu_io0, cpu_io1;
	struct CPU_6502 cpu;
	struct VIC vic;
} __attribute__((__packed__));

struct C64Global
{
	struct EmuGlobal H;
	struct C64 current;

	struct EmuRamHead ram_heads[1];
	struct EmuRomHead rom_heads[4];

	// C64
	uint8_t rom_basic[0x2000];
	uint8_t rom_kernal[0x2000];
	uint8_t rom_char[0x1000];
	uint8_t rom_cartridge[0x4000];
	bool rom_cartridge_present;

	// VIC
	uint8_t frame_data[SCREEN_HEIGHT * SCREEN_WIDTH];
};