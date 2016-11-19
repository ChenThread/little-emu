#include "system/sms/all.h"

#define Z80_STATE_PARAMS struct EmuGlobal *H, struct EmuState *state
#define Z80_STATE_ARGS H, state

#define Z80NAME(n) sms_z80_##n

#if 0
// OVERCLOCK
#define Z80_ADD_CYCLES(z80, v) (z80)->H.timestamp += ((v)*1)
#else
// Normal
#define Z80_ADD_CYCLES(z80, v) (z80)->H.timestamp += ((v)*3)
#endif

#define Z80_INT_CHECK ((((struct SMS *)state)->vdp.irq_out&((struct SMS *)state)->vdp.irq_mask) != 0)

void sms_z80_mem_write(struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp, uint16_t addr, uint8_t val)
{
	struct SMS *sms = (struct SMS *)state;
	struct SMSGlobal *G = (struct SMSGlobal *)H;

	if((sms->memcfg&0x10) != 0) { return; }

	if(addr >= 0xC000) {
		//printf("%p ram[%04X] = %02X\n", sms, addr&0x1FFF, val);
		sms->ram[addr&0x1FFF] = val;
	}

	if(G->rom_is_banked) {
		if(addr >= 0xFFFC) {
			// Sega mapper
			sms->paging[(addr-1)&3] = val;
		} else if((addr>>14) == 2) {
			// Codemasters mapper
			sms->paging[(addr>>14)&3] = val;
		}
	}
}

uint8_t sms_z80_mem_read(struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp, uint16_t addr)
{
	struct SMS *sms = (struct SMS *)state;
	struct SMSGlobal *G = (struct SMSGlobal *)H;

	if(addr >= 0xC000) {
		if((sms->memcfg&0x10) != 0) { return 0xFF; }
		//printf("%p %02X = ram[%04X]\n", sms, sms->ram[addr&0x1FFF], addr&0x1FFF);
		return sms->ram[addr&0x1FFF];
	} else if(addr < 0x0400 || !G->rom_is_banked) {
		//printf("%04X raw\n", addr);
		return G->rom[addr];
	} else {
		uint32_t raddr0 = (uint32_t)(addr&0x3FFF);
		uint32_t raddr1 = ((uint32_t)(sms->paging[(addr>>14)&3]))<<14;
		uint32_t raddr = raddr0|raddr1;
		return G->rom[raddr];
	}
}

static bool sms_th_pin_state(uint8_t ioctl)
{
	if((ioctl&0x02) == 0 && (ioctl&0x20) == 0 ) {
		if((ioctl&0x08) == 0 && (ioctl&0x80) == 0 ) {
			return false;
		}
	}

	return true;
}

void sms_z80_io_write(struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp, uint16_t addr, uint8_t val)
{
	struct SMS *sms = (struct SMS *)state;
	int port = ((addr>>5)&6)|(addr&1);
	//if(((addr>>8)&0xFF) == 0xBE && addr != 0xBEBE) { printf("IO WRITE %04X %d\n", addr, port); }

	switch(port)
	{
		case 0: // Memory control
			printf("!MEM! %02X\n", val);
			sms->memcfg = val;
			break;

		case 1: // I/O port control
			//printf("!IO! %02X\n", val);

			// Update latch on HT 0->1
			if((!sms_th_pin_state(sms->iocfg)) && sms_th_pin_state(val)) {
				uint32_t h = timestamp%(684ULL);
				//h = (h+684-9)%684;
				h = (h+684)%684;
				h -= 94;
				h += 3;
				h >>= 2;
				h &= 0xFF;
				sms->hlatch = h;
				//if(sms->z80.pc == 0x0C90 || sms->z80.pc == 0x0B59 || sms->z80.pc == 0x0B12)
				//printf("HC %04X = %02X\n", sms->z80.pc, h);
			}

			// Write actual thing
			sms->iocfg = val;
			break;

		case 2: // PSG / V counter
		case 3: // PSG / H counter
			sms_psg_write(&sms->psg, H, state, timestamp, val);
			break;

		case 4: // VDP data
			sms_vdp_write_data(&sms->vdp, H, state, timestamp, val);
			break;

		case 5: // VDP control
			sms_vdp_write_ctrl(&sms->vdp, H, state, timestamp, val);
			break;

		case 6: // I/O port A
			// also SDSC 0xFC control
			if((addr&0xFF) == 0xFC) {
				// TODO
			}
			break;
		case 7: // I/O port B
			// also SDSC 0xFD data
			if((addr&0xFF) == 0xFD) {
				fputc((val >= 0x20 && val <= 0x7E)
					|| val == '\n' || val == '\r' || val == '\t'
					? val : '?'
				, stdout);
				fflush(stdout);
			}
			break;

		default:
			assert(!"UNREACHABLE");
			abort();
	}
}

uint8_t sms_z80_io_read(struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp, uint16_t addr)
{
	struct SMS *sms = (struct SMS *)state;
	int port = ((addr>>5)&6)|(addr&1);

	switch(port)
	{
		case 0: // Memory control
		case 1: // I/O port control
			// These are write-only regs!
			return 0xFF;

		case 2: // V counter
			{
				// TODO: work out why there's an offset here
				uint64_t v = timestamp+18;
				v -= (94-16*2);
				v += 684ULL*(unsigned long long)SCANLINES;
				v /= 684ULL;
				v %= (unsigned long long)SCANLINES;
				v -= FRAME_START_Y;
				/*
				if(sms->z80.pc != 0x0046 && sms->z80.pc != 0x0632 && sms->z80.pc != 0x648) {
					uint32_t h = timestamp%(684ULL);
					h = (h+684)%684;
					h -= 94;
					h += 3;
					h >>= 2;
					h &= 0xFF;
					printf("VC %04X = %02X [%02X]\n"
						, sms->z80.pc
						, (uint32_t)(v&0xFF)
						, h);
				}
				*/
				return v;
			}

		case 3: // H counter
			return sms->hlatch;

		case 4: // VDP data
			return sms_vdp_read_data(&sms->vdp, H, state, timestamp);

		case 5: // VDP control
			/*
			if(sms->z80.pc != 0x003B) {
				uint32_t h = timestamp%(684ULL);
				h = (h+684)%684;
				h -= 94;
				h += 3;
				h >>= 2;
				h &= 0xFF;
				//printf("HC VDP %04X = %02X %02X\n", sms->z80.pc, h, sms->vdp.status);
			}
			*/
			return sms_vdp_read_ctrl(&sms->vdp, H, state, timestamp);

		case 6: // I/O port A
			if((sms->memcfg&0x04) != 0) { return 0xFF; }
			if(sms_hook_poll_input != NULL) {
				sms_hook_poll_input((struct SMSGlobal *)H, sms, 0, timestamp);
			}
			return sms->joy[0];

		case 7: // I/O port B
			if((sms->memcfg&0x04) != 0) { return 0xFF; }
			if(sms_hook_poll_input != NULL) {
				sms_hook_poll_input((struct SMSGlobal *)H, sms, 1, timestamp);
			}
			return sms->joy[1];

		default:
			assert(!"UNREACHABLE");
			abort();
			return 0xFF;
	}
}


#include "cpu/z80/core.c"

