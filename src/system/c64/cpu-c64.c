#include "system/c64/all.h"

#define CPU_MEMORY_PARAMS struct C64Global *H, struct C64 *Hstate
#define CPU_MEMORY_ARGS H, Hstate
#define CPU_6502_ENABLE_BCD

#define ROM_KERNAL_ENABLED (Hstate->cpu_io0 & 0x02)
#define ROM_BASIC_ENABLED (Hstate->cpu_io0 & 0x01)
#define ROM_CHAR_ENABLED (!(Hstate->cpu_io0 & 0x04))

uint8_t cpu_6502_read_mem(struct C64Global *H, struct C64 *Hstate, uint16_t addr);

#include "video/vicii/core.c"
#define CPU_ADD_CYCLES_EXTRA(H, Hstate, state, c) { vic_run(&(Hstate->vic), H, Hstate, Hstate->H.timestamp); }

uint8_t cpu_6502_read_mem(struct C64Global *H, struct C64 *Hstate, uint16_t addr) {
	if (addr == 0x0000)
		return Hstate->cpu_io0;
	if (addr == 0x0001)
		return Hstate->cpu_io1;
	if (ROM_BASIC_ENABLED && addr >= 0xA000 && addr <= 0xBFFF)
		return H->rom_basic[addr - 0xA000];
	if (ROM_KERNAL_ENABLED && addr >= 0xE000)
		return H->rom_kernal[addr - 0xE000];
	if (addr >= 0xD000 && addr <= 0xDFFF) {
		if (ROM_CHAR_ENABLED)
			return H->rom_char[addr - 0xD000];
		else {
			if (addr <= 0xD3FF)
				return vic_read_mem(H, Hstate, addr);
		}
	}

	return Hstate->ram[addr];
}

void cpu_6502_write_mem(struct C64Global *H, struct C64 *Hstate, uint16_t addr, uint8_t value) {
	if (addr == 0x0000)
		Hstate->cpu_io0 = 0xC0 | (value & 0x3F);
	else if (addr == 0x0001)
		Hstate->cpu_io1 = 0xC0 | (Hstate->cpu_io1 & 0x30) | (value & 0x0F);
	else if (addr >= 0xD000 && addr <= 0xD3FF)
		vic_write_mem(H, Hstate, addr, value);
	else
		Hstate->ram[addr] = value;
}

#include "cpu/6502/core.c"
