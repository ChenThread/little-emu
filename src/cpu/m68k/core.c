//include "cpu/m68k/all.h"

// TODO: support prefetch buffer

void M68KNAME(reset)(struct M68K *m68k)
{
	m68k->needs_reset = 1;
}

static uint16_t m68k_read_16(struct M68K *m68k, M68K_STATE_PARAMS, uint32_t addr)
{
	uint16_t val = M68KNAME(mem_read)(M68K_STATE_ARGS, m68k->H.timestamp, addr);
	if((addr&1) != 0) {
		val = (val<<8)|(val>>8);
	}
	M68K_ADD_CYCLES(m68k, 4);
	return val;
}

static uint32_t m68k_read_32(struct M68K *m68k, M68K_STATE_PARAMS, uint32_t addr)
{
	uint32_t val_h = m68k_read_16(m68k, M68K_STATE_ARGS, addr+0);
	uint32_t val_l = m68k_read_16(m68k, M68K_STATE_ARGS, addr+2);
	return (val_h<<16)|(val_l&0xFFFF);
}

static void m68k_write_8(struct M68K *m68k, M68K_STATE_PARAMS, uint32_t addr, uint8_t val)
{
	uint16_t aval = val;
	aval *= 0x0101;
	uint16_t amask = (0xFF00>>((addr&1)<<3));
	M68KNAME(mem_write)(M68K_STATE_ARGS, m68k->H.timestamp, addr, aval, amask);
	M68K_ADD_CYCLES(m68k, 4);
}

static void m68k_write_16(struct M68K *m68k, M68K_STATE_PARAMS, uint32_t addr, uint16_t val)
{
	uint16_t aval = val;
	if((addr&1) != 0) {
		aval = (aval<<8)|(aval>>8);
	}
	uint16_t amask = 0xFFFF;
	M68KNAME(mem_write)(M68K_STATE_ARGS, m68k->H.timestamp, addr, aval, amask);
	M68K_ADD_CYCLES(m68k, 4);
}

static void m68k_write_32(struct M68K *m68k, M68K_STATE_PARAMS, uint32_t addr, uint32_t val)
{
	m68k_write_16(m68k, M68K_STATE_ARGS, addr+0, (uint16_t)(val>>16));
	m68k_write_16(m68k, M68K_STATE_ARGS, addr+2, (uint16_t)val);
}

static uint16_t m68k_fetch_op_16(struct M68K *m68k, M68K_STATE_PARAMS)
{
	uint16_t op = m68k_read_16(m68k, M68K_STATE_ARGS, m68k->pc);
	m68k->pc += 2;
	return op;
}

static uint32_t m68k_fetch_op_32(struct M68K *m68k, M68K_STATE_PARAMS)
{
	uint32_t op = m68k_read_32(m68k, M68K_STATE_ARGS, m68k->pc);
	m68k->pc += 4;
	return op;
}

void M68KNAME(irq)(struct M68K *m68k, M68K_STATE_PARAMS)
{
	// TODO!
}

static bool m68k_cc4_true(struct M68K *m68k, int cond)
{
	bool base_cond = ((cond&1) == 0);
	switch(cond>>1) {
		case 0x0: // T (F)
			return base_cond == (true);
		case 0x1: // HI (LS)
			return base_cond == ((m68k->sr&(F_C|F_Z))==0);
		case 0x2: // CC (CS)
			return base_cond == ((m68k->sr&F_C)==0);
		case 0x3: // NE (EQ)
			return base_cond == ((m68k->sr&F_Z)==0);
		case 0x4: // VC (VS)
			return base_cond == ((m68k->sr&F_V)==0);
		case 0x5: // PL (MI)
			return base_cond == ((m68k->sr&F_N)==0);
		case 0x6: // GE (LT)
			return base_cond == (((m68k->sr&F_N)==0)==((m68k->sr&F_V)==0));
		case 0x7: // GT (LE)
			return base_cond == (((m68k->sr&F_N)==0)==((m68k->sr&F_V)==0)
				&&(m68k->sr&F_Z)==0);
		default:
			assert(!"unreachable");
			return false;
	}
}

static bool m68k_allow_ea(struct M68K *m68k, uint32_t eavals, uint32_t allow)
{
	uint32_t ea_mode = (eavals>>3)&7;
	uint32_t ea_reg = eavals&7;

	bool cond = (ea_mode != 7
		? (((0x0001<<ea_mode)&allow) != 0)
		: (((0x0100<<ea_reg)&allow) != 0)
	);

	return cond;
}

static bool m68k_ea_calc(struct M68K *m68k, M68K_STATE_PARAMS, uint32_t eavals, int size_bytes)
{
	uint32_t ea_mode = (eavals>>3)&7;
	uint32_t ea_reg = eavals&7;

	switch(ea_mode) {
		case 0x0: // Dn
			m68k->last_non_ea = m68k->rd[ea_reg];
			return false;
		case 0x1: // An
			m68k->last_non_ea = m68k->ra[ea_reg];
			return false;
		case 0x2: // (An)
			m68k->last_ea = m68k->ra[ea_reg];
			break;
		case 0x3: // (An)+
			m68k->last_ea = m68k->ra[ea_reg];
			m68k->ra[ea_reg] += 2;
			break;
		case 0x4: // -(An)
			M68K_ADD_CYCLES(m68k, 2);
			m68k->ra[ea_reg] -= 2;
			m68k->last_ea = m68k->ra[ea_reg];
			break;
		case 0x5: // (d16,An)
			assert(!"HALP (d16,An)");
			break;
		case 0x6: // (d8,An,Xn)
			assert(!"HALP (d8,An,Xn)");
			break;

		case 0x7: // lots of things
		switch(ea_reg) {
			case 0x0: // (xxx).W
				m68k->last_ea = (uint32_t)(int32_t)(int16_t)m68k_fetch_op_16(m68k, M68K_STATE_ARGS);
				break;
			case 0x1: // (xxx).L
				m68k->last_ea = m68k_fetch_op_32(m68k, M68K_STATE_ARGS);
				break;
			case 0x2: // (d16,PC)
				m68k->last_ea = (uint32_t)(int32_t)(int16_t)m68k_fetch_op_16(m68k, M68K_STATE_ARGS);
				m68k->last_ea += m68k->pc-2;
				break;
			case 0x4: // #xxx
				m68k->last_ea = m68k->pc;
				m68k->pc += size_bytes;
				break;

			default:
				assert(!"missing ext EA");
				break;
		} break;
	}

	return true;
}

static uint8_t m68k_ea_read_8(struct M68K *m68k, M68K_STATE_PARAMS, uint32_t eavals)
{
	if(m68k_ea_calc(m68k, M68K_STATE_ARGS, eavals, 2)) {
		// had EA
		return m68k_read_16(m68k, M68K_STATE_ARGS, m68k->last_ea);

	} else {
		// only had reg
		return m68k->last_non_ea;

	}
}

static uint16_t m68k_ea_read_16(struct M68K *m68k, M68K_STATE_PARAMS, uint32_t eavals)
{
	if(m68k_ea_calc(m68k, M68K_STATE_ARGS, eavals, 2)) {
		// had EA
		return m68k_read_16(m68k, M68K_STATE_ARGS, m68k->last_ea);

	} else {
		// only had reg
		return m68k->last_non_ea;
	}
}

static uint16_t m68k_ea_read_32(struct M68K *m68k, M68K_STATE_PARAMS, uint32_t eavals)
{
	if(m68k_ea_calc(m68k, M68K_STATE_ARGS, eavals, 4)) {
		// had EA
		return m68k_read_32(m68k, M68K_STATE_ARGS, m68k->last_ea);

	} else {
		// only had reg
		return m68k->last_non_ea;
	}
}

static void m68k_grp_0x1(struct M68K *m68k, M68K_STATE_PARAMS, uint16_t op)
{
	//printf("%08X: %04X (grp 0x1 move.b)\n", m68k->pc-2, op);
	if(!m68k_allow_ea(m68k, ((op>>6)&0x3F), 0x03FD)) {
		assert(!"invalid dest EA");
	}
	if(!m68k_allow_ea(m68k, (op&0x3F), 0x1FFD)) {
		assert(!"invalid source EA");
	}

	uint8_t val = m68k_ea_read_8(m68k, M68K_STATE_ARGS, op&0x3F);
	//printf("read %02X\n", val);
	if(m68k_ea_calc(m68k, M68K_STATE_ARGS, (op>>6)&0x3F, 2)) {
		m68k_write_8(m68k, M68K_STATE_ARGS, m68k->last_ea, val);

	} else if(((op>>9)&0x6) == 0) {
		m68k->rd[(op>>6)&0x7] &= ~0xFF;
		m68k->rd[(op>>6)&0x7] |= val;

	} else {
		assert(!"expected D reg for move dest");
	}
	//m68k->halted = 1;
	//m68k->H.timestamp = m68k->H.timestamp_end;
}

static void m68k_grp_0x2(struct M68K *m68k, M68K_STATE_PARAMS, uint16_t op)
{
	// TODO work out cycles on failure
	if(!m68k_allow_ea(m68k, ((op>>6)&0x3F), 0x03FF)) {
		assert(!"invalid dest EA");
	}
	if(!m68k_allow_ea(m68k, (op&0x3F), 0x1FFF)) {
		assert(!"invalid source EA");
	}

	printf("%08X: %04X (grp 0x2 move.l)\n", m68k->pc-2, op);
	uint32_t val = m68k_ea_read_32(m68k, M68K_STATE_ARGS, op&0x3F);
	printf("read %08X\n", val);
	m68k->halted = 1;
	m68k->H.timestamp = m68k->H.timestamp_end;
}

static void m68k_grp_0x3(struct M68K *m68k, M68K_STATE_PARAMS, uint16_t op)
{
	if(!m68k_allow_ea(m68k, ((op>>6)&0x3F), 0x03FF)) {
		assert(!"invalid dest EA");
	}
	if(!m68k_allow_ea(m68k, (op&0x3F), 0x1FFF)) {
		assert(!"invalid source EA");
	}
	printf("%08X: %04X (grp 0x3 move.w)\n", m68k->pc-2, op);
	uint16_t val = m68k_ea_read_16(m68k, M68K_STATE_ARGS, op&0x3F);
	printf("read %04X\n", val);
	m68k->halted = 1;
	m68k->H.timestamp = m68k->H.timestamp_end;
}

static void m68k_grp_0x4(struct M68K *m68k, M68K_STATE_PARAMS, uint16_t op)
{
	// Misc block contents:
	// . - 4000 00FF = negx.S {d|a}
	// A - 40C0 003F = move.w sr, {d|a}
	// . - 4200 00FF = clr.S {d|a}
	// A - 42C0 003F = move.b ccr, {d|a}
	// . - 4400 00FF = neg.S {d|a}
	// A - 44C0 003F = move.b {d|a}, ccr
	// . - 4600 00FF = not.S {d|a}
	// A - 46C0 003F = move.w {d|a}, sr
	// ! - 4800 01C7 = ext/extb [ CODEPENDENT WITH NBCD ]
	// ! - 4800 003F = nbcd [ CODEPENDENT WITH EXT ]
	// . - 4808 0007 [+2w] = link LONG
	// . - 4840 003F = pea
	// A - 4840 0007 = swap
	// A - 4848 0007 = bpkt #b3
	// . - 4AFA 0000 = bgnd
	// . - 4AFC 0000 = illegal
	// . - 4A00 00FF = tst
	// A - 4AC0 003F = tas
	// . - 4C00 003F | 0000 7407 = mulu LONG
	// . - 4C00 003F | 0800 7407 = muls LONG
	// . - 4C08 003F | 0000 7407 = divu/divul LONG
	// . - 4C08 003F | 0800 7407 = divs/divsl LONG
	// . - 4E40 000F = trap #b4
	// . - 4E50 0007 [+1w] = link WORD
	// . - 4E58 0007 = unlk
	// . - 4E60 000F = move usp
	// . - 4E70 0000 = reset
	// . - 4E71 0000 = nop
	// . - 4E72 0000 [+1w] = stop #imm
	// . - 4E73 0000 = rte
	// . - 4E74 0000 [+1w] = rtd #imm
	// . - 4E75 0000 = rts
	// . - 4E76 0000 = trapv
	// . - 4E77 0000 = rtr
	// . - 4E7A 0001 [+1w] = movec
	// . - 4E80 003F = jsr
	// . - 4EC0 003F = jmp
	// . - 4880 047F = movem
	// . - 41C0 0E3F = lea
	// . - 4100 0FBF = chk
	// 

	if((op&~0xE3F) == 0x41C0) {
		// LEA
		if(!m68k_allow_ea(m68k, (op&0x3F), 0x0FE4)) {
			assert(!"invalid source EA");
		}
		if(!m68k_ea_calc(m68k, M68K_STATE_ARGS, (op&0x3F), 4)) {
			printf("%08X: %04X (grp 0x4)\n", m68k->pc-2, op);
			assert(!"unexpected source EA for LEA");
		}
		uint32_t ea = m68k->last_ea;
		m68k->ra[(op>>9)&7] = ea;

	} else if((op&~0xFBF) == 0x4100) {
		// CHK
		printf("%08X: %04X (grp 0x4)\n", m68k->pc-2, op);
		printf("chk ops\n");
		m68k->halted = 1;
		m68k->H.timestamp = m68k->H.timestamp_end;

	} else if(false) {
		// TODO: movem

	} else switch((op>>9)&7) {
		case 0x3: switch((op>>6)&3) {
			case 0x3: {
				assert(((op>>3)&7) != 1); // TODO make this a cpu trap instead
				uint16_t val = m68k_ea_read_16(m68k, M68K_STATE_ARGS, op&0x3F);
				assert((m68k->sr&0x2000) != 0); // TODO trap instead
				m68k->sr = val;
			} break;

			default: {
				printf("%08X: %04X (grp 0x4)\n", m68k->pc-2, op);
				printf("not.S op\n");
				m68k->halted = 1;
				m68k->H.timestamp = m68k->H.timestamp_end;
			} break;

		} break;

		case 0x5: switch((op>>6)&3) {
			case 0x0: {
				// TST.b
				if(!m68k_allow_ea(m68k, (op&0x3F), 0x03FD)) {
					assert(!"invalid source EA");
				}
				uint8_t val = m68k_ea_read_8(m68k, M68K_STATE_ARGS, op&0x3F);
				m68k->sr &= ~(F_N|F_Z|F_V|F_C);
				m68k->sr |= ((val&0x80) != 0 ? F_N : 0);
				m68k->sr |= (val == 0 ? F_Z : 0);
			} break;

			case 0x1: {
				// TST.w
				if(!m68k_allow_ea(m68k, (op&0x3F), 0x03FD)) {
					assert(!"invalid source EA");
				}
				uint16_t val = m68k_ea_read_16(m68k, M68K_STATE_ARGS, op&0x3F);
				m68k->sr &= ~(F_N|F_Z|F_V|F_C);
				m68k->sr |= ((val&0x8000) != 0 ? F_N : 0);
				m68k->sr |= (val == 0 ? F_Z : 0);
			} break;

			case 0x2: {
				// TST.l
				if(!m68k_allow_ea(m68k, (op&0x3F), 0x03FD)) {
					assert(!"invalid source EA");
				}
				uint32_t val = m68k_ea_read_32(m68k, M68K_STATE_ARGS, op&0x3F);
				m68k->sr &= ~(F_N|F_Z|F_V|F_C);
				m68k->sr |= ((val&0x80000000) != 0 ? F_N : 0);
				m68k->sr |= (val == 0 ? F_Z : 0);
			} break;

			case 0x3: {
				// TAS
				printf("%08X: %04X (grp 0x4)\n", m68k->pc-2, op);
				printf("TAS op\n");
				m68k->halted = 1;
				m68k->H.timestamp = m68k->H.timestamp_end;
			} break;
		} break;

		default:
			printf("%08X: %04X (grp 0x4)\n", m68k->pc-2, op);
			m68k->halted = 1;
			m68k->H.timestamp = m68k->H.timestamp_end;
			break;
	}

}

static void m68k_grp_0x6(struct M68K *m68k, M68K_STATE_PARAMS, uint16_t op)
{
	// Branches
	// TODO: find out BSR memory access order
	// TODO: find out if second word is read even on failure
	int cond = (op>>8)&0xF;
	uint32_t offs = (uint32_t)(int32_t)(int8_t)(op&0xFF);
	bool is_bsr = (cond == 1);
	bool is_word_offs = (offs == 0);
	bool cond_pass = (is_bsr || m68k_cc4_true(m68k, cond));

	// TODO: BSR
	if(is_bsr) {
		printf("%08X: %04X (BSR, grp 0x6 branches)\n", m68k->pc-2, op);
		m68k->halted = 1;
		m68k->H.timestamp = m68k->H.timestamp_end;
		return;
	}

	if(is_word_offs) {
		offs = (uint32_t)(int32_t)(int8_t)m68k_fetch_op_16(m68k, M68K_STATE_ARGS);
	}

	if(cond_pass) {
		m68k->pc += offs;
		assert((m68k->pc&1) == 0);

		if(is_word_offs) {
			M68K_ADD_CYCLES(m68k, 2);
		} else {
			M68K_ADD_CYCLES(m68k, 6);
		}

	} else {
		M68K_ADD_CYCLES(m68k, 4);
	}
}

void M68KNAME(run)(struct M68K *m68k, M68K_STATE_PARAMS, uint64_t timestamp)
{
	// Don't jump back into the past
	if(!TIME_IN_ORDER(m68k->H.timestamp, timestamp)) {
		return;
	}

	// Apply reset if we requested it
	if(m68k->needs_reset) {
		// XXX: the timing here is a guess
		m68k->sr = 0x2700;
		M68K_ADD_CYCLES(m68k, 4);
		m68k->ra[7] = m68k_read_32(m68k, M68K_STATE_ARGS, 0);
		m68k->pc = m68k_read_32(m68k, M68K_STATE_ARGS, 4);
		printf("RESET: PC=%08X, SP=%08X\n", m68k->pc, m68k->ra[7]);
		m68k->needs_reset = 0;
	}

	// If halted, don't waste time fetching ops
	if(m68k->halted) {
		/*
		if(m68k->iff1 != 0 && M68K_INT_CHECK) {
			M68KNAME(irq)(m68k, M68K_STATE_ARGS, 0xFF);
		} else {
			while(TIME_IN_ORDER(m68k->H.timestamp, timestamp)) {
				M68K_ADD_CYCLES(m68k, 4);
				m68k->r = (m68k->r&0x80) + ((m68k->r+1)&0x7F);
			}
			//m68k->H.timestamp = timestamp;
			return;
		}
		*/
		m68k->H.timestamp_end = timestamp;
		m68k->H.timestamp = m68k->H.timestamp_end;
		return;
	}

	// Run ops
	//uint64_t lstamp = m68k->H.timestamp;
	m68k->H.timestamp_end = timestamp;
	while(TIME_IN_ORDER(m68k->H.timestamp, m68k->H.timestamp_end)) {
		//lstamp = m68k->H.timestamp;
		uint16_t op = m68k_fetch_op_16(m68k, M68K_STATE_ARGS);
		switch(op>>12) {
			case 0x1:
				m68k_grp_0x1(m68k, M68K_STATE_ARGS, op);
				break;

			case 0x2:
				m68k_grp_0x2(m68k, M68K_STATE_ARGS, op);
				break;

			case 0x3:
				m68k_grp_0x3(m68k, M68K_STATE_ARGS, op);
				break;

			case 0x4:
				m68k_grp_0x4(m68k, M68K_STATE_ARGS, op);
				break;

			case 0x6:
				m68k_grp_0x6(m68k, M68K_STATE_ARGS, op);
				break;

			default:
				printf("%08X: %04X\n", m68k->pc-2, op);
				m68k->halted = 1;
				m68k->H.timestamp = m68k->H.timestamp_end;
				break;
		}
	}
}

void M68KNAME(init)(struct EmuGlobal *H, struct M68K *m68k)
{
	*m68k = (struct M68K){ .H={.timestamp=0,}, };
	M68KNAME(reset)(m68k);
}

