#include "system/md/all.h"

#define M68K_STATE_PARAMS struct EmuGlobal *H, struct EmuState *state
#define M68K_STATE_ARGS H, state

#define M68KNAME(n) md_m68k_##n

#define M68K_ADD_CYCLES(m68k, v) (m68k)->H.timestamp += ((v)*1)

// TODO!
//define M68K_INT_CHECK ((((struct SMS *)state)->vdp.irq_out&((struct SMS *)state)->vdp.irq_mask) != 0)

static void md_m68k_mem_write(struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp, uint32_t addr, uint16_t val, uint16_t mask)
{
	//struct MD *md = (struct MD *)state;
	//struct MDGlobal *G = (struct MDGlobal *)H;

	// TODO: MMIO, RAM
}

static uint16_t md_m68k_mem_read(struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp, uint32_t addr)
{
	struct MD *md = (struct MD *)state;
	struct MDGlobal *G = (struct MDGlobal *)H;

	addr &= 0x00FFFFFF;
	uint32_t waddr = (addr>>1);

	if (addr <= 0x400000) {
		return ((uint16_t *)(G->rom))[waddr&0x1FFFFF];
	} else if(addr >= 0xFF0000) {
		return ((uint16_t *)(md->ram))[waddr&0x7FFF];
	} else {
		// TODO: MMIO
		return 0xFFFF;
	}
}

#include "cpu/m68k/core.c"

