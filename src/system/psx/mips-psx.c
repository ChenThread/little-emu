#include "system/psx/all.h"

#define MIPS_STATE_PARAMS struct EmuGlobal *H, struct EmuState *state
#define MIPS_STATE_ARGS H, state

#define MIPSNAME(n) psx_mips_##n

#define MIPS_ADD_CYCLES(mips, v) (mips)->H.timestamp += ((v)*11)

void MIPSNAME(fault_set)(struct MIPS *mips, MIPS_STATE_PARAMS, int cause);

extern uint32_t psx_bios_data[512<<8];

// TODO emulate this as a separate device
static void pad_update(struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp, uint32_t val)
{
	struct PSX *psx = (struct PSX *)state;
	val &= 0xFF;

	psx->joy[0].val <<= 8;
	psx->joy[0].val &= ~0xFF;
	uint32_t rval = 0xFF;
	switch(psx->joy[0].mode) {
		case PSX_JOY_MODE_UNENGAGED:
			break;
		case PSX_JOY_MODE_WAITING:
			if(val == 0x01) {
				psx->joy[0].mode = PSX_JOY_MODE_PAD_CMD;
			} else {
				psx->joy[0].mode = PSX_JOY_MODE_UNENGAGED;
			}
			break;
		case PSX_JOY_MODE_PAD_CMD:
			rval = 0x41;
			psx->joy[0].mode = PSX_JOY_MODE_BUTTON_0;
			break;
		case PSX_JOY_MODE_BUTTON_0:
			rval = 0x5A;
			psx->joy[0].mode = PSX_JOY_MODE_BUTTON_1;
			break;
		case PSX_JOY_MODE_BUTTON_1:
			rval = (uint8_t)(psx->joy[0].buttons);
			psx->joy[0].mode = PSX_JOY_MODE_BUTTON_2;
			break;
		case PSX_JOY_MODE_BUTTON_2:
			rval = (uint8_t)(psx->joy[0].buttons>>8);
			psx->joy[0].mode = PSX_JOY_MODE_UNENGAGED;
			break;
	}

	psx->joy[0].val |= (rval & 0xFF);
}

void psx_mips_mem_write(struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp, uint32_t addr, uint32_t latch, uint32_t val)
{
	struct PSX *psx = (struct PSX *)state;
	//struct PSXGlobal *G = (struct PSXGlobal *)H;

	// TODO: emulate memory control register
	if((addr&0xFFE00000) == 0x00000000) {
	//if((addr&0xFF800000) == 0x00000000) {
		//printf("W ram\n");
		MIPS_ADD_CYCLES(&(psx->mips), 6);
		uint32_t *v = &(psx->ram[(addr&0x1FFFFC)>>2]);
		*v = (*v & ~latch) | (val & latch);

	} else if((addr&0xFFFFFC00) == 0x1F800000) {
		//printf("W scratch\n");
		uint32_t *v = &(psx->scratch[(addr&0x3FC)>>2]);
		*v = (*v & ~latch) | (val & latch);

	} else if((addr&0xFFFF0000) == 0x1F800000) {
		// I/O
		switch(addr) {

			// Pad
			case 0x1F801040: // Pad TX
				//printf("Pad TX %08X\n", val);
				pad_update(H, state, timestamp, val);
				break;
			case 0x1F801048: // Pad Mode
			case 0x1F80104A: // Pad Ctrl
				//printf("Pad Mode/Ctrl %08X %08X\n", val, latch);
				if((val & 0x30000000) == 0x10000000) {
					if(psx->joy[0].mode == PSX_JOY_MODE_UNENGAGED) {
						psx->joy[0].mode = PSX_JOY_MODE_WAITING;
					}
				} else {
					psx->joy[0].mode = PSX_JOY_MODE_UNENGAGED;
				}
				psx->joy[0].pad_ctrl_mode &= ~latch;
				psx->joy[0].pad_ctrl_mode |= val & latch;
				break;
			case 0x1F80104C: // ----
			case 0x1F80104E: // Pad Baud
				printf("Pad Baud %08X %08X\n", val, latch);
				break;

			// GPU
			case 0x1F801810:
				psx_gpu_write_gp0(&(psx->gpu), H, state, timestamp, val);
				break;
			case 0x1F801814:
				psx_gpu_write_gp1(&(psx->gpu), H, state, timestamp, val);
				break;

			// SPU
			case 0x1F801DAA: // Control
			case 0x1F801DA8: // Data write
				// TODO!
				break;

			// EXP2
			case 0x1F802023: // THRA
				putchar(((val&latch)>>(8*3)));
				break;

			case 0x1F802041: { // 7-seg LED
				int dig = (val&latch&0xFF00)>>8;
				printf("7seg: %02X %08X\n", dig, psx->mips.op_pc);
			} break;

			// Unhandled
			default:
				printf("W %08X %08X %08X\n", latch, addr, val);
				break;
		}
	} else if((addr&0xFFFF0000) == 0x1FA00000) {
		// EXP1 - TODO memcontrol region
	} else {
		// TODO: trap
		printf("W TRAP %08X %08X %08X\n", latch, addr, val);
	}
}

uint32_t psx_mips_mem_read(struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp, uint32_t addr, uint32_t latch)
{
	struct PSX *psx = (struct PSX *)state;
	//struct PSXGlobal *G = (struct PSXGlobal *)H;

	// TODO: emulate memory control register
	if((addr&0xFFE00000) == 0x00000000) {
	//if((addr&0xFF800000) == 0x00000000) {
		MIPS_ADD_CYCLES(&(psx->mips), 6);
		return psx->ram[(addr&0x1FFFFC)>>2];
	} else if((addr&0xFFFFFC00) == 0x1F800000) {
		return psx->scratch[(addr&0x3FC)>>2];
	} else if(addr >= 0x1FC00000) {
		MIPS_ADD_CYCLES(&(psx->mips), 6);
		return psx_bios_data[(addr&0x07FFFC)>>2];
	} else if((addr&0xFFFF0000) == 0x1F800000) {
		// I/O
		switch(addr) {

			// Pad
			case 0x1F801040: { // Pad RX
				uint32_t ret = psx->joy[0].val;
				psx->joy[0].val >>= 8;
				psx->joy[0].val |= 0xFF000000;
				//printf("Pad RX %02X\n", ret);
				return ret;
			} break;
			case 0x1F801044: // Pad Status
				return 0x00000087;
			case 0x1F801048: // Pad Mode
			case 0x1F80104A: // Pad Ctrl
				return psx->joy[0].pad_ctrl_mode;
			case 0x1F80104C: // ----
			case 0x1F80104E: // Pad Baud
				return 0x00880000;

			// DMA
			case 0x1F8010A8: // D2
			case 0x1F8010E8: // D6
				// TODO!
				return 0x00000000;

			// GPU
			case 0x1F801810:
				return psx_gpu_read_gp0(&(psx->gpu), H, state, timestamp);
			case 0x1F801814:
				return psx_gpu_read_gp1(&(psx->gpu), H, state, timestamp);

			// SPU
			case 0x1F801DAA: // Control
				return 0x00000000; // TODO!
			case 0x1F801DAE: // Status
				return 0x00000000; // TODO!

			// EXP2
			case 0x1F802021: // SRA
				return 0x04<<(8*1); // TODO!

			// Unhandled
			default:
				printf("R %08X %08X\n", latch, addr);
				//return 0x00000000;
				return 0xFFFFFFFF;
		}
	} else if((addr&0xFF800000) == 0x1F000000) {
		// EXP1 - TODO memcontrol region
		return 0xFFFFFFFF;
	} else if((addr&0xFFE00000) == 0x1FA00000) {
		// EXP3 - TODO memcontrol region
		return 0xFFFFFFFF;
	} else {
		printf("R TRAP %08X %08X\n", latch, addr);
		psx_mips_fault_set(&(psx->mips), H, state, CAUSE_AdEL);
		return 0xFFFFFFFF;
	}

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
				//printf("[%08X]", mips->gpr[GPR_SP]);
				if((mips->gpr[GPR_SP]&0x1FFFFC00) == 0x1F800000) {
					//printf("[S]");
					arg_loc = psx->scratch;
					arg_loc += (mips->gpr[GPR_SP]&0x000003FC)>>2;
				} else {
					arg_loc = psx->ram;
					arg_loc += (mips->gpr[GPR_SP]&0x001FFFFC)>>2;

				}
				arg_loc += arg_idx;
				//printf("[%08X][%016p]", mips->gpr[GPR_SP], arg_loc);
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
					arg_idx++;
					break;
				case 'd':
				case 'i':
					printf("%d", (int)(int32_t)*arg_loc);
					arg_idx++;
					break;
				case 'u':
					printf("%u", (int)(int32_t)*arg_loc);
					arg_idx++;
					break;
				case 'x':
				case 'X':
					printf("%08X", *arg_loc);
					arg_idx++;
					break;
				default:
					printf("?!?%c", *fmt);
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

