//include "cpu/m68k/all.h"

// TODO: support prefetch buffer

void M68KNAME(reset)(struct M68K *m68k)
{
	m68k->needs_reset = 1;
}

static void m68k_set_sr(struct M68K *m68k, M68K_STATE_PARAMS, uint16_t new_sr)
{
	uint16_t old_sr = m68k->sr;
	m68k->sr = new_sr;

	if((old_sr&0x2000) == 0 && (new_sr&0x2000) != 0) {
		// entering supervisor mode
		uint32_t t = m68k->ra[7];
		m68k->ra[7] = m68k->usp_store;
		m68k->usp_store = t;

	} else if((old_sr&0x2000) != 0 && (new_sr&0x2000) == 0) {
		// exiting supervisor mode
		uint32_t t = m68k->ra[7];
		m68k->ra[7] = m68k->usp_store;
		m68k->usp_store = t;

	}
}

static uint8_t m68k_read_8(struct M68K *m68k, M68K_STATE_PARAMS, uint32_t addr)
{
	uint16_t val = M68KNAME(mem_read)(M68K_STATE_ARGS, m68k->H.timestamp, addr);
	if((addr&1) == 0) {
		val >>= 8;
	}
	M68K_ADD_CYCLES(m68k, 4);
	return (uint8_t)val;
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

static bool m68k_allow_ea_src(struct M68K *m68k, uint32_t eavals, uint32_t allow)
{
	uint32_t ea_mode = (eavals>>3)&7;
	uint32_t ea_reg = eavals&7;

	bool cond = (ea_mode != 7
		? (((0x0001<<ea_mode)&allow) != 0)
		: (((0x0100<<ea_reg)&allow) != 0)
	);

	return cond;
}

static bool m68k_allow_ea_dst(struct M68K *m68k, uint32_t eavals, uint32_t allow)
{
	uint32_t ea_reg = (eavals>>9)&7;
	uint32_t ea_mode = (eavals>>6)&7;

	bool cond = (ea_mode != 7
		? (((0x0001<<ea_mode)&allow) != 0)
		: (((0x0100<<ea_reg)&allow) != 0)
	);

	return cond;
}

static bool m68k_ea_calc_split(struct M68K *m68k, M68K_STATE_PARAMS, uint32_t ea_mode, uint32_t ea_reg, int size_bytes)
{
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
			m68k->ra[ea_reg] += size_bytes;
			break;
		case 0x4: // -(An)
			M68K_ADD_CYCLES(m68k, 2);
			m68k->ra[ea_reg] -= size_bytes;
			m68k->last_ea = m68k->ra[ea_reg];
			break;
		case 0x5: // (d16,An)
			m68k->last_ea = (uint32_t)(int32_t)(int16_t)m68k_fetch_op_16(m68k, M68K_STATE_ARGS);
			m68k->last_ea += m68k->ra[ea_reg];
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
				m68k->pc += ((size_bytes+0x1)&~0x1);
				break;

			default:
				assert(!"missing ext EA");
				break;
		} break;
	}

	return true;
}

static bool m68k_ea_calc_src(struct M68K *m68k, M68K_STATE_PARAMS, uint32_t eavals, int size_bytes)
{
	uint32_t ea_mode = (eavals>>3)&7;
	uint32_t ea_reg = eavals&7;

	return m68k_ea_calc_split(m68k, M68K_STATE_ARGS, ea_mode, ea_reg, size_bytes);
}

static bool m68k_ea_calc_dst(struct M68K *m68k, M68K_STATE_PARAMS, uint32_t eavals, int size_bytes)
{
	// yes, they're swapped!
	// thank you Motorola | sed sethanefuce
	uint32_t ea_reg = (eavals>>9)&7;
	uint32_t ea_mode = (eavals>>6)&7;

	return m68k_ea_calc_split(m68k, M68K_STATE_ARGS, ea_mode, ea_reg, size_bytes);
}

static uint8_t m68k_ea_read_8(struct M68K *m68k, M68K_STATE_PARAMS, uint32_t eavals)
{
	if(m68k_ea_calc_src(m68k, M68K_STATE_ARGS, eavals, 1)) {
		// had EA
		return m68k_read_16(m68k, M68K_STATE_ARGS, m68k->last_ea);

	} else {
		// only had reg
		return m68k->last_non_ea;

	}
}

static uint16_t m68k_ea_read_16(struct M68K *m68k, M68K_STATE_PARAMS, uint32_t eavals)
{
	if(m68k_ea_calc_src(m68k, M68K_STATE_ARGS, eavals, 2)) {
		// had EA
		return m68k_read_16(m68k, M68K_STATE_ARGS, m68k->last_ea);

	} else {
		// only had reg
		return m68k->last_non_ea;
	}
}

static uint16_t m68k_ea_read_32(struct M68K *m68k, M68K_STATE_PARAMS, uint32_t eavals)
{
	if(m68k_ea_calc_src(m68k, M68K_STATE_ARGS, eavals, 4)) {
		// had EA
		return m68k_read_32(m68k, M68K_STATE_ARGS, m68k->last_ea);

	} else {
		// only had reg
		return m68k->last_non_ea;
	}
}

static void m68k_grp_0x0(struct M68K *m68k, M68K_STATE_PARAMS, uint16_t op)
{
	// Bit/MOVEP/Imm block contents:
	// . - 0000 00FF [+Sw] = ori.S #, ea
	// A - 003C 0000 [+1w] = ori.b #, ccr
	// A - 007C 0000 [+1w] = ori.w #, sr
	// . - 0200 00FF [+Sw] = andi.S #, ea
	// A - 023C 0000 [+1w] = andi.b #, ccr
	// A - 027C 0000 [+1w] = andi.w #, sr
	// . - 0400 00FF [+Sw] = subi.S #, ea
	// . - 0600 00FF [+Sw] = addi.S #, ea
	// A - 06C0 003F [+1w] = callm #, ea
	// B - 06C0 000F [---] = rtm {d|a}
	// ! - 00C0 063F | 0000 F000 = cmp2
	// ! - 00C0 063F | 0100 F000 = chk2
	// . - 0800 003F [+1w] = btst #, ea
	// . - 0840 003F [+1w] = bchg #, ea
	// . - 0880 003F [+1w] = bclr #, ea
	// . - 08C0 003F [+1w] = bset #, ea
	// . - 0A00 00FF [+Sw] = eori.S #, ea
	// A - 0A3C 0000 [+1w] = eori.b #, ccr
	// A - 0A7C 0000 [+1w] = eori.w #, sr
	// . - 0C00 00FF [+Sw] = cmpi.S #, ea
	// moves/cas/cas2 goes here (these might not be base 68000)
	// . - 0100 0E3F [---] = btst Dn, ea
	// . - 0140 0E3F [---] = bchg Dn, ea
	// . - 0180 0E3F [---] = bclr Dn, ea
	// . - 01C0 0E3F [---] = bset Dn, ea
	// D - 0008 0FC7 [+2w] = movep #, ea

	// TODO: determine special cases
	if((op&~0x0FC7) == 0x0008) {
		// MOVEP
		printf("%08X: %04X (MOVEP grp 0x0)\n", m68k->pc-2, op);
		m68k->halted = 1;
		m68k->H.timestamp = m68k->H.timestamp_end;

	} else if((op&~0x0EFF) == 0x0100) {
		// bit op registers
		printf("%08X: %04X (bitreg grp 0x0)\n", m68k->pc-2, op);
		m68k->halted = 1;
		m68k->H.timestamp = m68k->H.timestamp_end;

	} else switch((op>>9)&0x7) {
		case 0x1: {
			// ANDI
			switch((op>>6)&0x3) {
				case 0x0: {
					// ANDI.b
					assert((op&0x3F) != 0x3C); // CCR access

					if(!m68k_allow_ea_src(m68k, op, 0x03FD)) {
						assert(!"invalid dest EA");
					}

					uint32_t src_val = 0xFF&m68k_fetch_op_16(m68k, M68K_STATE_ARGS);
					uint32_t val;
					if(m68k_ea_calc_src(m68k, M68K_STATE_ARGS, op, 1)) {
						val = m68k_read_8(m68k, M68K_STATE_ARGS, m68k->last_ea);
						val = (val & src_val);
						m68k_write_8(m68k, M68K_STATE_ARGS, m68k->last_ea, val);

					} else {
						// should be Dn by now
						val = m68k->rd[op&7];
						val = (val & (src_val | ~0xFF));
						m68k->rd[op&7] = val;
					}

					m68k->sr &= ~(F_N|F_Z|F_V|F_C);
					m68k->sr |= ((val&0x80) != 0 ? F_N : 0);
					m68k->sr |= (val == 0 ? F_Z : 0);
				} break;

				case 0x1: {
					// ANDI.w
					assert((op&0x3F) != 0x3C); // SR access
					printf("%08X: %04X (ANDI.w grp 0x0)\n", m68k->pc-2, op);
					m68k->halted = 1;
					m68k->H.timestamp = m68k->H.timestamp_end;
				} break;

				case 0x2: {
					// ANDI.l
					assert((op&0x3F) != 0x3C); // invalid SR access
					printf("%08X: %04X (ANDI.l grp 0x0)\n", m68k->pc-2, op);
					m68k->halted = 1;
					m68k->H.timestamp = m68k->H.timestamp_end;
				} break;

				default:
					printf("%08X: %04X (ANDI grp 0x0)\n", m68k->pc-2, op);
					m68k->halted = 1;
					m68k->H.timestamp = m68k->H.timestamp_end;
					break;
			};
		} break;

		default:
			printf("%08X: %04X (grp 0x0)\n", m68k->pc-2, op);
			m68k->halted = 1;
			m68k->H.timestamp = m68k->H.timestamp_end;
			break;
	}

}

static void m68k_grp_0x1(struct M68K *m68k, M68K_STATE_PARAMS, uint16_t op)
{
	// MOVE.b
	if(!m68k_allow_ea_dst(m68k, op, 0x03FD)) {
		assert(!"invalid dest EA");
	}
	if(!m68k_allow_ea_src(m68k, op, 0x1FFD)) {
		assert(!"invalid source EA");
	}

	uint8_t val = m68k_ea_read_8(m68k, M68K_STATE_ARGS, op);
	if(m68k_ea_calc_dst(m68k, M68K_STATE_ARGS, op, 1)) {
		m68k_write_8(m68k, M68K_STATE_ARGS, m68k->last_ea, val);

	} else if(((op>>6)&0x6) == 0) {
		m68k->rd[(op>>9)&0x7] &= ~0xFF;
		m68k->rd[(op>>9)&0x7] |= val;

	} else {
		assert(!"expected D reg for move.b dest");
	}

	m68k->sr &= ~(F_N|F_Z|F_V|F_C);
	m68k->sr |= ((val&0x80) != 0 ? F_N : 0);
	m68k->sr |= (val == 0 ? F_Z : 0);
}

static void m68k_grp_0x3(struct M68K *m68k, M68K_STATE_PARAMS, uint16_t op)
{
	// MOVE.w
	if(!m68k_allow_ea_dst(m68k, op, 0x03FF)) {
		assert(!"invalid dest EA");
	}
	bool is_movea = ((op>>9)&0x7);
	if(!m68k_allow_ea_src(m68k, op, 0x1FFF)) {
		assert(!"invalid source EA");
	}

	uint16_t val = m68k_ea_read_16(m68k, M68K_STATE_ARGS, op);
	if(m68k_ea_calc_dst(m68k, M68K_STATE_ARGS, op, 2)) {
		m68k_write_16(m68k, M68K_STATE_ARGS, m68k->last_ea, val);

	} else if(((op>>6)&0x6) == 0) {
		m68k->rd[(op>>9)&0x7] &= ~0xFFFF;
		m68k->rd[(op>>9)&0x7] |= val;

	} else {
		assert(is_movea);
		m68k->ra[(op>>9)&0x7] = (uint32_t)(int32_t)(int16_t)val;
		return; // skip flag change
	}

	m68k->sr &= ~(F_N|F_Z|F_V|F_C);
	m68k->sr |= ((val&0x8000) != 0 ? F_N : 0);
	m68k->sr |= (val == 0 ? F_Z : 0);
}

static void m68k_grp_0x2(struct M68K *m68k, M68K_STATE_PARAMS, uint16_t op)
{
	// MOVE.l
	if(!m68k_allow_ea_dst(m68k, op, 0x03FF)) {
		assert(!"invalid dest EA");
	}
	bool is_movea = ((op>>9)&0x7);
	if(!m68k_allow_ea_src(m68k, op, 0x1FFF)) {
		assert(!"invalid source EA");
	}

	uint32_t val = m68k_ea_read_32(m68k, M68K_STATE_ARGS, op);
	if(m68k_ea_calc_dst(m68k, M68K_STATE_ARGS, op, 4)) {
		m68k_write_32(m68k, M68K_STATE_ARGS, m68k->last_ea, val);

	} else if(((op>>6)&0x6) == 0) {
		m68k->rd[(op>>9)&0x7] = val;

	} else {
		assert(is_movea);
		m68k->ra[(op>>9)&0x7] = val;
		return; // skip flag change
	}

	m68k->sr &= ~(F_N|F_Z|F_V|F_C);
	m68k->sr |= ((val&0x80000000) != 0 ? F_N : 0);
	m68k->sr |= (val == 0 ? F_Z : 0);
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
	// . 2 4C00 003F | 0000 7407 = mulu LONG
	// . 2 4C00 003F | 0800 7407 = muls LONG
	// . 2 4C08 003F | 0000 7407 = divu/divul LONG
	// . 2 4C08 003F | 0800 7407 = divs/divsl LONG
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
		if(!m68k_allow_ea_src(m68k, op, 0x0FE4)) {
			assert(!"invalid source EA");
		}
		if(!m68k_ea_calc_src(m68k, M68K_STATE_ARGS, op, 4)) {
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

	} else if((op&~0x47F) == 0x4880) {
		// MOVEM
		bool movem_read_memory = ((op&(1<<10)) != 0);
		bool movem_is_long = ((op&(1<<6)) != 0);

		// mask is read before EA
		uint16_t movem_mask = m68k_fetch_op_16(m68k, M68K_STATE_ARGS);

		// Order: LSB to MSB
		// Postinc / Flat: 0 = D0
		// Predec (only available when movem_read_memory is false): 0 = A7

		// Get popcount
		int pcnt = 0;
		for(int i = 0; i < 16; i++) {
			if((movem_mask&(1<<i)) != 0) {
				pcnt++;
			}
		}

		if(movem_read_memory) {
			// mem reads are 4 cycles slower
			M68K_ADD_CYCLES(m68k, 4);
			if(!m68k_allow_ea_src(m68k, op, 0x07EC)) {
				assert(!"invalid source EA");
			}

			if(movem_is_long) {
				m68k_ea_calc_src(m68k, M68K_STATE_ARGS, op, 4*pcnt);

				uint32_t addr = m68k->last_ea;

				for(int i = 0; i < 8; i++) {
					if((movem_mask&(0x0001<<i)) != 0) {
						m68k->rd[i] = m68k_read_32(m68k, M68K_STATE_ARGS, addr);
						addr += 4;
					}
				}
				for(int i = 0; i < 8; i++) {
					if((movem_mask&(0x0100<<i)) != 0) {
						m68k->ra[i] = m68k_read_32(m68k, M68K_STATE_ARGS, addr);
						addr += 4;
					}
				}

				return;

			} else {
				m68k_ea_calc_src(m68k, M68K_STATE_ARGS, op, 2*pcnt);
				uint32_t addr = m68k->last_ea;

				for(int i = 0; i < 8; i++) {
					if((movem_mask&(0x0001<<i)) != 0) {
						m68k->rd[i] &= ~0xFFFF;
						m68k->rd[i] |= m68k_read_16(m68k, M68K_STATE_ARGS, addr);
						addr += 2;
					}
				}
				for(int i = 0; i < 8; i++) {
					if((movem_mask&(0x0100<<i)) != 0) {
						m68k->ra[i] &= ~0xFFFF;
						m68k->ra[i] |= m68k_read_16(m68k, M68K_STATE_ARGS, addr);
						addr += 2;
					}
				}

				return;
			}

		} else {
			printf("WRITE\n");
			bool movem_is_predec = (((op>>3)&0x7) == 4);

			if(!m68k_allow_ea_src(m68k, op, 0x03F4)) {
				assert(!"invalid dest EA");
			}

			if(movem_is_predec) {
				if(movem_is_long) {
					//
				} else {
					//
				}

			} else {
				if(movem_is_long) {
					m68k_ea_calc_src(m68k, M68K_STATE_ARGS, op, 4*pcnt);
				} else {
					m68k_ea_calc_src(m68k, M68K_STATE_ARGS, op, 2*pcnt);
				}

			}
			// TODO: remainder of this thing
		}

		printf("%08X: %04X (grp 0x4)\n", m68k->pc-2, op);
		printf("movem ops %d %d %04X\n"
			, movem_read_memory ? 1 : 0
			, movem_is_long ? 1 : 0
			, movem_mask
			);
		m68k->halted = 1;
		m68k->H.timestamp = m68k->H.timestamp_end;

	} else switch((op>>9)&7) {
		case 0x3: switch((op>>6)&3) {
			case 0x3: {
				assert(((op>>3)&7) != 1); // TODO make this a cpu trap instead
				uint16_t val = m68k_ea_read_16(m68k, M68K_STATE_ARGS, op);
				assert((m68k->sr&0x2000) != 0); // TODO trap instead
				m68k_set_sr(m68k, M68K_STATE_ARGS, val);
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
				if(!m68k_allow_ea_src(m68k, op, 0x03FD)) {
					assert(!"invalid source EA");
				}
				uint8_t val = m68k_ea_read_8(m68k, M68K_STATE_ARGS, op);
				m68k->sr &= ~(F_N|F_Z|F_V|F_C);
				m68k->sr |= ((val&0x80) != 0 ? F_N : 0);
				m68k->sr |= (val == 0 ? F_Z : 0);
			} break;

			case 0x1: {
				// TST.w
				if(!m68k_allow_ea_src(m68k, op, 0x03FD)) {
					assert(!"invalid source EA");
				}
				uint16_t val = m68k_ea_read_16(m68k, M68K_STATE_ARGS, op);
				m68k->sr &= ~(F_N|F_Z|F_V|F_C);
				m68k->sr |= ((val&0x8000) != 0 ? F_N : 0);
				m68k->sr |= (val == 0 ? F_Z : 0);
			} break;

			case 0x2: {
				// TST.l
				if(!m68k_allow_ea_src(m68k, op, 0x03FD)) {
					assert(!"invalid source EA");
				}
				uint32_t val = m68k_ea_read_32(m68k, M68K_STATE_ARGS, op);
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

		case 0x7: switch((op>>4)&0xF) { // Special snowflakes
			case 0x06: {
				assert((m68k->sr&0x2000)!=0);

				uint32_t *usp = &(m68k->usp_store);

				if((op&0x0008) == 0) {
					// A->USP
					*usp = m68k->ra[op&0x7];

				} else {
					// USP->A
					m68k->ra[op&0x7] = *usp;

				}
			} break;

			default:
				printf("%08X: %04X (special snowflakes, grp 0x4)\n", m68k->pc-2, op);
				m68k->halted = 1;
				m68k->H.timestamp = m68k->H.timestamp_end;
				break;
		} break;

		default:
			printf("%08X: %04X (grp 0x4)\n", m68k->pc-2, op);
			m68k->halted = 1;
			m68k->H.timestamp = m68k->H.timestamp_end;
			break;
	}

}

static void m68k_grp_0x5(struct M68K *m68k, M68K_STATE_PARAMS, uint16_t op)
{
	// DBcc
	int cond = (op>>8)&0xF;
	uint32_t offs = (uint32_t)(int32_t)(int16_t)m68k_fetch_op_16(m68k, M68K_STATE_ARGS);
	bool cond_pass = m68k_cc4_true(m68k, cond);

	if(cond_pass) {
		M68K_ADD_CYCLES(m68k, 8);
		return;
	}

	// Decrement
	int reg = op&0x7;
	uint16_t v = m68k->rd[reg]-1;
	m68k->rd[reg] &= ~0xFFFF;
	m68k->rd[reg] |= v;

	// Branch if necessary
	if(v == 0xFFFF) {
		M68K_ADD_CYCLES(m68k, 10);
	} else {
		m68k->pc += offs-2;
		assert((m68k->pc&1) == 0);
		M68K_ADD_CYCLES(m68k, 6);
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
		offs = (uint32_t)(int32_t)(int16_t)m68k_fetch_op_16(m68k, M68K_STATE_ARGS);
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

static void m68k_grp_0x7(struct M68K *m68k, M68K_STATE_PARAMS, uint16_t op)
{
	// MOVEQ
	assert((op&0x0100)==0);

	uint32_t val = (uint32_t)(int32_t)(int8_t)op;
	m68k->sr &= ~(F_N|F_Z|F_V|F_C);
	m68k->sr |= ((val&0x8000) != 0 ? F_N : 0);
	m68k->sr |= (val == 0 ? F_Z : 0);
	m68k->rd[(op>>9)&0x7] = val;
}

static void m68k_grp_0xD(struct M68K *m68k, M68K_STATE_PARAMS, uint16_t op)
{
	// ADD/ADDA/ADDX
	int opmode = (op>>6)&7;

	if(((op>>3)&6) == 0 && (opmode&4) != 0) {
		// ADDX, EA makes no damn sense
		printf("%08X: %04X (ADDX, grp 0x6)\n", m68k->pc-2, op);
		m68k->halted = 1;
		m68k->H.timestamp = m68k->H.timestamp_end;
		return;
	}

	// WARNING: V FLAG FEATURED.
	// LIKE EVERYTHING INVOLVING A V FLAG, HERE BE DRAGONS.
	// Going by sign, these set the overflow flag:
	// +a +b -r
	// -a -b +r
	// A and B are the same.
	// R is different from either one.
	// Thus, going by sign bits:
	// * NOT (A XOR B) is true.
	// * (R XOR A) is true.

	// NOTE: long timing is weird here.
	// Normally 6 cycles, but adds 2 if EA is An, Dn, or #.

	int auxreg = (op>>9)&7;
	switch(opmode)
	{
		/*
		case 0x0: // ADD.B to Dn
			break;
		*/

		case 0x1: {
			// ADD.W to Dn
			uint16_t la = m68k->rd[auxreg];
			uint16_t lb = m68k_ea_read_32(m68k, M68K_STATE_ARGS, op);
			uint16_t lr = la+lb;
			m68k->sr &= ~(F_N|F_Z|F_V|F_C|F_X);
			m68k->sr |= (lr < la ? (F_C|F_X) : 0);
			m68k->sr |= (lr == 0 ? F_Z : 0);
			m68k->sr |= ((lr&0x8000) != 0 ? F_N : 0);
			m68k->sr |= ((0x8000&(lr^la)&~(la^lb)) != 0 ? (F_V) : 0);
			m68k->rd[auxreg] &= ~0xFFFF;
			m68k->rd[auxreg] |= lr;
		} break;

		/*
		case 0x2: // ADD.L to Dn
			break;

		case 0x3: // ADDA.W to An
			break;

		case 0x4: // ADD.B from Dn
			break;

		case 0x5: // ADD.W from Dn
			break;

		case 0x6: // ADD.L from Dn
			break;

		case 0x7: // ADDA.L to An
			break;
		*/

		default:
			printf("%08X: %04X (ADD/ADDA, grp 0x6)\n", m68k->pc-2, op);
			m68k->halted = 1;
			m68k->H.timestamp = m68k->H.timestamp_end;
			return;
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
		m68k_set_sr(m68k, M68K_STATE_ARGS, 0x2700);
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
			case 0x0:
				m68k_grp_0x0(m68k, M68K_STATE_ARGS, op);
				break;

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

			case 0x5:
				m68k_grp_0x5(m68k, M68K_STATE_ARGS, op);
				break;

			case 0x6:
				m68k_grp_0x6(m68k, M68K_STATE_ARGS, op);
				break;

			case 0x7:
				m68k_grp_0x7(m68k, M68K_STATE_ARGS, op);
				break;

			case 0xD:
				m68k_grp_0xD(m68k, M68K_STATE_ARGS, op);
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

