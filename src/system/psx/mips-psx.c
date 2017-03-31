#include "system/psx/all.h"

#define MIPS_STATE_PARAMS struct EmuGlobal *H, struct EmuState *state
#define MIPS_STATE_ARGS H, state

#define MIPSNAME(n) psx_mips_##n

#define MIPS_ADD_CYCLES(mips, v) (mips)->H.timestamp += ((v)*7)

void psx_mips_mem_write(struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp, uint32_t addr, uint32_t latch, uint32_t val)
{
	struct PSX *psx = (struct PSX *)state;
	//struct PSXGlobal *G = (struct PSXGlobal *)H;

	// TODO: emulate memory control register
	if(addr < 0x800000) {
		MIPS_ADD_CYCLES(&(psx->mips), 6);
		uint32_t *v = &(psx->ram[(addr&0x1FFFFC)>>2]);
		*v = (*v & ~latch) | (val & latch);
	} else if(addr >= 0x1F800000 && addr <= 0x1F8003FF) {
		uint32_t *v = &(psx->scratch[(addr&0x3FC)>>2]);
		*v = (*v & ~latch) | (val & latch);
	}


	// TODO: I/O
}

uint32_t psx_mips_mem_read(struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp, uint32_t addr, uint32_t latch)
{
	struct PSX *psx = (struct PSX *)state;
	//struct PSXGlobal *G = (struct PSXGlobal *)H;

	// TODO: emulate memory control register
	if(addr < 0x800000) {
		MIPS_ADD_CYCLES(&(psx->mips), 6);
		return psx->ram[(addr&0x1FFFFC)>>2];
	} else if(addr >= 0x1F800000 && addr <= 0x1F8003FF) {
		MIPS_ADD_CYCLES(&(psx->mips), 6);
		return psx->scratch[(addr&0x3FC)>>2];
	}

	// TODO: I/O

	// TODO!
	return 0xFFFFFFFF;
}

void psx_mips_printf(struct MIPS *mips, struct EmuGlobal *H, struct EmuState *state)
{
	struct PSX *psx = (struct PSX *)state;
	//struct PSXGlobal *G = (struct PSXGlobal *)H;

	//printf("[%s]\n", ((uint8_t *)psx->ram) + (mips->gpr[4]&0x001FFFFF));
	uint8_t *fmt = ((uint8_t *)psx->ram) + (mips->gpr[4]&0x001FFFFF);

	//printf("[");
	int arg_idx = 1;
	int last_arg_idx = 0;
	uint32_t *arg_loc = NULL;
	while(*fmt != '\x00') {
		if(last_arg_idx != arg_idx) {
			last_arg_idx = arg_idx;
			if(arg_idx < 4) {
				arg_loc = &(mips->gpr[4+arg_idx]);
			} else {
				arg_loc = &(psx->ram[((mips->gpr[GPR_SP]&0x001FFFFC)>>2)+arg_idx]);
			}
		}
		if(*fmt == '%') {
			fmt++;
			while(*fmt >= '0' && *fmt <= '9') {
				fmt++;
			}

			switch(*fmt) {
				case '%':
					printf("%%");
					break;
				case 's':
					printf("%s", ((uint8_t *)psx->ram) + (*arg_loc&0x001FFFFF));
					arg_loc++;
					break;
				case 'd':
				case 'i':
					printf("%d", (int)(int32_t)*arg_loc);
					arg_loc++;
					break;
				case 'x':
				case 'X':
					printf("%08X", *arg_loc);
					arg_loc++;
					break;
			}
			fmt++;
		} else {
			printf("%c", *fmt);
			fmt++;
		}
	}
	fflush(stdout);
	//printf("]\n");
}

void psx_mips_puts(struct MIPS *mips, struct EmuGlobal *H, struct EmuState *state)
{
	struct PSX *psx = (struct PSX *)state;
	//struct PSXGlobal *G = (struct PSXGlobal *)H;

	//psx_mips_printf(mips, H, state);
	printf("[%s]\n", ((uint8_t *)psx->ram) + (mips->gpr[4]&0x001FFFFF));
}

#include "cpu/psx/core.c"

