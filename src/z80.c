#include "common.h"

static void z80_mem_write(struct SMS *sms, uint64_t timestamp, uint16_t addr, uint8_t val)
{
	if(addr >= 0xC000) {
		sms->ram[addr&0x1FFF] = val;
	}

	if(addr >= 0xFFFC) {
		sms->paging[addr&3] = val;
	}
}

static uint8_t z80_mem_read(struct SMS *sms, uint64_t timestamp, uint16_t addr)
{
	if(addr >= 0xC000) {
		return sms->ram[addr&0x1FFF];
	} else if(addr < 0x0400) {
		return sms_rom[addr];
	} else {
		return sms_rom[((uint32_t)(addr&0x3FFF))
			+((sms->paging[(addr>>14)+1]&0x1F)<<14)];
	}
}

static void z80_io_write(struct SMS *sms, uint64_t timestamp, uint16_t addr, uint8_t val)
{
	int port = ((addr>>5)&6)|(addr&1);

	switch(port)
	{
		case 0: // Memory control
			// TODO!
			break;

		case 1: // I/O port control
			// TODO!
			break;

		case 2: // PSG / V counter
		case 3: // PSG / H counter
			// TODO!
			break;

		case 4: // VDP data
			// TODO!
			break;

		case 5: // VDP control
			// TODO!
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
			return 0xAB;

		case 1: // I/O port control
			// TODO!
			return 0xFF;

		case 2: // V counter
			// TODO!
			return 0xFF;

		case 3: // H counter
			// TODO!
			return 0xFF;

		case 4: // VDP data
			// TODO!
			return 0xFF;

		case 5: // VDP control
			// TODO!
			return 0xFF;

		case 6: // I/O port A
			return sms_input_fetch(sms, timestamp, 0);

		case 7: // I/O port B
			return sms_input_fetch(sms, timestamp, 1);

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
	z80->noni = 0;
	Z80_ADD_CYCLES(z80, 3);
}

uint8_t z80_fetch_op_m1(struct Z80 *z80, struct SMS *sms)
{
	uint8_t op = z80_mem_read(sms, z80->timestamp, z80->pc++);
	Z80_ADD_CYCLES(z80, 4);
	return op;
}

uint8_t z80_fetch_op_x(struct Z80 *z80, struct SMS *sms)
{
	uint8_t op = z80_mem_read(sms, z80->timestamp, z80->pc++);
	Z80_ADD_CYCLES(z80, 3);
	return op;
}

static uint8_t z80_parity(uint8_t r)
{
	r ^= (r>>4);
	r ^= (r>>2);
	r ^= (r<<1);
	return r&0x02;
}

static uint8_t z80_add8(struct Z80 *z80, uint8_t a, uint8_t b)
{
	uint8_t r = a+b;
	uint8_t h = ((a&0xF)+(b&0xF))&0x10;
	uint8_t c = (r < a ? 0x01 : 0x00);
	uint8_t z = (r == 0 ? 0x40 : 0x00);
	uint8_t v = 0; // TODO!
	z80->gpr[RF] = (r&0xA8) | h | c | v | z;
	return r;
}

static uint8_t z80_adc8(struct Z80 *z80, uint8_t a, uint8_t b)
{
	if((z80->gpr[RF]&0x01) != 0 && b == 0xFF) {
		// FIXME
		z80_add8(z80, a, 0);
		z80->gpr[RF] |= 0x01;
		return a;
	} else {
		return z80_add8(z80, a, b+(z80->gpr[RF]&0x01));
	}
}

static uint8_t z80_sub8(struct Z80 *z80, uint8_t a, uint8_t b)
{
	uint8_t r = a-b;
	uint8_t h = ((a&0xF)-(b&0xF))&0x10;
	uint8_t c = (r > a ? 0x01 : 0x00);
	uint8_t z = (r == 0 ? 0x40 : 0x00);
	uint8_t v = 0; // TODO!
	z80->gpr[RF] = (r&0xA8) | h | c | v | z | 0x02;
	return r;
}

static uint8_t z80_sbc8(struct Z80 *z80, uint8_t a, uint8_t b)
{
	if((z80->gpr[RF]&0x01) != 0 && b == 0xFF) {
		// FIXME
		z80_sub8(z80, a, 0);
		z80->gpr[RF] |= 0x01;
		return a;
	} else {
		return z80_sub8(z80, a, b+(z80->gpr[RF]&0x01));
	}
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

static void z80_op_jr_cond(struct Z80 *z80, struct SMS *sms, bool cond)
{
	uint16_t offs = (uint16_t)(int16_t)(int8_t)z80_fetch_op_x(z80, sms);
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

static void z80_op_call_cond(struct Z80 *z80, struct SMS *sms, bool cond)
{
	uint16_t pcl = (uint16_t)z80_fetch_op_x(z80, sms);
	uint16_t pch = (uint16_t)z80_fetch_op_x(z80, sms);
	if(cond) {
		z80_mem_write(sms, z80->timestamp, --z80->sp, (uint8_t)(z80->pc>>0));
		Z80_ADD_CYCLES(z80, 4);
		z80_mem_write(sms, z80->timestamp, --z80->sp, (uint8_t)(z80->pc>>8));
		Z80_ADD_CYCLES(z80, 3);
		z80->pc = pcl+(pch<<8);
	}
}

void z80_run(struct Z80 *z80, struct SMS *sms, uint64_t timestamp)
{
	// Don't jump back into the past
	if(!TIME_IN_ORDER(z80->timestamp, timestamp)) {
		return;
	}

	// If halted, don't waste time fetching ops
	if(z80->halted) {
		z80->timestamp = timestamp;
		return;
	}

	// Run ops
	uint64_t lstamp = z80->timestamp;
	while(z80->timestamp < timestamp) {
		printf("%020lld: %04X: %02X: A=%02X\n"
			, (unsigned long long)((z80->timestamp-lstamp)/(uint64_t)3)
			, z80->pc, z80->gpr[RF], z80->gpr[RA]);

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
			// Split by X
			switch(op>>6) {
				case 0:
				case 3:
					// TODO: NONI
					break;

				case 1: switch(op&7) {
					case 6:
						// IM
						z80->im = (op>>3)&3;
						break;

					default:
						// TODO!
						fprintf(stderr, "OP: ED %02X X=1\n", op);
						fflush(stderr); abort();
						break;
				} break;

				case 2:
					// TODO!
					fprintf(stderr, "OP: ED %02X X=2\n", op);
					fflush(stderr); abort();
					break;

				default:
					fprintf(stderr, "UNREACHABLE OP: ED %02X\n", op);
					fflush(stderr); abort();
					break;
			} continue;
		}

		// Decode
		switch(op) {
			//
			// X=0
			//

			// Z=0
			case 0x18: // JR d
				z80_op_jr_cond(z80, sms, true);
				break;
			case 0x20: // JR NZ, d
				z80_op_jr_cond(z80, sms, (z80->gpr[RF]&0x40) != 0);
				break;
			case 0x28: // JR Z, d
				z80_op_jr_cond(z80, sms, (z80->gpr[RF]&0x40) == 0);
				break;
			case 0x30: // JR NC, d
				z80_op_jr_cond(z80, sms, (z80->gpr[RF]&0x01) != 0);
				break;
			case 0x38: // JR C, d
				z80_op_jr_cond(z80, sms, (z80->gpr[RF]&0x01) == 0);
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
				z80->sp = spl+(sph<<8);
			} break;

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
					// 
					uint16_t addr = (((uint16_t)z80->idx[ix&1][0])<<8)
						+((uint16_t)z80->idx[ix&1][1]);
					addr += (uint16_t)(int16_t)(int8_t)z80_fetch_op_x(z80, sms);
					Z80_ADD_CYCLES(z80, 6);
					z80_mem_write(sms, z80->timestamp,
						addr,
						z80_fetch_op_x(z80, sms));
					Z80_ADD_CYCLES(z80, 3);
			
				} else {
					// LD (HL), n
					z80_mem_write(sms, z80->timestamp,
						(((uint16_t)z80->gpr[RH])<<8)+((uint16_t)z80->gpr[RL]),
						z80_fetch_op_x(z80, sms));
					Z80_ADD_CYCLES(z80, 3);
				} break;
			case 0x3E: // LD A, n
				z80->gpr[RA] = z80_fetch_op_x(z80, sms);
				break;

			// X=3
			//

			// Z=3
			case 0xC3: // JP nn
				z80_op_jp_cond(z80, sms, true);
				break;

			case 0xD3: { // OUT (n), A
				uint16_t port = z80_fetch_op_x(z80, sms);
				port &= 0x00FF;
				port |= (port << 8);
				//printf("IO WRITE %04X %02X\n", port, z80->gpr[RA]);
				z80_io_write(sms, z80->timestamp, port, z80->gpr[RA]);
				Z80_ADD_CYCLES(z80, 4);
			} break;
			case 0xDB: { // IN A, (n)
				uint16_t port = z80_fetch_op_x(z80, sms);
				port &= 0x00FF;
				port |= (port << 8);
				z80->gpr[RA] = z80_io_read(sms, z80->timestamp, port);
				Z80_ADD_CYCLES(z80, 4);
			} break;

			case 0xF3: // DI
				z80->iff1 = 0;
				z80->iff2 = 0;
				break;
			case 0xFB: // EI
				z80->iff1 = 1;
				z80->iff2 = 1;
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
			case 0xC6: {// ADD A, n
				uint8_t imm = z80_fetch_op_x(z80, sms);
				z80->gpr[RA] = z80_add8(z80, z80->gpr[RA], imm);
			} break;
			case 0xCE: { // ADC A, n
				uint8_t imm = z80_fetch_op_x(z80, sms);
				z80->gpr[RA] = z80_adc8(z80, z80->gpr[RA], imm);
			} break;
			case 0xD6: { // SUB n
				uint8_t imm = z80_fetch_op_x(z80, sms);
				z80->gpr[RA] = z80_sub8(z80, z80->gpr[RA], imm);
			} break;
			case 0xDE: { // SBC A, n
				uint8_t imm = z80_fetch_op_x(z80, sms);
				z80->gpr[RA] = z80_sbc8(z80, z80->gpr[RA], imm);
			} break;
			case 0xE6: { // AND n
				uint8_t imm = z80_fetch_op_x(z80, sms);
				z80->gpr[RA] = z80_and8(z80, z80->gpr[RA], imm);
			} break;
			case 0xEE: { // XOR n
				uint8_t imm = z80_fetch_op_x(z80, sms);
				z80->gpr[RA] = z80_xor8(z80, z80->gpr[RA], imm);
			} break;
			case 0xF6: { // OR n
				uint8_t imm = z80_fetch_op_x(z80, sms);
				z80->gpr[RA] = z80_or8(z80, z80->gpr[RA], imm);
			} break;
			case 0xFE: { // CP n
				uint8_t imm = z80_fetch_op_x(z80, sms);
				z80_sub8(z80, z80->gpr[RA], imm);
				z80->gpr[RF] = (z80->gpr[RF]&~0x28)
					| (imm&0x28);
			} break;

			default:
				// TODO!
				fprintf(stderr, "OP: %02X\n", op);
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

