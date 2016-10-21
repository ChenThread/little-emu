#include "common.h"

void (*sms_hook_poll_input)(struct SMS *sms, int controller, uint64_t timestamp) = NULL;

static void z80_mem_write(struct SMS *sms, uint64_t timestamp, uint16_t addr, uint8_t val)
{
	if((sms->memcfg&0x10) != 0) { return; }

	if(addr >= 0xC000) {
		//printf("%p ram[%04X] = %02X\n", sms, addr&0x1FFF, val);
		sms->ram[addr&0x1FFF] = val;
	}

	if(addr >= 0xFFFC && sms_rom_is_banked) {
		sms->paging[(addr-1)&3] = val;
	}
}

static uint8_t z80_mem_read(struct SMS *sms, uint64_t timestamp, uint16_t addr)
{
	if(addr >= 0xC000) {
		if((sms->memcfg&0x10) != 0) { return 0xFF; }
		//printf("%p %02X = ram[%04X]\n", sms, sms->ram[addr&0x1FFF], addr&0x1FFF);
		return sms->ram[addr&0x1FFF];
	} else if(addr < 0x0400 || !sms_rom_is_banked) {
		//printf("%04X raw\n", addr);
		return sms_rom[addr];
	} else {
		uint32_t raddr0 = (uint32_t)(addr&0x3FFF);
		uint32_t raddr1 = ((uint32_t)(sms->paging[(addr>>14)&3]))<<14;
		uint32_t raddr = raddr0|raddr1;
		return sms_rom[raddr];
	}
}

static bool th_pin_state(uint8_t ioctl)
{
	if((ioctl&0x02) == 0 && (ioctl&0x20) == 0 ) {
		if((ioctl&0x08) == 0 && (ioctl&0x80) == 0 ) {
			return false;
		}
	}

	return true;
}

static void z80_io_write(struct SMS *sms, uint64_t timestamp, uint16_t addr, uint8_t val)
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
			if((!th_pin_state(sms->iocfg)) && th_pin_state(val)) {
				uint32_t h = timestamp%(684ULL);
				//h = (h+684-9)%684;
				h = (h+684)%684;
				h -= 94;
				h += 3;
				h >>= 2;
				h &= 0xFF;
				sms->hlatch = h;
				//if(sms->z80.pc == 0x0C90 || sms->z80.pc == 0x0B59)
				//printf("HC %04X = %02X\n", sms->z80.pc, h);
			}

			// Write actual thing
			sms->iocfg = val;
			break;

		case 2: // PSG / V counter
		case 3: // PSG / H counter
			// TODO!
			break;

		case 4: // VDP data
			vdp_write_data(&sms->vdp, sms, timestamp, val);
			break;

		case 5: // VDP control
			vdp_write_ctrl(&sms->vdp, sms, timestamp, val);
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

static uint8_t z80_io_read(struct SMS *sms, uint64_t timestamp, uint16_t addr)
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
			return vdp_read_data(&sms->vdp, sms, timestamp);

		case 5: // VDP control
			/*
			if(sms->z80.pc != 0x003B) {
				uint32_t h = timestamp%(684ULL);
				h = (h+684)%684;
				h -= 94;
				h += 2;
				h >>= 2;
				h &= 0xFF;
				//printf("HC VDP %04X = %02X %02X\n", sms->z80.pc, h, sms->vdp.status);
			}
			*/
			return vdp_read_ctrl(&sms->vdp, sms, timestamp);

		case 6: // I/O port A
			if((sms->memcfg&0x04) != 0) { return 0xFF; }
			if(sms_hook_poll_input != NULL) {
				sms_hook_poll_input(sms, 0, timestamp);
			}
			return sms->joy[0];

		case 7: // I/O port B
			if((sms->memcfg&0x04) != 0) { return 0xFF; }
			if(sms_hook_poll_input != NULL) {
				sms_hook_poll_input(sms, 1, timestamp);
			}
			return sms->joy[1];

		default:
			assert(!"UNREACHABLE");
			abort();
			return 0xFF;
	}
}

void z80_reset(struct Z80 *z80)
{
	z80->iff1 = 0;
	z80->iff2 = 0;
	z80->pc = 0;
	z80->i = 0;
	z80->r = 0;
	z80->sp = 0xFFFF;
	z80->gpr[RF] = 0xFF;
	z80->gpr[RA] = 0xFF;
	z80->halted = 0;
	z80->im = 0;
	Z80_ADD_CYCLES(z80, 3);
}

uint16_t z80_pair(uint8_t h, uint8_t l)
{
	return (((uint16_t)h)<<8) + ((uint16_t)l);
}

uint16_t z80_pair_pbe(uint8_t *p)
{
	return z80_pair(p[0], p[1]);
}

static uint8_t z80_fetch_op_m1(struct Z80 *z80, struct SMS *sms)
{
	uint8_t op = z80_mem_read(sms, z80->timestamp, z80->pc++);
	Z80_ADD_CYCLES(z80, 4);
	z80->r = (z80->r&0x80) + ((z80->r+1)&0x7F); // TODO: confirm
	return op;
}

static uint8_t z80_fetch_op_x(struct Z80 *z80, struct SMS *sms)
{
	uint8_t op = z80_mem_read(sms, z80->timestamp, z80->pc++);
	Z80_ADD_CYCLES(z80, 3);
	return op;
}

static uint16_t z80_fetch_ix_d(struct Z80 *z80, struct SMS *sms, int ix)
{
	uint16_t addr = z80_pair_pbe(z80->idx[ix&1]);
	addr += (uint16_t)(int16_t)(int8_t)z80_fetch_op_x(z80, sms);
	Z80_ADD_CYCLES(z80, 5);
	z80->wz[0] = (uint8_t)(addr>>8);
	return addr;
}

static uint8_t z80_parity(uint8_t r)
{
	r ^= (r>>4);
	r ^= (r>>2);
	r ^= (r>>1);
	r = ~r;
	return (r<<2)&0x04;
}

static uint8_t z80_inc8(struct Z80 *z80, uint8_t a)
{
	uint8_t r = a+1;
	uint8_t h = ((a&0xF)+1)&0x10;
	uint8_t z = (r == 0 ? 0x40 : 0x00);
	uint8_t v = (r == 0x80 ? 0x04 : 0x00);
	z80->gpr[RF] = (r&0xA8) | (z80->gpr[RF]&0x01) | h | v | z;
	return r;
}

static uint8_t z80_dec8(struct Z80 *z80, uint8_t a)
{
	uint8_t r = a-1;
	uint8_t h = ((a&0xF)-1)&0x10;
	uint8_t z = (r == 0 ? 0x40 : 0x00);
	uint8_t v = (r == 0x7F ? 0x04 : 0x00);
	z80->gpr[RF] = (r&0xA8) | (z80->gpr[RF]&0x01) | h | v | z | 0x02;
	return r;
}

static uint8_t z80_add8(struct Z80 *z80, uint8_t a, uint8_t b)
{
	uint8_t r = a+b;
	uint8_t h = ((a&0xF)+(b&0xF))&0x10;
	uint8_t c = (r < a ? 0x01 : 0x00);
	uint8_t z = (r == 0 ? 0x40 : 0x00);
	// Sa == Sb && Sa != Sr
	uint8_t v = (((a^b)&0x80) == 0 && ((a^r)&0x80) != 0) ? 0x04 : 0x00;
	z80->gpr[RF] = (r&0xA8) | h | c | v | z;
	return r;
}

static uint8_t z80_adc8(struct Z80 *z80, uint8_t a, uint8_t b)
{
	uint8_t rc = (z80->gpr[RF]&0x01);
	uint8_t r = a+b+rc;
	uint8_t h = ((a&0xF)+(b&0xF)+rc)&0x10;
	uint8_t c = (r < a || (rc == 1 && r == a) ? 0x01 : 0x00);
	uint8_t z = (r == 0 ? 0x40 : 0x00);
	// Sa == Sb && Sa != Sr
	uint8_t v = (((a^b)&0x80) == 0 && ((a^r)&0x80) != 0) ? 0x04 : 0x00;
	z80->gpr[RF] = (r&0xA8) | h | c | v | z;
	return r;
}

static uint8_t z80_sub8(struct Z80 *z80, uint8_t a, uint8_t b)
{
	uint8_t r = a-b;
	uint8_t h = ((a&0xF)-(b&0xF))&0x10;
	uint8_t c = (r > a ? 0x01 : 0x00);
	uint8_t z = (r == 0 ? 0x40 : 0x00);
	// Sa != Sb && Sa != Sr
	uint8_t v = (((a^b)&0x80) != 0 && ((a^r)&0x80) != 0) ? 0x04 : 0x00;
	z80->gpr[RF] = (r&0xA8) | h | c | v | z | 0x02;
	return r;
}

static uint8_t z80_sbc8(struct Z80 *z80, uint8_t a, uint8_t b)
{
	uint8_t rc = (z80->gpr[RF]&0x01);
	uint8_t r = a-b-rc;
	uint8_t h = ((a&0xF)-(b&0xF)-rc)&0x10;
	uint8_t c = (r > a || (rc == 1 && r == a) ? 0x01 : 0x00);
	uint8_t z = (r == 0 ? 0x40 : 0x00);
	// Sa != Sb && Sa != Sr
	uint8_t v = (((a^b)&0x80) != 0 && ((a^r)&0x80) != 0) ? 0x04 : 0x00;
	z80->gpr[RF] = (r&0xA8) | h | c | v | z | 0x02;
	return r;
}

static uint8_t z80_and8(struct Z80 *z80, uint8_t a, uint8_t b)
{
	uint8_t r = a&b;
	uint8_t z = (r == 0 ? 0x40 : 0x00);
	uint8_t p = z80_parity(r);
	z80->gpr[RF] = (r&0xA8) | p | z | 0x10;
	return r;
}

static uint8_t z80_xor8(struct Z80 *z80, uint8_t a, uint8_t b)
{
	uint8_t r = a^b;
	uint8_t z = (r == 0 ? 0x40 : 0x00);
	uint8_t p = z80_parity(r);
	z80->gpr[RF] = (r&0xA8) | p | z | 0x00;
	return r;
}

static uint8_t z80_or8(struct Z80 *z80, uint8_t a, uint8_t b)
{
	uint8_t r = a|b;
	uint8_t z = (r == 0 ? 0x40 : 0x00);
	uint8_t p = z80_parity(r);
	z80->gpr[RF] = (r&0xA8) | p | z | 0x00;
	return r;
}

static uint16_t z80_add16(struct Z80 *z80, uint16_t a, uint16_t b)
{
	z80->wz[0] = (uint8_t)(a>>8);
	uint16_t r = a+b;
	uint8_t h = (((a&0xFFF)+(b&0xFFF))&0x1000)>>8;
	uint8_t c = (r < a ? 0x01 : 0x00);
	z80->gpr[RF] = (z80->gpr[RF]&0xC4)
		| ((r>>8)&0x28) | h | c;
	return r;
}

static uint16_t z80_adc16(struct Z80 *z80, uint16_t a, uint16_t b)
{
	uint16_t rc = (uint16_t)(z80->gpr[RF]&0x01);
	uint16_t r = a+b+rc;
	uint8_t h = (((a&0xFFF)+(b&0xFFF)+rc)&0x1000)>>8;
	uint8_t c = (r < a || (rc == 1 && r == a) ? 0x01 : 0x00);
	uint8_t z = (r == 0 ? 0x40 : 0x00);
	// Sa == Sb && Sa != Sr
	uint8_t v = (((a^b)&0x8000) == 0 && ((a^r)&0x8000) != 0) ? 0x04 : 0x00;
	z80->gpr[RF] = ((r>>8)&0xA8) | h | c | v | z;
	return r;
}

static uint16_t z80_sbc16(struct Z80 *z80, uint16_t a, uint16_t b)
{
	uint16_t rc = (uint16_t)(z80->gpr[RF]&0x01);
	uint16_t r = a-b-rc;
	uint8_t h = (((a&0xFFF)-(b&0xFFF)-rc)&0x1000)>>8;
	uint8_t c = (r > a || (rc == 1 && r == a) ? 0x01 : 0x00);
	uint8_t z = (r == 0 ? 0x40 : 0x00);
	// Sa != Sb && Sa != Sr
	uint8_t v = (((a^b)&0x8000) != 0 && ((a^r)&0x8000) != 0) ? 0x04 : 0x00;
	z80->gpr[RF] = ((r>>8)&0xA8) | h | c | v | z | 0x02;
	return r;
}

static void z80_op_jr_cond(struct Z80 *z80, struct SMS *sms, bool cond)
{
	uint16_t offs = (uint16_t)(int16_t)(int8_t)z80_fetch_op_x(z80, sms);
	z80->wz[0] = (uint8_t)(offs>>8);
	if(cond) {
		z80->pc += offs;
		Z80_ADD_CYCLES(z80, 5);
	}
}

static void z80_op_jp_cond(struct Z80 *z80, struct SMS *sms, bool cond)
{
	uint16_t pcl = (uint16_t)z80_fetch_op_x(z80, sms);
	uint16_t pch = (uint16_t)z80_fetch_op_x(z80, sms);
	if(cond) {
		z80->pc = pcl+(pch<<8);
	}
}

static void z80_op_ret(struct Z80 *z80, struct SMS *sms)
{
	uint8_t pcl = z80_mem_read(sms, z80->timestamp, z80->sp++);
	Z80_ADD_CYCLES(z80, 3);
	uint8_t pch = z80_mem_read(sms, z80->timestamp, z80->sp++);
	Z80_ADD_CYCLES(z80, 3);
	//printf("RET %04X [%04X]\n", z80->pc, z80->sp);
	z80->pc = z80_pair(pch, pcl);
	//printf("->  %04X\n", z80->pc);
}

static void z80_op_ret_cond(struct Z80 *z80, struct SMS *sms, bool cond)
{
	Z80_ADD_CYCLES(z80, 1);
	if(cond) {
		z80_op_ret(z80, sms);
	}
}

static void z80_op_call_cond(struct Z80 *z80, struct SMS *sms, bool cond)
{
	uint16_t pcl = (uint16_t)z80_fetch_op_x(z80, sms);
	uint16_t pch = (uint16_t)z80_fetch_op_x(z80, sms);
	if(cond) {
		z80_mem_write(sms, z80->timestamp, --z80->sp, (uint8_t)(z80->pc>>8));
		Z80_ADD_CYCLES(z80, 4);
		z80_mem_write(sms, z80->timestamp, --z80->sp, (uint8_t)(z80->pc>>0));
		Z80_ADD_CYCLES(z80, 3);
		z80->pc = pcl+(pch<<8);
	}
}

static void z80_op_rst(struct Z80 *z80, struct SMS *sms, uint16_t addr)
{
	z80_mem_write(sms, z80->timestamp, --z80->sp, (uint8_t)(z80->pc>>8));
	Z80_ADD_CYCLES(z80, 4);
	z80_mem_write(sms, z80->timestamp, --z80->sp, (uint8_t)(z80->pc>>0));
	Z80_ADD_CYCLES(z80, 3);
	z80->pc = addr;
}

void z80_irq(struct Z80 *z80, struct SMS *sms, uint8_t dat)
{
	if(z80->iff1 == 0) {
		return;
	}

	/*
	printf("IRQ %d %d\n"
		, (int32_t)(z80->timestamp%684)
		, (int32_t)((z80->timestamp%(684*SCANLINES))/684)
		);
	*/

	// TODO: fetch op using dat
	assert(z80->im == 1);

	z80->iff1 = 0;
	z80->iff2 = 0;
	Z80_ADD_CYCLES(z80, 7);
	z80->r = (z80->r&0x80) + ((z80->r+1)&0x7F); // TODO: confirm
	z80_mem_write(sms, z80->timestamp, --z80->sp, (uint8_t)(z80->pc>>8));
	Z80_ADD_CYCLES(z80, 3);
	z80_mem_write(sms, z80->timestamp, --z80->sp, (uint8_t)(z80->pc>>0));
	Z80_ADD_CYCLES(z80, 3);
	z80->pc = 0x0038;
	z80->halted = false;
}

void z80_nmi(struct Z80 *z80, struct SMS *sms)
{
	z80->iff1 = 0;
	Z80_ADD_CYCLES(z80, 5);
	z80->r = (z80->r&0x80) + ((z80->r+1)&0x7F); // TODO: confirm
	z80_mem_write(sms, z80->timestamp, --z80->sp, (uint8_t)(z80->pc>>8));
	Z80_ADD_CYCLES(z80, 3);
	z80_mem_write(sms, z80->timestamp, --z80->sp, (uint8_t)(z80->pc>>0));
	Z80_ADD_CYCLES(z80, 3);
	z80->pc = 0x0066;
	z80->halted = false;
}

void z80_run(struct Z80 *z80, struct SMS *sms, uint64_t timestamp)
{
	// Don't jump back into the past
	if(!TIME_IN_ORDER(z80->timestamp, timestamp)) {
		return;
	}

	// If halted, don't waste time fetching ops
	if(z80->halted) {
		if(z80->iff1 != 0 && (sms->vdp.irq_out&sms->vdp.irq_mask) != 0) {
			/*
			printf("IN_IRQ HALT2 %d %d %02X %02X %02X %016llX\n"
				, z80->noni
				, z80->iff1
				, sms->vdp.irq_out
				, sms->vdp.irq_mask
				, sms->vdp.status
				, (unsigned long long)z80->timestamp
				);
			*/

			z80_irq(z80, sms, 0xFF);
		} else {
			while(TIME_IN_ORDER(z80->timestamp, timestamp)) {
				Z80_ADD_CYCLES(z80, 4);
				z80->r = (z80->r&0x80) + ((z80->r+1)&0x7F);
			}
			//z80->timestamp = timestamp;
			return;
		}
	}

	// Run ops
	uint64_t lstamp = z80->timestamp;
	z80->timestamp_end = timestamp;
	while(z80->timestamp < z80->timestamp_end) {
		//if(false && z80->pc != 0x215A) {
		if(false) {
			printf("%020lld: %04X: %02X: A=%02X SP=%04X\n"
				, (unsigned long long)((z80->timestamp-lstamp)/(uint64_t)3)
				, z80->pc, z80->gpr[RF], z80->gpr[RA], z80->sp);
		}

		// Check for IRQ
		if(z80->noni == 0 && z80->iff1 != 0 && (sms->vdp.irq_out&sms->vdp.irq_mask) != 0) {
			/*
			printf("IN_IRQ %d %d %02X %02X %02X %016llX\n"
				, z80->noni
				, z80->iff1
				, sms->vdp.irq_out
				, sms->vdp.irq_mask
				, sms->vdp.status
				, (unsigned long long)z80->timestamp
				);
			*/

			z80_irq(z80, sms, 0xFF);
		}
		z80->noni = 0;

		lstamp = z80->timestamp;
		// Fetch
		int ix = -1;
		uint8_t op;

		for(;;) {
			op = z80_fetch_op_m1(z80, sms);

			if((op|0x20) == 0xFD) {
				ix = (op&0x20)>>5;
			} else {
				break;
			}
		}

		if(op == 0xED) {
			op = z80_fetch_op_m1(z80, sms);
			//printf("*ED %04X %02X %04X\n", z80->pc-2, op, z80->sp);
			// Decode
			switch(op) {
				//
				// X=1
				//

				// Z=0
				case 0x40: // IN B, (C)
					z80->gpr[RB] = z80_io_read(sms, z80->timestamp,
						z80_pair_pbe(&z80->gpr[RB]));
					z80->gpr[RF] = (z80->gpr[RF]&0x01)
						| (z80->gpr[RB]&0xA8)
						| z80_parity(z80->gpr[RB])
						| (z80->gpr[RB] == 0 ? 0x40 : 0x00);
					Z80_ADD_CYCLES(z80, 4);
					break;
				case 0x48: // IN C, (C)
					z80->gpr[RC] = z80_io_read(sms, z80->timestamp,
						z80_pair_pbe(&z80->gpr[RB]));
					z80->gpr[RF] = (z80->gpr[RF]&0x01)
						| (z80->gpr[RC]&0xA8)
						| z80_parity(z80->gpr[RC])
						| (z80->gpr[RC] == 0 ? 0x40 : 0x00);
					Z80_ADD_CYCLES(z80, 4);
					break;
				case 0x50: // IN D, (C)
					z80->gpr[RD] = z80_io_read(sms, z80->timestamp,
						z80_pair_pbe(&z80->gpr[RB]));
					z80->gpr[RF] = (z80->gpr[RF]&0x01)
						| (z80->gpr[RD]&0xA8)
						| z80_parity(z80->gpr[RD])
						| (z80->gpr[RD] == 0 ? 0x40 : 0x00);
					Z80_ADD_CYCLES(z80, 4);
					break;
				case 0x58: // IN E, (C)
					z80->gpr[RE] = z80_io_read(sms, z80->timestamp,
						z80_pair_pbe(&z80->gpr[RB]));
					z80->gpr[RF] = (z80->gpr[RF]&0x01)
						| (z80->gpr[RE]&0xA8)
						| z80_parity(z80->gpr[RE])
						| (z80->gpr[RE] == 0 ? 0x40 : 0x00);
					Z80_ADD_CYCLES(z80, 4);
					break;
				case 0x60: // IN H, (C)
					z80->gpr[RH] = z80_io_read(sms, z80->timestamp,
						z80_pair_pbe(&z80->gpr[RB]));
					z80->gpr[RF] = (z80->gpr[RF]&0x01)
						| (z80->gpr[RH]&0xA8)
						| z80_parity(z80->gpr[RH])
						| (z80->gpr[RH] == 0 ? 0x40 : 0x00);
					Z80_ADD_CYCLES(z80, 4);
					break;
				case 0x68: // IN L, (C)
					z80->gpr[RL] = z80_io_read(sms, z80->timestamp,
						z80_pair_pbe(&z80->gpr[RB]));
					z80->gpr[RF] = (z80->gpr[RF]&0x01)
						| (z80->gpr[RL]&0xA8)
						| z80_parity(z80->gpr[RL])
						| (z80->gpr[RL] == 0 ? 0x40 : 0x00);
					Z80_ADD_CYCLES(z80, 4);
					break;
				case 0x70: { // IN (C)
					uint8_t tmp = z80_io_read(sms, z80->timestamp,
						z80_pair_pbe(&z80->gpr[RB]));
					z80->gpr[RF] = (z80->gpr[RF]&0x01)
						| (tmp&0xA8)
						| z80_parity(tmp)
						| (tmp == 0 ? 0x40 : 0x00);
					Z80_ADD_CYCLES(z80, 4);
				} break;
				case 0x78: // IN A, (C)
					z80->gpr[RA] = z80_io_read(sms, z80->timestamp,
						z80_pair_pbe(&z80->gpr[RB]));
					z80->gpr[RF] = (z80->gpr[RF]&0x01)
						| (z80->gpr[RA]&0xA8)
						| z80_parity(z80->gpr[RA])
						| (z80->gpr[RA] == 0 ? 0x40 : 0x00);
					Z80_ADD_CYCLES(z80, 4);
					break;

				// Z=1
				case 0x41: // OUT (C), B
					z80_io_write(sms, z80->timestamp,
						z80_pair_pbe(&z80->gpr[RB]),
						z80->gpr[RB]);
					Z80_ADD_CYCLES(z80, 4);
					break;
				case 0x49: // OUT (C), C
					z80_io_write(sms, z80->timestamp,
						z80_pair_pbe(&z80->gpr[RB]),
						z80->gpr[RC]);
					Z80_ADD_CYCLES(z80, 4);
					break;
				case 0x51: // OUT (C), D
					z80_io_write(sms, z80->timestamp,
						z80_pair_pbe(&z80->gpr[RB]),
						z80->gpr[RD]);
					Z80_ADD_CYCLES(z80, 4);
					break;
				case 0x59: // OUT (C), E
					z80_io_write(sms, z80->timestamp,
						z80_pair_pbe(&z80->gpr[RB]),
						z80->gpr[RE]);
					Z80_ADD_CYCLES(z80, 4);
					break;
				case 0x61: // OUT (C), H
					z80_io_write(sms, z80->timestamp,
						z80_pair_pbe(&z80->gpr[RB]),
						z80->gpr[RH]);
					Z80_ADD_CYCLES(z80, 4);
					break;
				case 0x69: // OUT (C), L
					z80_io_write(sms, z80->timestamp,
						z80_pair_pbe(&z80->gpr[RB]),
						z80->gpr[RL]);
					Z80_ADD_CYCLES(z80, 4);
					break;
				case 0x71: // OUT (C), 0
					z80_io_write(sms, z80->timestamp,
						z80_pair_pbe(&z80->gpr[RB]),
						0);
					Z80_ADD_CYCLES(z80, 4);
					break;
				case 0x79: // OUT (C), A
					z80_io_write(sms, z80->timestamp,
						z80_pair_pbe(&z80->gpr[RB]),
						z80->gpr[RA]);
					Z80_ADD_CYCLES(z80, 4);
					break;

				// Z=2
				case 0x42: { // SBC HL, BC
					uint16_t hl = z80_pair_pbe(&z80->gpr[RH]);
					uint16_t q = z80_pair_pbe(&z80->gpr[RB]);
					hl = z80_sbc16(z80, hl, q);
					z80->gpr[RH] = (uint8_t)(hl>>8);
					z80->gpr[RL] = (uint8_t)(hl>>0);
					Z80_ADD_CYCLES(z80, 7);
				} break;
				case 0x52: { // SBC HL, DE
					uint16_t hl = z80_pair_pbe(&z80->gpr[RH]);
					uint16_t q = z80_pair_pbe(&z80->gpr[RD]);
					hl = z80_sbc16(z80, hl, q);
					z80->gpr[RH] = (uint8_t)(hl>>8);
					z80->gpr[RL] = (uint8_t)(hl>>0);
					Z80_ADD_CYCLES(z80, 7);
				} break;
				case 0x62: { // SBC HL, HL
					uint16_t hl = z80_pair_pbe(&z80->gpr[RH]);
					uint16_t q = z80_pair_pbe(&z80->gpr[RH]);
					hl = z80_sbc16(z80, hl, q);
					z80->gpr[RH] = (uint8_t)(hl>>8);
					z80->gpr[RL] = (uint8_t)(hl>>0);
					Z80_ADD_CYCLES(z80, 7);
				} break;
				case 0x72: { // SBC HL, SP
					uint16_t hl = z80_pair_pbe(&z80->gpr[RH]);
					uint16_t q = z80->sp;
					hl = z80_sbc16(z80, hl, q);
					z80->gpr[RH] = (uint8_t)(hl>>8);
					z80->gpr[RL] = (uint8_t)(hl>>0);
					Z80_ADD_CYCLES(z80, 7);
				} break;

				case 0x4A: { // ADC HL, BC
					uint16_t hl = z80_pair_pbe(&z80->gpr[RH]);
					uint16_t q = z80_pair_pbe(&z80->gpr[RB]);
					hl = z80_adc16(z80, hl, q);
					z80->gpr[RH] = (uint8_t)(hl>>8);
					z80->gpr[RL] = (uint8_t)(hl>>0);
					Z80_ADD_CYCLES(z80, 7);
				} break;
				case 0x5A: { // ADC HL, DE
					uint16_t hl = z80_pair_pbe(&z80->gpr[RH]);
					uint16_t q = z80_pair_pbe(&z80->gpr[RD]);
					hl = z80_adc16(z80, hl, q);
					z80->gpr[RH] = (uint8_t)(hl>>8);
					z80->gpr[RL] = (uint8_t)(hl>>0);
					Z80_ADD_CYCLES(z80, 7);
				} break;
				case 0x6A: { // ADC HL, HL
					uint16_t hl = z80_pair_pbe(&z80->gpr[RH]);
					uint16_t q = z80_pair_pbe(&z80->gpr[RH]);
					hl = z80_adc16(z80, hl, q);
					z80->gpr[RH] = (uint8_t)(hl>>8);
					z80->gpr[RL] = (uint8_t)(hl>>0);
					Z80_ADD_CYCLES(z80, 7);
				} break;
				case 0x7A: { // ADC HL, SP
					uint16_t hl = z80_pair_pbe(&z80->gpr[RH]);
					uint16_t q = z80->sp;
					hl = z80_adc16(z80, hl, q);
					z80->gpr[RH] = (uint8_t)(hl>>8);
					z80->gpr[RL] = (uint8_t)(hl>>0);
					Z80_ADD_CYCLES(z80, 7);
				} break;

				// Z=3
				case 0x43: { // LD (nn), BC
					uint8_t nl = z80_fetch_op_x(z80, sms);
					uint8_t nh = z80_fetch_op_x(z80, sms);
					uint16_t addr = z80_pair(nh, nl);
					z80_mem_write(sms, z80->timestamp,
						addr, z80->gpr[RC]);
					Z80_ADD_CYCLES(z80, 3);
					addr++;
					z80_mem_write(sms, z80->timestamp,
						addr, z80->gpr[RB]);
					Z80_ADD_CYCLES(z80, 3);
				} break;
				case 0x53: { // LD (nn), DE
					uint8_t nl = z80_fetch_op_x(z80, sms);
					uint8_t nh = z80_fetch_op_x(z80, sms);
					uint16_t addr = z80_pair(nh, nl);
					z80_mem_write(sms, z80->timestamp,
						addr, z80->gpr[RE]);
					Z80_ADD_CYCLES(z80, 3);
					addr++;
					z80_mem_write(sms, z80->timestamp,
						addr, z80->gpr[RD]);
					Z80_ADD_CYCLES(z80, 3);
				} break;
				// 0x63 appears to be an alias to LD (nn), HL/IX/IY
				// I don't know if IX/IY works there, though
				case 0x73: { // LD (nn), SP
					uint8_t nl = z80_fetch_op_x(z80, sms);
					uint8_t nh = z80_fetch_op_x(z80, sms);
					uint16_t addr = z80_pair(nh, nl);
					z80_mem_write(sms, z80->timestamp,
						addr,
						(uint8_t)(z80->sp>>0));
					Z80_ADD_CYCLES(z80, 3);
					addr++;
					z80_mem_write(sms, z80->timestamp,
						addr,
						(uint8_t)(z80->sp>>8));
					Z80_ADD_CYCLES(z80, 3);
				} break;

				case 0x4B: { // LD BC, (nn)
					uint8_t nl = z80_fetch_op_x(z80, sms);
					uint8_t nh = z80_fetch_op_x(z80, sms);
					uint16_t addr = z80_pair(nh, nl);
					z80->gpr[RC] = z80_mem_read(sms, z80->timestamp, addr);
					Z80_ADD_CYCLES(z80, 3);
					addr++;
					z80->gpr[RB] = z80_mem_read(sms, z80->timestamp, addr);
					Z80_ADD_CYCLES(z80, 3);
				} break;
				case 0x5B: { // LD DE, (nn)
					uint8_t nl = z80_fetch_op_x(z80, sms);
					uint8_t nh = z80_fetch_op_x(z80, sms);
					uint16_t addr = z80_pair(nh, nl);
					z80->gpr[RE] = z80_mem_read(sms, z80->timestamp, addr);
					Z80_ADD_CYCLES(z80, 3);
					addr++;
					z80->gpr[RD] = z80_mem_read(sms, z80->timestamp, addr);
					Z80_ADD_CYCLES(z80, 3);
				} break;
				// 0x6B
				case 0x7B: { // LD SP, (nn)
					uint8_t nl = z80_fetch_op_x(z80, sms);
					uint8_t nh = z80_fetch_op_x(z80, sms);
					uint16_t addr = z80_pair(nh, nl);
					uint8_t spl = z80_mem_read(sms, z80->timestamp, addr);
					Z80_ADD_CYCLES(z80, 3);
					addr++;
					uint8_t sph = z80_mem_read(sms, z80->timestamp, addr);
					Z80_ADD_CYCLES(z80, 3);
					z80->sp = z80_pair(sph, spl);
				} break;

				// Z=4: NEG
				case 0x44: case 0x4C:
				case 0x54: case 0x5C:
				case 0x64: case 0x6C:
				case 0x74: case 0x7C:
					z80->gpr[RA] = z80_sub8(z80, 0, z80->gpr[RA]);
					break;

				// Z=5: RETI/RETN
				case 0x45: case 0x4D:
				case 0x55: case 0x5D:
				case 0x65: case 0x6D:
				case 0x75: case 0x7D:
					z80->iff1 = z80->iff2;
					z80_op_ret(z80, sms);
					break;

				// Z=6: IM x
				case 0x46: z80->im = 0; break;
				case 0x4E: z80->im = 0; break;
				case 0x56: z80->im = 1; break;
				case 0x5E: z80->im = 2; break;
				case 0x66: z80->im = 0; break;
				case 0x6E: z80->im = 0; break;
				case 0x76: z80->im = 1; break;
				case 0x7E: z80->im = 2; break;

				// Z=7
				case 0x57: // LD A, I
					z80->gpr[RA] = z80->i;
					z80->gpr[RF] = (z80->gpr[RF]&0x01)
						| (z80->gpr[RA]&0xA8)
						| (z80->gpr[RA] == 0 ? 0x40 : 0x00)
						| (z80->iff2 ? 0x04 : 0x00);
					Z80_ADD_CYCLES(z80, 5);
					break;
				case 0x5F: // LD A, R
					z80->gpr[RA] = z80->r;
					z80->gpr[RF] = (z80->gpr[RF]&0x01)
						| (z80->gpr[RA]&0xA8)
						| (z80->gpr[RA] == 0 ? 0x40 : 0x00)
						| (z80->iff2 ? 0x04 : 0x00);
					Z80_ADD_CYCLES(z80, 5);
					break;

				case 0x67: { // RRD
					uint16_t addr = z80_pair_pbe(&z80->gpr[RH]);
					uint16_t val = 0xFF&(uint16_t)z80_mem_read(sms,
						z80->timestamp, addr);
					Z80_ADD_CYCLES(z80, 3);
					uint8_t na = (val&0x0F);
					val = (val>>4);
					val |= ((z80->gpr[RA]<<4)&0xF0);
					z80->gpr[RA] &= 0xF0;
					z80->gpr[RA] |= na;
					Z80_ADD_CYCLES(z80, 4);
					z80_mem_write(sms, z80->timestamp, addr, (uint8_t)(val));
					Z80_ADD_CYCLES(z80, 3);
					z80->gpr[RF] = (z80->gpr[RF]&0x01)
						| (z80->gpr[RA]&0xA8)
						| (z80->gpr[RA] == 0 ? 0x40 : 0x00)
						| z80_parity(z80->gpr[RA]);
				} break;
				case 0x6F: { // RLD
					uint16_t addr = z80_pair_pbe(&z80->gpr[RH]);
					uint16_t val = 0xFF&(uint16_t)z80_mem_read(sms,
						z80->timestamp, addr);
					Z80_ADD_CYCLES(z80, 3);
					val = (val<<4);
					val |= ((z80->gpr[RA])&0x0F);
					z80->gpr[RA] &= 0xF0;
					z80->gpr[RA] |= (val>>8)&0x0F;
					Z80_ADD_CYCLES(z80, 4);
					z80_mem_write(sms, z80->timestamp, addr, (uint8_t)(val));
					Z80_ADD_CYCLES(z80, 3);
					z80->gpr[RF] = (z80->gpr[RF]&0x01)
						| (z80->gpr[RA]&0xA8)
						| (z80->gpr[RA] == 0 ? 0x40 : 0x00)
						| z80_parity(z80->gpr[RA]);
				} break;

				//
				// X=2
				//

				// Z=0
				case 0xA0: { // LDI
					uint16_t dr = z80_pair_pbe(&z80->gpr[RD]);
					uint16_t sr = z80_pair_pbe(&z80->gpr[RH]);
					uint8_t dat = z80_mem_read(sms, z80->timestamp, sr);
					Z80_ADD_CYCLES(z80, 3);
					z80_mem_write(sms, z80->timestamp, dr, dat);
					Z80_ADD_CYCLES(z80, 3+2);
					z80->gpr[RF] = (z80->gpr[RF]&0xC1)
						| ((dat+z80->gpr[RA])&0x08)
						| (((dat+z80->gpr[RA])<<4)&0x20);

					if((++z80->gpr[RE]) == 0) { z80->gpr[RD]++; }
					if((++z80->gpr[RL]) == 0) { z80->gpr[RH]++; }
					if((z80->gpr[RC]--) == 0) {
						z80->gpr[RB]--;
					} else if(z80->gpr[RC] == 0 && z80->gpr[RB] == 0) {
						break;
					}
					z80->gpr[RF] |= 0x04;
				} break;
				case 0xB0: { // LDIR
					uint16_t dr = z80_pair_pbe(&z80->gpr[RD]);
					uint16_t sr = z80_pair_pbe(&z80->gpr[RH]);
					uint8_t dat = z80_mem_read(sms, z80->timestamp, sr);
					Z80_ADD_CYCLES(z80, 3);
					z80_mem_write(sms, z80->timestamp, dr, dat);
					Z80_ADD_CYCLES(z80, 3+2);
					z80->gpr[RF] = (z80->gpr[RF]&0xC1)
						| ((dat+z80->gpr[RA])&0x08)
						| (((dat+z80->gpr[RA])<<4)&0x20);

					if((++z80->gpr[RE]) == 0) { z80->gpr[RD]++; }
					if((++z80->gpr[RL]) == 0) { z80->gpr[RH]++; }
					if((z80->gpr[RC]--) == 0) {
						z80->gpr[RB]--;
					} else if(z80->gpr[RC] == 0 && z80->gpr[RB] == 0) {
						break;
					}
					z80->gpr[RF] |= 0x04;
					z80->pc -= 2;
					z80->wz[0] = (uint8_t)(z80->pc>>8);
					Z80_ADD_CYCLES(z80, 5);
				} break;

				case 0xA8: { // LDD
					uint16_t dr = z80_pair_pbe(&z80->gpr[RD]);
					uint16_t sr = z80_pair_pbe(&z80->gpr[RH]);
					uint8_t dat = z80_mem_read(sms, z80->timestamp, sr);
					Z80_ADD_CYCLES(z80, 3);
					z80_mem_write(sms, z80->timestamp, dr, dat);
					Z80_ADD_CYCLES(z80, 3+2);
					z80->gpr[RF] = (z80->gpr[RF]&0xC1)
						| ((dat+z80->gpr[RA])&0x08)
						| (((dat+z80->gpr[RA])<<4)&0x20);

					if((z80->gpr[RE]--) == 0) { z80->gpr[RD]--; }
					if((z80->gpr[RL]--) == 0) { z80->gpr[RH]--; }
					if((z80->gpr[RC]--) == 0) {
						z80->gpr[RB]--;
					} else if(z80->gpr[RC] == 0 && z80->gpr[RB] == 0) {
						break;
					}
					z80->gpr[RF] |= 0x04;
				} break;
				case 0xB8: { // LDDR
					uint16_t dr = z80_pair_pbe(&z80->gpr[RD]);
					uint16_t sr = z80_pair_pbe(&z80->gpr[RH]);
					uint8_t dat = z80_mem_read(sms, z80->timestamp, sr);
					Z80_ADD_CYCLES(z80, 3);
					z80_mem_write(sms, z80->timestamp, dr, dat);
					Z80_ADD_CYCLES(z80, 3+2);
					z80->gpr[RF] = (z80->gpr[RF]&0xC1)
						| ((dat+z80->gpr[RA])&0x08)
						| (((dat+z80->gpr[RA])<<4)&0x20);

					if((z80->gpr[RE]--) == 0) { z80->gpr[RD]--; }
					if((z80->gpr[RL]--) == 0) { z80->gpr[RH]--; }
					if((z80->gpr[RC]--) == 0) {
						z80->gpr[RB]--;
					} else if(z80->gpr[RC] == 0 && z80->gpr[RB] == 0) {
						break;
					}
					z80->gpr[RF] |= 0x04;
					z80->pc -= 2;
					z80->wz[0] = (uint8_t)(z80->pc>>8);
					Z80_ADD_CYCLES(z80, 5);
				} break;

				//

				case 0xA1: { // CPI
					uint16_t sr = z80_pair_pbe(&z80->gpr[RH]);
					uint8_t dat = z80_mem_read(sms, z80->timestamp, sr);
					Z80_ADD_CYCLES(z80, 3);
					uint8_t c = z80->gpr[RF]&0x01;
					z80_sub8(z80, z80->gpr[RA], dat);
					Z80_ADD_CYCLES(z80, 3+2);
					z80->gpr[RF] = (z80->gpr[RF]&0xD0)
						| 0x02
						| c;

					uint8_t f31 = z80->gpr[RA]-dat-((z80->gpr[RF]&0x10)>>4);
					z80->gpr[RF] |= (f31&0x08)|((f31<<4)&0x20);
					if((++z80->gpr[RL]) == 0) { z80->gpr[RH]++; }
					if((z80->gpr[RC]--) == 0) {
						z80->gpr[RB]--;
					} else if(z80->gpr[RC] == 0 && z80->gpr[RB] == 0) {
						break;
					}
					z80->gpr[RF] |= 0x04;
				} break;
				case 0xB1: { // CPIR
					uint16_t sr = z80_pair_pbe(&z80->gpr[RH]);
					uint8_t dat = z80_mem_read(sms, z80->timestamp, sr);
					Z80_ADD_CYCLES(z80, 3);
					uint8_t c = z80->gpr[RF]&0x01;
					z80_sub8(z80, z80->gpr[RA], dat);
					Z80_ADD_CYCLES(z80, 3+2);
					z80->gpr[RF] = (z80->gpr[RF]&0xD0)
						| 0x02
						| c;

					uint8_t f31 = z80->gpr[RA]-dat-((z80->gpr[RF]&0x10)>>4);
					z80->gpr[RF] |= (f31&0x08)|((f31<<4)&0x20);
					if((++z80->gpr[RL]) == 0) { z80->gpr[RH]++; }
					if((z80->gpr[RC]--) == 0) {
						z80->gpr[RB]--;
					} else if(z80->gpr[RC] == 0 && z80->gpr[RB] == 0) {
						break;
					}
					z80->gpr[RF] |= 0x04;
					if(z80->gpr[RA] != dat) {
						z80->pc -= 2;
						z80->wz[0] = (uint8_t)(z80->pc>>8);
						Z80_ADD_CYCLES(z80, 5);
					}
				} break;

				case 0xA9: { // CPD
					uint16_t sr = z80_pair_pbe(&z80->gpr[RH]);
					uint8_t dat = z80_mem_read(sms, z80->timestamp, sr);
					Z80_ADD_CYCLES(z80, 3);
					uint8_t c = z80->gpr[RF]&0x01;
					z80_sub8(z80, z80->gpr[RA], dat);
					Z80_ADD_CYCLES(z80, 3+2);
					z80->gpr[RF] = (z80->gpr[RF]&0xD0)
						| 0x02
						| c;

					uint8_t f31 = z80->gpr[RA]-dat-((z80->gpr[RF]&0x10)>>4);
					z80->gpr[RF] |= (f31&0x08)|((f31<<4)&0x20);
					if((z80->gpr[RL]--) == 0) { z80->gpr[RH]--; }
					if((z80->gpr[RC]--) == 0) {
						z80->gpr[RB]--;
					} else if(z80->gpr[RC] == 0 && z80->gpr[RB] == 0) {
						break;
					}
					z80->gpr[RF] |= 0x04;
				} break;
				case 0xB9: { // CPDR
					uint16_t sr = z80_pair_pbe(&z80->gpr[RH]);
					uint8_t dat = z80_mem_read(sms, z80->timestamp, sr);
					Z80_ADD_CYCLES(z80, 3);
					uint8_t c = z80->gpr[RF]&0x01;
					z80_sub8(z80, z80->gpr[RA], dat);
					Z80_ADD_CYCLES(z80, 3+2);
					z80->gpr[RF] = (z80->gpr[RF]&0xD0)
						| 0x02
						| c;

					uint8_t f31 = z80->gpr[RA]-dat-((z80->gpr[RF]&0x10)>>4);
					z80->gpr[RF] |= (f31&0x08)|((f31<<4)&0x20);
					if((z80->gpr[RL]--) == 0) { z80->gpr[RH]--; }
					if((z80->gpr[RC]--) == 0) {
						z80->gpr[RB]--;
					} else if(z80->gpr[RC] == 0 && z80->gpr[RB] == 0) {
						break;
					}
					z80->gpr[RF] |= 0x04;
					if(z80->gpr[RA] != dat) {
						z80->pc -= 2;
						z80->wz[0] = (uint8_t)(z80->pc>>8);
						Z80_ADD_CYCLES(z80, 5);
					}
				} break;

				//
				// HERE BE DRAGONS
				// NO REALLY, THE FLAG AFFECTION MAKES LESS THAN NO SENSE
				//

				case 0xA2: { // INI
					uint16_t sr = z80_pair_pbe(&z80->gpr[RB]);
					uint16_t dr = z80_pair_pbe(&z80->gpr[RH]);
					uint8_t dat = z80_io_read(sms, z80->timestamp, sr);
					Z80_ADD_CYCLES(z80, 4);
					z80_mem_write(sms, z80->timestamp, dr, dat);
					Z80_ADD_CYCLES(z80, 4);
					z80->gpr[RB]--;

					uint8_t magic1 = (z80->gpr[RC]+1)&0xFF;
					uint8_t magic2 = (magic1+dat);
					z80->gpr[RF] = (z80->gpr[RB]&0xA8)
						| (magic2 < dat ? 0x11 : 0x00)
						| (z80->gpr[RB] == 0 ? 0x40 : 0x00)
						| z80_parity(z80->gpr[RB]^(magic2&7))
						| ((dat>>6)&0x02);

					if((++z80->gpr[RL]) == 0) { z80->gpr[RH]++; }
				} break;
				case 0xB2: { // INIR
					uint16_t sr = z80_pair_pbe(&z80->gpr[RB]);
					uint16_t dr = z80_pair_pbe(&z80->gpr[RH]);
					uint8_t dat = z80_io_read(sms, z80->timestamp, sr);
					Z80_ADD_CYCLES(z80, 4);
					z80_mem_write(sms, z80->timestamp, dr, dat);
					Z80_ADD_CYCLES(z80, 4);
					z80->gpr[RB]--;

					uint8_t magic1 = (z80->gpr[RC]+1)&0xFF;
					uint8_t magic2 = (magic1+dat);
					z80->gpr[RF] = (z80->gpr[RB]&0xA8)
						| (magic2 < dat ? 0x11 : 0x00)
						| (z80->gpr[RB] == 0 ? 0x40 : 0x00)
						| z80_parity(z80->gpr[RB]^(magic2&7))
						| ((dat>>6)&0x02);

					if((++z80->gpr[RL]) == 0) { z80->gpr[RH]++; }
					if(z80->gpr[RB] == 0x00) {
						break;
					}
					z80->pc -= 2;
					z80->wz[0] = (uint8_t)(z80->pc>>8);
					Z80_ADD_CYCLES(z80, 5);
				} break;

				case 0xAA: { // IND
					uint16_t sr = z80_pair_pbe(&z80->gpr[RB]);
					uint16_t dr = z80_pair_pbe(&z80->gpr[RH]);
					uint8_t dat = z80_io_read(sms, z80->timestamp, sr);
					Z80_ADD_CYCLES(z80, 4);
					z80_mem_write(sms, z80->timestamp, dr, dat);
					Z80_ADD_CYCLES(z80, 4);
					z80->gpr[RB]--;

					uint8_t magic1 = (z80->gpr[RC]-1)&0xFF;
					uint8_t magic2 = (magic1+dat);
					z80->gpr[RF] = (z80->gpr[RB]&0xA8)
						| (magic2 < dat ? 0x11 : 0x00)
						| (z80->gpr[RB] == 0 ? 0x40 : 0x00)
						| z80_parity(z80->gpr[RB]^(magic2&7))
						| ((dat>>6)&0x02);

					if((z80->gpr[RL]--) == 0) { z80->gpr[RH]--; }
				} break;
				case 0xBA: { // INDR
					uint16_t sr = z80_pair_pbe(&z80->gpr[RB]);
					uint16_t dr = z80_pair_pbe(&z80->gpr[RH]);
					uint8_t dat = z80_io_read(sms, z80->timestamp, sr);
					Z80_ADD_CYCLES(z80, 4);
					z80_mem_write(sms, z80->timestamp, dr, dat);
					Z80_ADD_CYCLES(z80, 4);
					z80->gpr[RB]--;

					uint8_t magic1 = (z80->gpr[RC]-1)&0xFF;
					uint8_t magic2 = (magic1+dat);
					z80->gpr[RF] = (z80->gpr[RB]&0xA8)
						| (magic2 < dat ? 0x11 : 0x00)
						| (z80->gpr[RB] == 0 ? 0x40 : 0x00)
						| z80_parity(z80->gpr[RB]^(magic2&7))
						| ((dat>>6)&0x02);

					if((z80->gpr[RL]--) == 0) { z80->gpr[RH]--; }
					if(z80->gpr[RB] == 0x00) {
						break;
					}
					z80->pc -= 2;
					z80->wz[0] = (uint8_t)(z80->pc>>8);
					Z80_ADD_CYCLES(z80, 5);
				} break;

				//

				case 0xA3: { // OUTI
					uint16_t sr = z80_pair_pbe(&z80->gpr[RH]);
					uint16_t dr = z80_pair_pbe(&z80->gpr[RB]);
					uint8_t dat = z80_mem_read(sms, z80->timestamp, sr);
					//if(z80->gpr[RC] == 0xBF) printf("OUTI %02X %02X %04X\n", z80->gpr[RB], z80->gpr[RC], sr);
					Z80_ADD_CYCLES(z80, 4);
					z80_io_write(sms, z80->timestamp, dr, dat);
					Z80_ADD_CYCLES(z80, 4);
					z80->gpr[RB]--;

					uint8_t magic1 = z80->gpr[RL];
					uint8_t magic2 = (magic1+dat);
					z80->gpr[RF] = (z80->gpr[RB]&0xA8)
						| (magic2 < dat ? 0x11 : 0x00)
						| (z80->gpr[RB] == 0 ? 0x40 : 0x00)
						| z80_parity(z80->gpr[RB]^(magic2&7))
						| ((dat>>6)&0x02);

					if((++z80->gpr[RL]) == 0) { z80->gpr[RH]++; }
				} break;
				case 0xB3: { // OTIR
					uint16_t sr = z80_pair_pbe(&z80->gpr[RH]);
					uint16_t dr = z80_pair_pbe(&z80->gpr[RB]);
					uint8_t dat = z80_mem_read(sms, z80->timestamp, sr);
					//printf("OTIR %02X %02X %04X\n", z80->gpr[RB], z80->gpr[RC], sr);
					Z80_ADD_CYCLES(z80, 4);
					z80_io_write(sms, z80->timestamp, dr, dat);
					Z80_ADD_CYCLES(z80, 4);
					z80->gpr[RB]--;

					uint8_t magic1 = z80->gpr[RL];
					uint8_t magic2 = (magic1+dat);
					z80->gpr[RF] = (z80->gpr[RB]&0xA8)
						| (magic2 < dat ? 0x11 : 0x00)
						| (z80->gpr[RB] == 0 ? 0x40 : 0x00)
						| z80_parity(z80->gpr[RB]^(magic2&7))
						| ((dat>>6)&0x02);

					if((++z80->gpr[RL]) == 0) { z80->gpr[RH]++; }
					if(z80->gpr[RB] == 0x00) {
						break;
					}
					z80->pc -= 2;
					z80->wz[0] = (uint8_t)(z80->pc>>8);
					Z80_ADD_CYCLES(z80, 5);
				} break;

				case 0xAB: { // OUTD
					uint16_t sr = z80_pair_pbe(&z80->gpr[RH]);
					uint16_t dr = z80_pair_pbe(&z80->gpr[RB]);
					uint8_t dat = z80_mem_read(sms, z80->timestamp, sr);
					Z80_ADD_CYCLES(z80, 4);
					z80_io_write(sms, z80->timestamp, dr, dat);
					Z80_ADD_CYCLES(z80, 4);
					z80->gpr[RB]--;

					uint8_t magic1 = z80->gpr[RL];
					uint8_t magic2 = (magic1+dat);
					z80->gpr[RF] = (z80->gpr[RB]&0xA8)
						| (magic2 < dat ? 0x11 : 0x00)
						| (z80->gpr[RB] == 0 ? 0x40 : 0x00)
						| z80_parity(z80->gpr[RB]^(magic2&7))
						| ((dat>>6)&0x02);

					if((z80->gpr[RL]--) == 0) { z80->gpr[RH]--; }
				} break;
				case 0xBB: { // OTDR
					uint16_t sr = z80_pair_pbe(&z80->gpr[RH]);
					uint16_t dr = z80_pair_pbe(&z80->gpr[RB]);
					uint8_t dat = z80_mem_read(sms, z80->timestamp, sr);
					Z80_ADD_CYCLES(z80, 4);
					z80_io_write(sms, z80->timestamp, dr, dat);
					Z80_ADD_CYCLES(z80, 4);
					z80->gpr[RB]--;

					uint8_t magic1 = z80->gpr[RL];
					uint8_t magic2 = (magic1+dat);
					z80->gpr[RF] = (z80->gpr[RB]&0xA8)
						| (magic2 < dat ? 0x11 : 0x00)
						| (z80->gpr[RB] == 0 ? 0x40 : 0x00)
						| z80_parity(z80->gpr[RB]^(magic2&7))
						| ((dat>>6)&0x02);

					if((z80->gpr[RL]--) == 0) { z80->gpr[RH]--; }
					if(z80->gpr[RB] == 0x00) {
						break;
					}
					z80->pc -= 2;
					z80->wz[0] = (uint8_t)(z80->pc>>8);
					Z80_ADD_CYCLES(z80, 5);
				} break;

				default:
					// TODO!
					fprintf(stderr, "OP: ED %02X\n", op);
					fflush(stderr); abort();
					break;
			} continue;
		}

		if(op == 0xCB) {
			if(false && ix >= 0) {
				// TODO!
				fprintf(stderr, "OP: %02X CB %02X\n", (ix<<5)|0xDD, op);
				fflush(stderr); abort();
				break;
			}

			uint8_t val;
			uint8_t bval;

			// Read Iz if necessary
			// FIXME: timing sucks here
			if(ix >= 0) {
				uint16_t addr = z80_pair_pbe(z80->idx[ix&1]);
				addr += (uint16_t)(int16_t)(int8_t)z80_fetch_op_x(z80, sms);
				z80->wz[0] = (uint8_t)(addr>>8);
				z80->wz[1] = (uint8_t)(addr>>0);
				Z80_ADD_CYCLES(z80, 1);
				val = z80_mem_read(sms, z80->timestamp, addr);
				Z80_ADD_CYCLES(z80, 4);
			}

			// Fetch
			op = z80_fetch_op_m1(z80, sms);
			//printf("*CB %04X %2d %02X %04X\n", z80->pc, ix, op, z80->sp);
			//int oy = (op>>3)&7;
			int oz = op&7;
			int ox = (op>>6);

			// Read if we haven't read Iz
			if(ix >= 0) {
				// Do nothing to val
				// But set bval to something suitable
				bval = z80->wz[0];

			} else if(oz == 6) {
				bval = z80->wz[0];
				val = z80_mem_read(sms, z80->timestamp,
					z80_pair_pbe(&z80->gpr[RH]));
				Z80_ADD_CYCLES(z80, 4);

			} else {
				bval = val = z80->gpr[oz];
			}


			// ALU
			switch(op&~7) {
				//
				// X=0
				//
				case 0x00: // RLC
					z80->gpr[RF] = ((val>>7)&0x01);
					val = ((val<<1)|((val>>7)&1));
					z80->gpr[RF] |= (val&0xA8) | z80_parity(val);
					z80->gpr[RF] |= (val == 0 ? 0x40 : 0x00);
					break;
				case 0x08: // RRC
					z80->gpr[RF] = (val&0x01);
					val = ((val<<7)|((val>>1)&0x7F));
					z80->gpr[RF] |= (val&0xA8) | z80_parity(val);
					z80->gpr[RF] |= (val == 0 ? 0x40 : 0x00);
					break;

				case 0x10: { // RL
					uint8_t c = z80->gpr[RF]&0x01;
					z80->gpr[RF] = ((val>>7)&0x01);
					val = (val<<1)|c;
					z80->gpr[RF] |= (val&0xA8) | z80_parity(val);
					z80->gpr[RF] |= (val == 0 ? 0x40 : 0x00);
				} break;
				case 0x18: { // RR
					uint8_t c = z80->gpr[RF]&0x01;
					z80->gpr[RF] = (val&0x01);
					val = ((c<<7)|((val>>1)&0x7F));
					z80->gpr[RF] |= (val&0xA8) | z80_parity(val);
					z80->gpr[RF] |= (val == 0 ? 0x40 : 0x00);
				} break;

				case 0x20: // SLL
					z80->gpr[RF] = ((val>>7)&0x01);
					val = (val<<1);
					z80->gpr[RF] |= (val&0xA8) | z80_parity(val);
					z80->gpr[RF] |= (val == 0 ? 0x40 : 0x00);
					break;
				case 0x28: // SRA
					z80->gpr[RF] = (val&0x01);
					val = (val>>1)|(val&0x80);
					z80->gpr[RF] |= (val&0xA8) | z80_parity(val);
					z80->gpr[RF] |= (val == 0 ? 0x40 : 0x00);
					break;
				case 0x30: // SLA
					z80->gpr[RF] = ((val>>7)&0x01);
					val = (val<<1)|1;
					z80->gpr[RF] |= (val&0xA8) | z80_parity(val);
					z80->gpr[RF] |= (val == 0 ? 0x40 : 0x00);
					break;
				case 0x38: // SRL
					z80->gpr[RF] = (val&0x01);
					val = (val>>1)&0x7F;
					z80->gpr[RF] |= (val&0xA8) | z80_parity(val);
					z80->gpr[RF] |= (val == 0 ? 0x40 : 0x00);
					break;

				//
				// X=1
				//
				case 0x40: { // BIT 0, r
					uint8_t sf = z80->gpr[RF]&0x01;
					z80_and8(z80, val, 0x01);
					z80->gpr[RF] &= ~0x29;
					z80->gpr[RF] |= sf | (bval&0x28);
				} break;
				case 0x48: { // BIT 1, r
					uint8_t sf = z80->gpr[RF]&0x01;
					z80_and8(z80, val, 0x02);
					z80->gpr[RF] &= ~0x29;
					z80->gpr[RF] |= sf | (bval&0x28);
				} break;
				case 0x50: { // BIT 2, r
					uint8_t sf = z80->gpr[RF]&0x01;
					z80_and8(z80, val, 0x04);
					z80->gpr[RF] &= ~0x29;
					z80->gpr[RF] |= sf | (bval&0x28);
				} break;
				case 0x58: { // BIT 3, r
					uint8_t sf = z80->gpr[RF]&0x01;
					z80_and8(z80, val, 0x08);
					z80->gpr[RF] &= ~0x29;
					z80->gpr[RF] |= sf | (bval&0x28);
				} break;
				case 0x60: { // BIT 4, r
					uint8_t sf = z80->gpr[RF]&0x01;
					z80_and8(z80, val, 0x10);
					z80->gpr[RF] &= ~0x29;
					z80->gpr[RF] |= sf | (bval&0x28);
				} break;
				case 0x68: { // BIT 5, r
					uint8_t sf = z80->gpr[RF]&0x01;
					z80_and8(z80, val, 0x20);
					z80->gpr[RF] &= ~0x29;
					z80->gpr[RF] |= sf | (bval&0x28);
				} break;
				case 0x70: { // BIT 6, r
					uint8_t sf = z80->gpr[RF]&0x01;
					z80_and8(z80, val, 0x40);
					z80->gpr[RF] &= ~0x29;
					z80->gpr[RF] |= sf | (bval&0x28);
				} break;
				case 0x78: { // BIT 7, r
					uint8_t sf = z80->gpr[RF]&0x01;
					z80_and8(z80, val, 0x80);
					z80->gpr[RF] &= ~0x29;
					z80->gpr[RF] |= sf | (bval&0x28);
				} break;

				//
				// X=2
				//
				case 0x80: val &= ~0x01; break; // RES 0, r
				case 0x88: val &= ~0x02; break; // RES 1, r
				case 0x90: val &= ~0x04; break; // RES 2, r
				case 0x98: val &= ~0x08; break; // RES 3, r
				case 0xA0: val &= ~0x10; break; // RES 4, r
				case 0xA8: val &= ~0x20; break; // RES 5, r
				case 0xB0: val &= ~0x40; break; // RES 6, r
				case 0xB8: val &= ~0x80; break; // RES 7, r

				//
				// X=3
				//
				case 0xC0: val |= 0x01; break; // SET 0, r
				case 0xC8: val |= 0x02; break; // SET 1, r
				case 0xD0: val |= 0x04; break; // SET 2, r
				case 0xD8: val |= 0x08; break; // SET 3, r
				case 0xE0: val |= 0x10; break; // SET 4, r
				case 0xE8: val |= 0x20; break; // SET 5, r
				case 0xF0: val |= 0x40; break; // SET 6, r
				case 0xF8: val |= 0x80; break; // SET 7, r

				default:
					// TODO!
					fprintf(stderr, "OP: CB %02X\n", op);
					fflush(stderr); abort();
					break;
			}

			// Write
			if(ox == 1) {
				// BIT - Do nothing here

			} else if(oz == 6) {
				if(ix < 0) {
					z80_mem_write(sms, z80->timestamp,
						z80_pair_pbe(&z80->gpr[RH]),
						val);
					Z80_ADD_CYCLES(z80, 3);
				}

			} else {
				z80->gpr[oz] = val;
			}

			// Write to (Iz+d) if we use DD/FD
			if(ix >= 0) {
				uint16_t addr = z80_pair_pbe(&z80->wz[0]);
				z80_mem_write(sms, z80->timestamp,
					addr,
					val);
				Z80_ADD_CYCLES(z80, 3);
			}

			//printf("*CB DONE %04X\n", z80->pc);

			continue;
		}

		//printf("%04X %02X %04X\n", z80->pc-1, op, z80->sp);

		// X=1 decode (LD y, z)
		if((op>>6) == 1) {

			if(op == 0x76) {
				// HALT
				if(TIME_IN_ORDER(z80->timestamp, z80->timestamp_end)) {
					z80->timestamp = z80->timestamp_end;
				}
				z80->halted = true;
				z80->noni = 0;
				if(z80->iff1 != 0 && (sms->vdp.irq_out&sms->vdp.irq_mask) != 0) {
					/*
					printf("IN_IRQ HALT %d %d %02X %02X %02X %016llX\n"
						, z80->noni
						, z80->iff1
						, sms->vdp.irq_out
						, sms->vdp.irq_mask
						, sms->vdp.status
						, (unsigned long long)z80->timestamp
						);
					*/

					z80_irq(z80, sms, 0xFF);
					continue;
				} else {
					while(TIME_IN_ORDER(z80->timestamp, timestamp)) {
						Z80_ADD_CYCLES(z80, 4);
					}
					return;
				}
			}

			int oy = (op>>3)&7;
			int oz = op&7;

			uint8_t val;

			// Read
			if((oz&~1)==4 && ix >= 0 && oy != 6) {
				val = z80->idx[ix&1][oz&1];

			} else if(oz == 6) {
				if(ix >= 0) {
					uint16_t addr = z80_fetch_ix_d(z80, sms, ix);
					val = z80_mem_read(sms, z80->timestamp, addr);
					Z80_ADD_CYCLES(z80, 3);
				} else {
					val = z80_mem_read(sms, z80->timestamp,
						z80_pair_pbe(&z80->gpr[RH]));
					Z80_ADD_CYCLES(z80, 3);
				}

			} else {
				val = z80->gpr[oz];
			}

			// Write
			if((oy&~1)==4 && ix >= 0 && oz != 6) {
				z80->idx[ix&1][oy&1] = val;

			} else if(oy == 6) {
				if(ix >= 0) {
					uint16_t addr = z80_fetch_ix_d(z80, sms, ix);
					z80_mem_write(sms, z80->timestamp,
						addr,
						val);
					Z80_ADD_CYCLES(z80, 3);
				} else {
					z80_mem_write(sms, z80->timestamp,
						z80_pair_pbe(&z80->gpr[RH]),
						val);
					Z80_ADD_CYCLES(z80, 3);
				}

			} else {
				z80->gpr[oy] = val;
			}

			continue;
		} 

		// X=2 decode (ALU[y] A, z)
		if((op>>6) == 2) {
			int oy = (op>>3)&7;
			int oz = op&7;

			uint8_t val;

			// Read
			if((oz&~1)==4 && ix >= 0) {
				val = z80->idx[ix&1][oz&1];

			} else if(oz == 6) {
				if(ix >= 0) {
					uint16_t addr = z80_fetch_ix_d(z80, sms, ix);
					val = z80_mem_read(sms, z80->timestamp, addr);
					Z80_ADD_CYCLES(z80, 3);
				} else {
					val = z80_mem_read(sms, z80->timestamp,
						z80_pair_pbe(&z80->gpr[RH]));
					Z80_ADD_CYCLES(z80, 3);
				}

			} else {
				val = z80->gpr[oz];
			}

			// ALU
			switch(oy) {
				case 0: { // ADD A, r
					z80->gpr[RA] = z80_add8(z80, z80->gpr[RA], val);
				} break;
				case 1: { // ADC A, r
					z80->gpr[RA] = z80_adc8(z80, z80->gpr[RA], val);
				} break;
				case 2: { // SUB r
					z80->gpr[RA] = z80_sub8(z80, z80->gpr[RA], val);
				} break;
				case 3: { // SBC A, r
					z80->gpr[RA] = z80_sbc8(z80, z80->gpr[RA], val);
				} break;
				case 4: { // AND r
					z80->gpr[RA] = z80_and8(z80, z80->gpr[RA], val);
				} break;
				case 5: { // XOR r
					z80->gpr[RA] = z80_xor8(z80, z80->gpr[RA], val);
				} break;
				case 6: { // OR r
					z80->gpr[RA] = z80_or8(z80, z80->gpr[RA], val);
				} break;
				case 7: { // CP r
					z80_sub8(z80, z80->gpr[RA], val);
					z80->gpr[RF] = (z80->gpr[RF]&~0x28) | (val&0x28);
				} break;
			}

			continue;
		}

		// Decode
		switch(op) {
			//
			// X=0
			//

			// Z=0
			case 0x00: // NOP
				break;
			case 0x08: { // EX AF, AF'
				uint8_t t;
				t = z80->gpr[RA]; z80->gpr[RA] = z80->shadow[RA]; z80->shadow[RA] = t;
				t = z80->gpr[RF]; z80->gpr[RF] = z80->shadow[RF]; z80->shadow[RF] = t;
			} break;
			case 0x10: // DJNZ d
				z80->gpr[RB]--;
				Z80_ADD_CYCLES(z80, 1);
				z80_op_jr_cond(z80, sms, z80->gpr[RB] != 0);
				break;
			case 0x18: // JR d
				z80_op_jr_cond(z80, sms, true);
				break;
			case 0x20: // JR NZ, d
				z80_op_jr_cond(z80, sms, (z80->gpr[RF]&0x40) == 0);
				break;
			case 0x28: // JR Z, d
				z80_op_jr_cond(z80, sms, (z80->gpr[RF]&0x40) != 0);
				break;
			case 0x30: // JR NC, d
				z80_op_jr_cond(z80, sms, (z80->gpr[RF]&0x01) == 0);
				break;
			case 0x38: // JR C, d
				z80_op_jr_cond(z80, sms, (z80->gpr[RF]&0x01) != 0);
				break;

			// Z=1
			case 0x01: // LD BC, nn
				z80->gpr[RC] = z80_fetch_op_x(z80, sms);
				z80->gpr[RB] = z80_fetch_op_x(z80, sms);
				break;
			case 0x11: // LD DE, nn
				z80->gpr[RE] = z80_fetch_op_x(z80, sms);
				z80->gpr[RD] = z80_fetch_op_x(z80, sms);
				break;
			case 0x21: if(ix >= 0) {
					// LD Iz, nn
					z80->idx[ix&1][1] = z80_fetch_op_x(z80, sms);
					z80->idx[ix&1][0] = z80_fetch_op_x(z80, sms);
				} else {
					// LD HL, nn
					z80->gpr[RL] = z80_fetch_op_x(z80, sms);
					z80->gpr[RH] = z80_fetch_op_x(z80, sms);
				} break;
			case 0x31: { // LD SP, nn
				uint16_t spl = (uint16_t)z80_fetch_op_x(z80, sms);
				uint16_t sph = (uint16_t)z80_fetch_op_x(z80, sms);
				z80->sp = z80_pair(sph, spl);
			} break;

			case 0x09: if(ix >= 0) {
					// ADD Iz, BC
					uint16_t hl = z80_pair_pbe(z80->idx[ix&1]);
					uint16_t q = z80_pair_pbe(&z80->gpr[RB]);
					hl = z80_add16(z80, hl, q);
					z80->idx[ix&1][0] = (uint8_t)(hl>>8);
					z80->idx[ix&1][1] = (uint8_t)(hl>>0);
					Z80_ADD_CYCLES(z80, 7);
				} else {
					// ADD HL, BC
					uint16_t hl = z80_pair_pbe(&z80->gpr[RH]);
					uint16_t q = z80_pair_pbe(&z80->gpr[RB]);
					hl = z80_add16(z80, hl, q);
					z80->gpr[RH] = (uint8_t)(hl>>8);
					z80->gpr[RL] = (uint8_t)(hl>>0);
					Z80_ADD_CYCLES(z80, 7);
				} break;
			case 0x19: if(ix >= 0) {
					// ADD Iz, DE
					uint16_t hl = z80_pair_pbe(z80->idx[ix&1]);
					uint16_t q = z80_pair_pbe(&z80->gpr[RD]);
					hl = z80_add16(z80, hl, q);
					z80->idx[ix&1][0] = (uint8_t)(hl>>8);
					z80->idx[ix&1][1] = (uint8_t)(hl>>0);
					Z80_ADD_CYCLES(z80, 7);
				} else {
					// ADD HL, DE
					uint16_t hl = z80_pair_pbe(&z80->gpr[RH]);
					uint16_t q = z80_pair_pbe(&z80->gpr[RD]);
					hl = z80_add16(z80, hl, q);
					z80->gpr[RH] = (uint8_t)(hl>>8);
					z80->gpr[RL] = (uint8_t)(hl>>0);
					Z80_ADD_CYCLES(z80, 7);
				} break;
			case 0x29: if(ix >= 0) {
					// ADD Iz, Iz
					uint16_t hl = z80_pair_pbe(z80->idx[ix&1]);
					uint16_t q = hl;
					hl = z80_add16(z80, hl, q);
					z80->idx[ix&1][0] = (uint8_t)(hl>>8);
					z80->idx[ix&1][1] = (uint8_t)(hl>>0);
					Z80_ADD_CYCLES(z80, 7);
				} else {
					// ADD HL, HL
					uint16_t hl = z80_pair_pbe(&z80->gpr[RH]);
					uint16_t q = hl;
					hl = z80_add16(z80, hl, q);
					z80->gpr[RH] = (uint8_t)(hl>>8);
					z80->gpr[RL] = (uint8_t)(hl>>0);
					Z80_ADD_CYCLES(z80, 7);
				} break;
			case 0x39: if(ix >= 0) {
					// ADD Iz, SP
					uint16_t hl = z80_pair_pbe(z80->idx[ix&1]);
					uint16_t q = z80->sp;
					hl = z80_add16(z80, hl, q);
					z80->idx[ix&1][0] = (uint8_t)(hl>>8);
					z80->idx[ix&1][1] = (uint8_t)(hl>>0);
					Z80_ADD_CYCLES(z80, 7);
				} else {
					// ADD HL, SP
					uint16_t hl = z80_pair_pbe(&z80->gpr[RH]);
					uint16_t q = z80->sp;
					hl = z80_add16(z80, hl, q);
					z80->gpr[RH] = (uint8_t)(hl>>8);
					z80->gpr[RL] = (uint8_t)(hl>>0);
					Z80_ADD_CYCLES(z80, 7);
				} break;

			// Z=2
			case 0x02: // LD (BC), A
				z80_mem_write(sms, z80->timestamp,
					z80_pair_pbe(&z80->gpr[RB]),
					z80->gpr[RA]);
				Z80_ADD_CYCLES(z80, 3);
				break;
			case 0x12: // LD (DE), A
				z80_mem_write(sms, z80->timestamp,
					z80_pair_pbe(&z80->gpr[RD]),
					z80->gpr[RA]);
				Z80_ADD_CYCLES(z80, 3);
				break;
			case 0x0A: // LD A, (BC)
				z80->gpr[RA] = z80_mem_read(sms, z80->timestamp,
					z80_pair_pbe(&z80->gpr[RB]));
				Z80_ADD_CYCLES(z80, 3);
				break;
			case 0x1A: // LD A, (DE)
				z80->gpr[RA] = z80_mem_read(sms, z80->timestamp,
					z80_pair_pbe(&z80->gpr[RD]));
				Z80_ADD_CYCLES(z80, 3);
				break;

			case 0x22:
				if(ix >= 0) {
					// LD Iz, (nn)
					uint8_t nl = z80_fetch_op_x(z80, sms);
					uint8_t nh = z80_fetch_op_x(z80, sms);
					uint16_t addr = z80_pair(nh, nl);
					z80_mem_write(sms, z80->timestamp, addr, z80->idx[ix&1][1]);
					Z80_ADD_CYCLES(z80, 3);
					addr++;
					z80_mem_write(sms, z80->timestamp, addr, z80->idx[ix&1][0]);
					Z80_ADD_CYCLES(z80, 3);
				} else {
					// LD (nn), HL
					uint8_t nl = z80_fetch_op_x(z80, sms);
					uint8_t nh = z80_fetch_op_x(z80, sms);
					uint16_t addr = z80_pair(nh, nl);
					z80_mem_write(sms, z80->timestamp, addr, z80->gpr[RL]);
					Z80_ADD_CYCLES(z80, 3);
					addr++;
					z80_mem_write(sms, z80->timestamp, addr, z80->gpr[RH]);
					Z80_ADD_CYCLES(z80, 3);
				} break;
			case 0x32: { // LD (nn), A
				uint8_t nl = z80_fetch_op_x(z80, sms);
				uint8_t nh = z80_fetch_op_x(z80, sms);
				uint16_t addr = z80_pair(nh, nl);
				z80_mem_write(sms, z80->timestamp, addr, z80->gpr[RA]);
				Z80_ADD_CYCLES(z80, 3);
			} break;
			case 0x2A:
				if(ix >= 0) {
					// LD Iz, (nn)
					uint8_t nl = z80_fetch_op_x(z80, sms);
					uint8_t nh = z80_fetch_op_x(z80, sms);
					uint16_t addr = z80_pair(nh, nl);
					z80->idx[ix&1][1] = z80_mem_read(sms, z80->timestamp, addr);
					Z80_ADD_CYCLES(z80, 3);
					addr++;
					z80->idx[ix&1][0] = z80_mem_read(sms, z80->timestamp, addr);
					Z80_ADD_CYCLES(z80, 3);
				} else {
					// LD HL, (nn)
					uint8_t nl = z80_fetch_op_x(z80, sms);
					uint8_t nh = z80_fetch_op_x(z80, sms);
					uint16_t addr = z80_pair(nh, nl);
					z80->gpr[RL] = z80_mem_read(sms, z80->timestamp, addr);
					Z80_ADD_CYCLES(z80, 3);
					addr++;
					z80->gpr[RH] = z80_mem_read(sms, z80->timestamp, addr);
					Z80_ADD_CYCLES(z80, 3);
				} break;
			case 0x3A: { // LD A, (nn)
				uint8_t nl = z80_fetch_op_x(z80, sms);
				uint8_t nh = z80_fetch_op_x(z80, sms);
				uint16_t addr = z80_pair(nh, nl);
				z80->gpr[RA] = z80_mem_read(sms, z80->timestamp, addr);
				Z80_ADD_CYCLES(z80, 3);
			} break;

			// Z=3
			case 0x03: // INC BC
				if((++z80->gpr[RC]) == 0) { z80->gpr[RB]++; }
				Z80_ADD_CYCLES(z80, 2);
				break;
			case 0x13: // INC DE
				if((++z80->gpr[RE]) == 0) { z80->gpr[RD]++; }
				Z80_ADD_CYCLES(z80, 2);
				break;
			case 0x23: if(ix >= 0) {
					// INC Iz
					if((++z80->idx[ix&1][1]) == 0) { z80->idx[ix&1][0]++; }
					Z80_ADD_CYCLES(z80, 2);
				} else {
					// INC HL
					if((++z80->gpr[RL]) == 0) { z80->gpr[RH]++; }
					Z80_ADD_CYCLES(z80, 2);
				} break;
			case 0x33: // INC SP
				z80->sp++;
				Z80_ADD_CYCLES(z80, 2);
				break;

			case 0x0B: // DEC BC
				if((z80->gpr[RC]--) == 0) { z80->gpr[RB]--; }
				Z80_ADD_CYCLES(z80, 2);
				break;
			case 0x1B: // DEC DE
				if((z80->gpr[RE]--) == 0) { z80->gpr[RD]--; }
				Z80_ADD_CYCLES(z80, 2);
				break;
			case 0x2B: if(ix >= 0) {
					// DEC Iz
					if((z80->idx[ix&1][1]--) == 0) { z80->idx[ix&1][0]--; }
					Z80_ADD_CYCLES(z80, 2);
				} else {
					// DEC HL
					if((z80->gpr[RL]--) == 0) { z80->gpr[RH]--; }
					Z80_ADD_CYCLES(z80, 2);
				} break;
			case 0x3B: // DEC SP
				z80->sp--;
				Z80_ADD_CYCLES(z80, 2);
				break;

			// Z=4
			case 0x04: // INC B
				z80->gpr[RB] = z80_inc8(z80, z80->gpr[RB]);
				break;
			case 0x0C: // INC C
				z80->gpr[RC] = z80_inc8(z80, z80->gpr[RC]);
				break;
			case 0x14: // INC D
				z80->gpr[RD] = z80_inc8(z80, z80->gpr[RD]);
				break;
			case 0x1C: // INC E
				z80->gpr[RE] = z80_inc8(z80, z80->gpr[RE]);
				break;
			case 0x24: if(ix >= 0) {
					// INC IzH
					z80->idx[ix&1][0] = z80_inc8(z80, z80->idx[ix&1][0]);
				} else {
					// INC H
					z80->gpr[RH] = z80_inc8(z80, z80->gpr[RH]);
				} break;
			case 0x2C: if(ix >= 0) {
					// INC IzL
					z80->idx[ix&1][1] = z80_inc8(z80, z80->idx[ix&1][1]);
				} else {
					// INC L
					z80->gpr[RL] = z80_inc8(z80, z80->gpr[RL]);
				} break;
			case 0x34: if(ix >= 0) {
					// INC (Iz+d)
					uint16_t addr = z80_fetch_ix_d(z80, sms, ix);
					uint8_t val = z80_mem_read(sms, z80->timestamp, addr);
					Z80_ADD_CYCLES(z80, 4);
					val = z80_inc8(z80, val);
					z80_mem_write(sms, z80->timestamp, addr, val);
					Z80_ADD_CYCLES(z80, 3);
				} else {
					// INC (HL)
					uint16_t addr = z80_pair_pbe(&z80->gpr[RH]);
					uint8_t val = z80_mem_read(sms, z80->timestamp, addr);
					Z80_ADD_CYCLES(z80, 4);
					val = z80_inc8(z80, val);
					z80_mem_write(sms, z80->timestamp, addr, val);
					Z80_ADD_CYCLES(z80, 3);
				} break;
			case 0x3C: // INC A
				z80->gpr[RA] = z80_inc8(z80, z80->gpr[RA]);
				break;

			// Z=5
			case 0x05: // DEC B
				z80->gpr[RB] = z80_dec8(z80, z80->gpr[RB]);
				break;
			case 0x0D: // DEC C
				z80->gpr[RC] = z80_dec8(z80, z80->gpr[RC]);
				break;
			case 0x15: // DEC D
				z80->gpr[RD] = z80_dec8(z80, z80->gpr[RD]);
				break;
			case 0x1D: // DEC E
				z80->gpr[RE] = z80_dec8(z80, z80->gpr[RE]);
				break;
			case 0x25: if(ix >= 0) {
					// DEC IzH
					z80->idx[ix&1][0] = z80_dec8(z80, z80->idx[ix&1][0]);
				} else {
					// DEC H
					z80->gpr[RH] = z80_dec8(z80, z80->gpr[RH]);
				} break;
			case 0x2D: if(ix >= 0) {
					// DEC IzL
					z80->idx[ix&1][1] = z80_dec8(z80, z80->idx[ix&1][1]);
				} else {
					// DEC L
					z80->gpr[RL] = z80_dec8(z80, z80->gpr[RL]);
				} break;
			case 0x35: if(ix >= 0) {
					// DEC (Iz+d)
					uint16_t addr = z80_fetch_ix_d(z80, sms, ix);
					uint8_t val = z80_mem_read(sms, z80->timestamp, addr);
					Z80_ADD_CYCLES(z80, 4);
					val = z80_dec8(z80, val);
					z80_mem_write(sms, z80->timestamp, addr, val);
					Z80_ADD_CYCLES(z80, 3);
				} else {
					// DEC (HL)
					uint16_t addr = z80_pair_pbe(&z80->gpr[RH]);
					uint8_t val = z80_mem_read(sms, z80->timestamp, addr);
					Z80_ADD_CYCLES(z80, 4);
					val = z80_dec8(z80, val);
					z80_mem_write(sms, z80->timestamp, addr, val);
					Z80_ADD_CYCLES(z80, 3);
				} break;
			case 0x3D: // DEC A
				z80->gpr[RA] = z80_dec8(z80, z80->gpr[RA]);
				break;

			// Z=6
			case 0x06: // LD B, n
				z80->gpr[RB] = z80_fetch_op_x(z80, sms);
				break;
			case 0x0E: // LD C, n
				z80->gpr[RC] = z80_fetch_op_x(z80, sms);
				break;
			case 0x16: // LD D, n
				z80->gpr[RD] = z80_fetch_op_x(z80, sms);
				break;
			case 0x1E: // LD E, n
				z80->gpr[RE] = z80_fetch_op_x(z80, sms);
				break;
			case 0x26: if(ix >= 0) {
					// LD IxH, n
					z80->idx[ix&1][0] = z80_fetch_op_x(z80, sms);
				} else {
					// LD H, n
					z80->gpr[RH] = z80_fetch_op_x(z80, sms);
				} break;
			case 0x2E: if(ix >= 0) {
					// LD IxL, n
					z80->idx[ix&1][1] = z80_fetch_op_x(z80, sms);
				} else {
					// LD L, n
					z80->gpr[RL] = z80_fetch_op_x(z80, sms);
				} break;
			case 0x36: if(ix >= 0) {
					// LD (Iz+d), n
					uint16_t addr = z80_fetch_ix_d(z80, sms, ix);
					Z80_ADD_CYCLES(z80, -5);
					uint8_t dat = z80_fetch_op_x(z80, sms);
					Z80_ADD_CYCLES(z80, 2);
					z80_mem_write(sms, z80->timestamp, addr, dat);
					Z80_ADD_CYCLES(z80, 3);
			
				} else {
					// LD (HL), n
					z80_mem_write(sms, z80->timestamp,
						z80_pair_pbe(&z80->gpr[RH]),
						z80_fetch_op_x(z80, sms));
					Z80_ADD_CYCLES(z80, 3);
				} break;
			case 0x3E: // LD A, n
				z80->gpr[RA] = z80_fetch_op_x(z80, sms);
				break;

			// Z=7
			case 0x07: // RLCA
				z80->gpr[RF] = (z80->gpr[RF]&0xC4)
					| ((z80->gpr[RA]>>7)&0x01);
				z80->gpr[RA] = ((z80->gpr[RA]<<1)|((z80->gpr[RA]>>7)&1));
				z80->gpr[RF] |= (z80->gpr[RA]&0x28);
				break;
			case 0x0F: // RRCA
				z80->gpr[RF] = (z80->gpr[RF]&0xC4)
					| ((z80->gpr[RA])&0x01);
				z80->gpr[RA] = ((z80->gpr[RA]<<7)|((z80->gpr[RA]>>1)&0x7F));
				z80->gpr[RF] |= (z80->gpr[RA]&0x28);
				break;
			case 0x17: { // RLA
				uint8_t c = z80->gpr[RF]&0x01;
				z80->gpr[RF] = (z80->gpr[RF]&0xC4)
					 |((z80->gpr[RA]>>7)&0x01);
				z80->gpr[RA] = (z80->gpr[RA]<<1)|c;
				z80->gpr[RF] |= (z80->gpr[RA]&0x28);
			} break;
			case 0x1F: { // RRA
				uint8_t c = z80->gpr[RF]&0x01;
				z80->gpr[RF] = (z80->gpr[RF]&0xC4)
					| ((z80->gpr[RA])&0x01);
				z80->gpr[RA] = ((c<<7)|((z80->gpr[RA]>>1)&0x7F));
				z80->gpr[RF] |= (z80->gpr[RA]&0x28);
			} break;

			case 0x27: { // DAA
				// HALP
				uint8_t a = z80->gpr[RA];
				uint8_t ah = a>>4;
				uint8_t al = a&15;
				uint8_t c = z80->gpr[RF]&0x01;
				uint8_t n = z80->gpr[RF]&0x02;
				uint8_t h = z80->gpr[RF]&0x10;
				uint8_t nf = c | n;

				// Get diff + C'
				uint8_t diff = 0;
				if(c != 0) {
					diff |= 0x60;
					if(h != 0 || al >= 0xA) {
						diff |= 0x06;
					}
				} else if(al >= 0xA) {
					diff |= 0x06;
					if(ah >= 0x9) {
						diff |= 0x60;
						nf |= 0x01;
					}
				} else {
					if(h != 0) {
						diff |= 0x06;
					}
					if(ah >= 0xA) {
						diff |= 0x60;
						nf |= 0x01;
					}
				}

				// Get H' + Calc result
				if(n == 0) {
					if(al >= 0xA) {
						nf |= 0x10;
					}
					a += diff;

				} else {
					if(h != 0 && al <= 0x5) {
						nf |= 0x10;
					}
					a -= diff;
				}

				// Calc flags
				nf |= (a&0xA8);
				nf |= z80_parity(a);
				nf |= (a == 0 ? 0x40 : 0x00);
				z80->gpr[RF] = nf;
				z80->gpr[RA] = a;
			} break;

			case 0x2F: // CPL
				z80->gpr[RA] ^= 0xFF;
				z80->gpr[RF] = (z80->gpr[RF]&0xC5)
					| (z80->gpr[RA]&0x28)
					| 0x12;
				break;

			case 0x37: // SCF
				z80->gpr[RF] = (z80->gpr[RF]&0xC4)
					| (z80->gpr[RA]&0x28)
					| 0x01;
				break;
			case 0x3F: // CCF
				z80->gpr[RF] = ((z80->gpr[RF]&0xC5)
					| ((z80->gpr[RF]&0x01)<<4)
					| (z80->gpr[RA]&0x28)) ^ 0x01;
				break;

			//
			// X=3
			//

			// Z=0
			case 0xC0: // RET NZ
				z80_op_ret_cond(z80, sms, (z80->gpr[RF]&0x40) == 0);
				break;
			case 0xC8: // RET Z
				z80_op_ret_cond(z80, sms, (z80->gpr[RF]&0x40) != 0);
				break;
			case 0xD0: // RET NC
				z80_op_ret_cond(z80, sms, (z80->gpr[RF]&0x01) == 0);
				break;
			case 0xD8: // RET C
				z80_op_ret_cond(z80, sms, (z80->gpr[RF]&0x01) != 0);
				break;
			case 0xE0: // RET PO
				z80_op_ret_cond(z80, sms, (z80->gpr[RF]&0x04) == 0);
				break;
			case 0xE8: // RET PE
				z80_op_ret_cond(z80, sms, (z80->gpr[RF]&0x04) != 0);
				break;
			case 0xF0: // RET P
				z80_op_ret_cond(z80, sms, (z80->gpr[RF]&0x80) == 0);
				break;
			case 0xF8: // RET M
				z80_op_ret_cond(z80, sms, (z80->gpr[RF]&0x80) != 0);
				break;

			// Z=1
			case 0xC1: // POP BC
				z80->gpr[RC] = z80_mem_read(sms, z80->timestamp, z80->sp++);
				Z80_ADD_CYCLES(z80, 3);
				z80->gpr[RB] = z80_mem_read(sms, z80->timestamp, z80->sp++);
				Z80_ADD_CYCLES(z80, 3);
				break;
			case 0xD1: // POP DE
				z80->gpr[RE] = z80_mem_read(sms, z80->timestamp, z80->sp++);
				Z80_ADD_CYCLES(z80, 3);
				z80->gpr[RD] = z80_mem_read(sms, z80->timestamp, z80->sp++);
				Z80_ADD_CYCLES(z80, 3);
				break;
			case 0xE1: if(ix >= 0) {
					// POP Iz
					z80->idx[ix&1][1] = z80_mem_read(sms, z80->timestamp, z80->sp++);
					Z80_ADD_CYCLES(z80, 3);
					z80->idx[ix&1][0] = z80_mem_read(sms, z80->timestamp, z80->sp++);
					Z80_ADD_CYCLES(z80, 3);
				} else {
					// POP HL
					z80->gpr[RL] = z80_mem_read(sms, z80->timestamp, z80->sp++);
					Z80_ADD_CYCLES(z80, 3);
					z80->gpr[RH] = z80_mem_read(sms, z80->timestamp, z80->sp++);
					Z80_ADD_CYCLES(z80, 3);
				} break;
			case 0xF1: // POP AF
				z80->gpr[RF] = z80_mem_read(sms, z80->timestamp, z80->sp++);
				Z80_ADD_CYCLES(z80, 3);
				z80->gpr[RA] = z80_mem_read(sms, z80->timestamp, z80->sp++);
				Z80_ADD_CYCLES(z80, 3);
				break;

			case 0xC9: // RET
				z80_op_ret(z80, sms);
				break;
			case 0xD9: { // EXX
				uint8_t t;
				t = z80->gpr[RB]; z80->gpr[RB] = z80->shadow[RB]; z80->shadow[RB] = t;
				t = z80->gpr[RC]; z80->gpr[RC] = z80->shadow[RC]; z80->shadow[RC] = t;
				t = z80->gpr[RD]; z80->gpr[RD] = z80->shadow[RD]; z80->shadow[RD] = t;
				t = z80->gpr[RE]; z80->gpr[RE] = z80->shadow[RE]; z80->shadow[RE] = t;
				t = z80->gpr[RH]; z80->gpr[RH] = z80->shadow[RH]; z80->shadow[RH] = t;
				t = z80->gpr[RL]; z80->gpr[RL] = z80->shadow[RL]; z80->shadow[RL] = t;

				// also cover WZ
				t = z80->wz[0];
				z80->wz[0] = z80->wz[2];
				z80->wz[2] = t;
				t = z80->wz[1];
				z80->wz[1] = z80->wz[3];
				z80->wz[3] = t;
			} break;

			case 0xE9: if(ix >= 0) {
					// JP (Iz)
					z80->pc = z80_pair_pbe(z80->idx[ix&1]);
				} else {
					// JP (HL)
					z80->pc = z80_pair_pbe(&z80->gpr[RH]);
				} break;
			case 0xF9: if(ix >= 0) {
					// LD SP, Iz
					z80->sp = z80_pair_pbe(z80->idx[ix&1]);
					Z80_ADD_CYCLES(z80, 2);
				} else {
					// LD SP, HL
					z80->sp = z80_pair_pbe(&z80->gpr[RH]);
					Z80_ADD_CYCLES(z80, 2);
				} break;

			// Z=2
			case 0xC2: // JP NZ, d
				z80_op_jp_cond(z80, sms, (z80->gpr[RF]&0x40) == 0);
				break;
			case 0xCA: // JP Z, d
				z80_op_jp_cond(z80, sms, (z80->gpr[RF]&0x40) != 0);
				break;
			case 0xD2: // JP NC, d
				z80_op_jp_cond(z80, sms, (z80->gpr[RF]&0x01) == 0);
				break;
			case 0xDA: // JP C, d
				z80_op_jp_cond(z80, sms, (z80->gpr[RF]&0x01) != 0);
				break;
			case 0xE2: // JP PO, d
				z80_op_jp_cond(z80, sms, (z80->gpr[RF]&0x04) == 0);
				break;
			case 0xEA: // JP PE, d
				z80_op_jp_cond(z80, sms, (z80->gpr[RF]&0x04) != 0);
				break;
			case 0xF2: // JP P, d
				z80_op_jp_cond(z80, sms, (z80->gpr[RF]&0x80) == 0);
				break;
			case 0xFA: // JP M, d
				z80_op_jp_cond(z80, sms, (z80->gpr[RF]&0x80) != 0);
				break;

			// Z=3
			case 0xC3: // JP nn
				z80_op_jp_cond(z80, sms, true);
				break;

			case 0xD3: { // OUT (n), A
				uint16_t port = z80_fetch_op_x(z80, sms);
				port &= 0x00FF;
				port |= (port << 8);
				//printf("IO WRITE %04X %02X [%04X]\n", port, z80->gpr[RA], z80->pc-2);
				z80_io_write(sms, z80->timestamp, port, z80->gpr[RA]);
				Z80_ADD_CYCLES(z80, 4);
			} break;
			case 0xDB: { // IN A, (n)
				uint16_t port = z80_fetch_op_x(z80, sms);
				port &= 0x00FF;
				port |= (port << 8);
				z80->gpr[RA] = z80_io_read(sms, z80->timestamp, port);
				//printf("IN %04X %02X [%04X]\n", port, z80->gpr[RA], z80->pc-2);
				z80->gpr[RF] = (z80->gpr[RF]&0x01)
					| (z80->gpr[RA]&0xA8)
					| (z80->gpr[RA] == 0 ? 0x40 : 0x00)
					| z80_parity(z80->gpr[RA]);
				Z80_ADD_CYCLES(z80, 4);
			} break;

			case 0xE3: if(ix >= 0) {
					// EX (SP), Iz
					// XXX: actual timing pattern is unknown
					uint8_t tl = z80_mem_read(sms, z80->timestamp, z80->sp+0);
					Z80_ADD_CYCLES(z80, 4);
					uint8_t th = z80_mem_read(sms, z80->timestamp, z80->sp+1);
					Z80_ADD_CYCLES(z80, 4);
					z80_mem_write(sms, z80->timestamp, z80->sp+0, z80->idx[ix&1][1]);
					Z80_ADD_CYCLES(z80, 4);
					z80_mem_write(sms, z80->timestamp, z80->sp+1, z80->idx[ix&1][0]);
					Z80_ADD_CYCLES(z80, 3);
					z80->idx[ix&1][1] = tl;
					z80->idx[ix&1][0] = th;

				} else {
					// EX (SP), HL
					// XXX: actual timing pattern is unknown
					uint8_t tl = z80_mem_read(sms, z80->timestamp, z80->sp+0);
					Z80_ADD_CYCLES(z80, 4);
					uint8_t th = z80_mem_read(sms, z80->timestamp, z80->sp+1);
					Z80_ADD_CYCLES(z80, 4);
					z80_mem_write(sms, z80->timestamp, z80->sp+0, z80->gpr[RL]);
					Z80_ADD_CYCLES(z80, 4);
					z80_mem_write(sms, z80->timestamp, z80->sp+1, z80->gpr[RH]);
					Z80_ADD_CYCLES(z80, 3);
					z80->gpr[RL] = tl;
					z80->gpr[RH] = th;
				} break;
			case 0xEB: { // EX DE, HL
				uint8_t t;
				t = z80->gpr[RH];
				z80->gpr[RH] = z80->gpr[RD];
				z80->gpr[RD] = t;
				t = z80->gpr[RL];
				z80->gpr[RL] = z80->gpr[RE];
				z80->gpr[RE] = t;
			} break;

			case 0xF3: // DI
				z80->iff1 = 0;
				z80->iff2 = 0;
				break;
			case 0xFB: // EI
				z80->iff1 = 1;
				z80->iff2 = 1;
				z80->noni = 1;
				break;

			// Z=4
			case 0xC4: // CALL NZ, d
				z80_op_call_cond(z80, sms, (z80->gpr[RF]&0x40) == 0);
				break;
			case 0xCC: // CALL Z, d
				z80_op_call_cond(z80, sms, (z80->gpr[RF]&0x40) != 0);
				break;
			case 0xD4: // CALL NC, d
				z80_op_call_cond(z80, sms, (z80->gpr[RF]&0x01) == 0);
				break;
			case 0xDC: // CALL C, d
				z80_op_call_cond(z80, sms, (z80->gpr[RF]&0x01) != 0);
				break;
			case 0xE4: // CALL PO, d
				z80_op_call_cond(z80, sms, (z80->gpr[RF]&0x04) == 0);
				break;
			case 0xEC: // CALL PE, d
				z80_op_call_cond(z80, sms, (z80->gpr[RF]&0x04) != 0);
				break;
			case 0xF4: // CALL P, d
				z80_op_call_cond(z80, sms, (z80->gpr[RF]&0x80) == 0);
				break;
			case 0xFC: // CALL M, d
				z80_op_call_cond(z80, sms, (z80->gpr[RF]&0x80) != 0);
				break;

			// Z=5
			case 0xC5: // PUSH BC
				z80_mem_write(sms, z80->timestamp, --z80->sp, z80->gpr[RB]);
				Z80_ADD_CYCLES(z80, 4);
				z80_mem_write(sms, z80->timestamp, --z80->sp, z80->gpr[RC]);
				Z80_ADD_CYCLES(z80, 3);
				break;
			case 0xD5: // PUSH DE
				z80_mem_write(sms, z80->timestamp, --z80->sp, z80->gpr[RD]);
				Z80_ADD_CYCLES(z80, 4);
				z80_mem_write(sms, z80->timestamp, --z80->sp, z80->gpr[RE]);
				Z80_ADD_CYCLES(z80, 3);
				break;
			case 0xE5: if(ix >= 0) {
					// PUSH Iz
					z80_mem_write(sms, z80->timestamp, --z80->sp, z80->idx[ix&1][0]);
					Z80_ADD_CYCLES(z80, 4);
					z80_mem_write(sms, z80->timestamp, --z80->sp, z80->idx[ix&1][1]);
					Z80_ADD_CYCLES(z80, 3);
				} else {
					// PUSH HL
					z80_mem_write(sms, z80->timestamp, --z80->sp, z80->gpr[RH]);
					Z80_ADD_CYCLES(z80, 4);
					z80_mem_write(sms, z80->timestamp, --z80->sp, z80->gpr[RL]);
					Z80_ADD_CYCLES(z80, 3);
				} break;
			case 0xF5: // PUSH AF
				z80_mem_write(sms, z80->timestamp, --z80->sp, z80->gpr[RA]);
				Z80_ADD_CYCLES(z80, 4);
				z80_mem_write(sms, z80->timestamp, --z80->sp, z80->gpr[RF]);
				Z80_ADD_CYCLES(z80, 3);
				break;

			case 0xCD: // CALL nn
				z80_op_call_cond(z80, sms, true);
				break;

			// Z=6
			case 0xC6: { // ADD A, n
				uint8_t val = z80_fetch_op_x(z80, sms);
				z80->gpr[RA] = z80_add8(z80, z80->gpr[RA], val);
			} break;
			case 0xCE: { // ADC A, n
				uint8_t val = z80_fetch_op_x(z80, sms);
				z80->gpr[RA] = z80_adc8(z80, z80->gpr[RA], val);
			} break;
			case 0xD6: { // SUB n
				uint8_t val = z80_fetch_op_x(z80, sms);
				z80->gpr[RA] = z80_sub8(z80, z80->gpr[RA], val);
			} break;
			case 0xDE: { // SBC A, n
				uint8_t val = z80_fetch_op_x(z80, sms);
				z80->gpr[RA] = z80_sbc8(z80, z80->gpr[RA], val);
			} break;
			case 0xE6: { // AND n
				uint8_t val = z80_fetch_op_x(z80, sms);
				z80->gpr[RA] = z80_and8(z80, z80->gpr[RA], val);
			} break;
			case 0xEE: { // XOR n
				uint8_t val = z80_fetch_op_x(z80, sms);
				z80->gpr[RA] = z80_xor8(z80, z80->gpr[RA], val);
			} break;
			case 0xF6: { // OR n
				uint8_t val = z80_fetch_op_x(z80, sms);
				z80->gpr[RA] = z80_or8(z80, z80->gpr[RA], val);
			} break;
			case 0xFE: { // CP n
				uint8_t val = z80_fetch_op_x(z80, sms);
				z80_sub8(z80, z80->gpr[RA], val);
				z80->gpr[RF] = (z80->gpr[RF]&~0x28) | (val&0x28);
			} break;

			// Z=7: RST
			case 0xC7: // RST $00
				z80_op_rst(z80, sms, 0x0000);
				break;
			case 0xCF: // RST $08
				z80_op_rst(z80, sms, 0x0008);
				break;
			case 0xD7: // RST $10
				z80_op_rst(z80, sms, 0x0010);
				break;
			case 0xDF: // RST $18
				z80_op_rst(z80, sms, 0x0018);
				break;
			case 0xE7: // RST $20
				z80_op_rst(z80, sms, 0x0020);
				break;
			case 0xEF: // RST $28
				z80_op_rst(z80, sms, 0x0028);
				break;
			case 0xF7: // RST $30
				z80_op_rst(z80, sms, 0x0030);
				break;
			case 0xFF: // RST $38
				z80_op_rst(z80, sms, 0x0038);
				break;

			default:
				// TODO!
				fprintf(stderr, "OP: %02X [%04X]\n", op, z80->pc-1);
				fflush(stderr); abort();
				break;
		}
	}
}

void z80_init(struct Z80 *z80)
{
	*z80 = (struct Z80){ .timestamp=0 };
	z80_reset(z80);
}

