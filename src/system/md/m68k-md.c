#include "system/md/all.h"

#define M68K_STATE_PARAMS struct EmuGlobal *H, struct EmuState *state
#define M68K_STATE_ARGS H, state

#define M68KNAME(n) md_m68k_##n

#define M68K_ADD_CYCLES(m68k, v) (m68k)->H.timestamp += ((v)*1)

// TODO!
//define M68K_INT_CHECK ((((struct SMS *)state)->vdp.irq_out&((struct SMS *)state)->vdp.irq_mask) != 0)

static void md_m68k_mem_write(struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp, uint32_t addr, uint16_t val, uint16_t mask)
{
	struct MD *md = (struct MD *)state;
	//struct MDGlobal *G = (struct MDGlobal *)H;

	printf("W %08X %04X %04X\n", addr, val, mask);

	addr &= 0x00FFFFFF;
	uint32_t waddr = (addr>>1);

	if(addr >= 0xFF0000) {
		uint16_t *v = &((uint16_t *)(md->ram))[waddr&0x7FFF];
		*v &= ~mask;
		*v |= val&mask;
	} else if(addr == 0xA11100) {
		// Z80 BUSREQ
		md->z80_busreq = ((val>>8)&1) != 0;
		// TODO: emulate the Z80 and set BUSACK there
		md->z80_busack = md->z80_busreq;
	}
}

static uint16_t md_m68k_mem_read(struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp, uint32_t addr)
{
	struct MD *md = (struct MD *)state;
	struct MDGlobal *G = (struct MDGlobal *)H;

	printf("R %08X\n", addr);

	addr &= 0x00FFFFFF;
	uint32_t waddr = (addr>>1);

	if (addr <= 0x400000) {
		return ((uint16_t *)(G->rom))[waddr&0x1FFFFF];
	} else if(addr >= 0xFF0000) {
		return ((uint16_t *)(md->ram))[waddr&0x7FFF];
	} else if((addr&0xFFFFE0) == 0xA10000) {
		// Joypad (well, mostly)

		// TODO: not use static values
		uint16_t v = 0;
		switch((addr>>1)&0xF) {
			case 0x00: return 0xA0;

			case 0x01: return 0x7F;
			case 0x02: return 0x7F;
			case 0x03: return 0x7F;

			case 0x04: return 0x00;
			case 0x05: return 0x00;
			case 0x06: return 0x00;

			case 0x07: return 0xFF;
			case 0x08: return 0x00;
			case 0x09: return 0x00;

			case 0x0A: return 0xFF;
			case 0x0B: return 0x00;
			case 0x0C: return 0x00;

			case 0x0D: return 0xFF;
			case 0x0E: return 0x00;
			case 0x0F: return 0x00;
		}

		v &= 0xFF;
		v *= 0x0101;
		return v;
	} else if(addr == 0xA11100) {
		// Z80 BUSACK
		return md->z80_busack ? 0x0000 : 0x0100;
	} else {
		return 0xFFFF;
	}
}

#include "cpu/m68k/core.c"

