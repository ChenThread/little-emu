#include "common.h"

struct SMSGlobal sms_glob;
void (*sms_hook_poll_input)(struct SMSGlobal *G, struct SMS *sms, int controller, uint64_t timestamp) = NULL;

uint64_t time_now(void)
{
	struct timeval ts;
	gettimeofday(&ts, NULL);

	uint64_t sec = ts.tv_sec;
	uint64_t usec = ts.tv_usec;
	sec *= 1000000ULL;
	usec += sec;
	return usec;
}

void sms_z80_mem_write(struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp, uint16_t addr, uint8_t val)
{
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

uint8_t sms_z80_mem_read(struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp, uint16_t addr)
{
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

void sms_z80_io_write(struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp, uint16_t addr, uint8_t val)
{
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
			psg_write(&sms->psg, G, sms, timestamp, val);
			break;

		case 4: // VDP data
			vdp_write_data(&sms->vdp, G, sms, timestamp, val);
			break;

		case 5: // VDP control
			vdp_write_ctrl(&sms->vdp, G, sms, timestamp, val);
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

uint8_t sms_z80_io_read(struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp, uint16_t addr)
{
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
			return vdp_read_data(&sms->vdp, G, sms, timestamp);

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
			return vdp_read_ctrl(&sms->vdp, G, sms, timestamp);

		case 6: // I/O port A
			if((sms->memcfg&0x04) != 0) { return 0xFF; }
			if(sms_hook_poll_input != NULL) {
				sms_hook_poll_input(G, sms, 0, timestamp);
			}
			return sms->joy[0];

		case 7: // I/O port B
			if((sms->memcfg&0x04) != 0) { return 0xFF; }
			if(sms_hook_poll_input != NULL) {
				sms_hook_poll_input(G, sms, 1, timestamp);
			}
			return sms->joy[1];

		default:
			assert(!"UNREACHABLE");
			abort();
			return 0xFF;
	}
}

void sms_init(struct SMSGlobal *G, struct SMS *sms)
{
	*sms = (struct SMS){ .timestamp = 0, };
	sms->paging[3] = 0; // 0xFFFC
	sms->paging[0] = 0; // 0xFFFD
	sms->paging[1] = 1; // 0xFFFE
	sms->paging[2] = 2; // 0xFFFF
	sms->joy[0] = 0xFF;
	sms->joy[1] = 0xFF;
	sms->memcfg = 0xAB;
	sms->iocfg = 0xFF;
	sms->hlatch = 0x80; // TODO: find out what this is on reset
	sms_z80_init(G, &(sms->z80));
	vdp_init(G, &(sms->vdp));
	psg_init(G, &(sms->psg));
	//sms->z80.timestamp = 1;
	//sms->vdp.timestamp = 0;

	sms->no_draw = false;
}

void sms_init_global(struct SMSGlobal *G)
{
	*G = (struct SMSGlobal){ .twait = 0, };
	sms_init(G, &(G->current));
}

void sms_copy(struct SMS *dest, struct SMS *src)
{
	memcpy(dest, src, sizeof(struct SMS));
}

void sms_run(struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp)
{
	if(!TIME_IN_ORDER(sms->timestamp, timestamp)) {
		return;
	}

	//uint64_t dt = timestamp - sms->timestamp;
	while(TIME_IN_ORDER(sms->z80.timestamp_end, timestamp)) {
		sms->z80.timestamp_end = timestamp;
		vdp_estimate_line_irq(&(sms->vdp), G, sms, sms->vdp.timestamp);
		//printf("%016lX %016lX %016lX %016lX %016lX\n", timestamp, sms->z80.timestamp, sms->z80.timestamp_end, sms->vdp.timestamp, sms->vdp.timestamp_end);
		sms_z80_run(&(sms->z80), G, sms, sms->z80.timestamp_end);
		vdp_run(&(sms->vdp), G, sms, sms->z80.timestamp_end);
		psg_run(&(sms->psg), G, sms, sms->z80.timestamp_end);
	}

	sms->timestamp = timestamp;
}

void sms_run_frame(struct SMSGlobal *G, struct SMS *sms)
{
	const int pt_VINT1 = 684*(FRAME_START_Y+0xC1) + (94-18*2);
#if !USE_NTSC
	const int pt_VINT2 = 684*(FRAME_START_Y+0xE1) + (94-18*2);
	//const int pt_VINT3 = 684*(FRAME_START_Y+0xF1) + (94-18*2);
#endif

	// Run a frame
	if(sms->timestamp == 0) {
		sms->z80.timestamp = pt_VINT1;// - (pt_VINT1%684);
		sms_run(G, sms, sms->timestamp + pt_VINT1);
	}
#if USE_NTSC
	sms_run(G, sms, sms->timestamp + 684*SCANLINES-pt_VINT1);
#else
	sms_run(G, sms, sms->timestamp + pt_VINT2-pt_VINT1);
	sms_run(G, sms, sms->timestamp + 684*SCANLINES-pt_VINT2);
	// FIXME: V-centre the frame properly so this doesn't break
	//sms_run(G, sms, sms->timestamp + pt_VINT3-pt_VINT2);
	//sms_run(G, sms, sms->timestamp + 684*SCANLINES-pt_VINT3);
#endif
	sms_run(G, sms, sms->timestamp + pt_VINT1);

	//sms_copy(&sms_prev, &sms_current);
}

