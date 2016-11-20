#define CPU_FUNC_I(n, am) static void cpu_ ## n ## _ ## am(CPU_STATE_PARAMS)
#define CPU_CALL_FUNC_I(n, am) cpu_ ## n ## _ ## am (CPU_STATE_ARGS)
#define CPU_ADDR_I(am, ipb) cpu_6502_addr_##am(CPU_STATE_ARGS, ipb)

#define CPU_FUNC_A(n, am) CPU_FUNC_I(n, am)
#define CPU_CALL_FUNC_A(n, am) CPU_CALL_FUNC_I(n, am)
#define CPU_ADDR_A(am, ipb) CPU_ADDR_I(am, ipb)

#define CPU_FUNC(n) CPU_FUNC_A(n, CPU_ADDR_MODE)
#define CPU_CALL_FUNC(n) CPU_CALL_FUNC_A(n, CPU_ADDR_MODE)
#define CPU_ADDR(ipb) CPU_ADDR_A(CPU_ADDR_MODE, ipb)

// 6502 - normal opcodes

CPU_FUNC(6502_nop) {
	// TODO: verify speeds (this is in for illegal variants)
	uint16_t addr = CPU_ADDR(false);
	CPU_READ(addr);
}

CPU_FUNC(6502_adc) {
	uint16_t addr = CPU_ADDR(false);
	uint8_t value = CPU_READ(addr);
	CPU_CLEAR_FLAGS(FLAG_N | FLAG_Z | FLAG_V);

	cpu_6502_internal_adc(CPU_STATE_ARGS, value);

	CPU_SET_N(state->ra);
	CPU_SET_Z(state->ra);
}

CPU_FUNC(6502_sbc) {
	uint16_t addr = CPU_ADDR(false);
	uint8_t value = CPU_READ(addr);
	CPU_CLEAR_FLAGS(FLAG_N | FLAG_Z | FLAG_V);

	cpu_6502_internal_sbc(CPU_STATE_ARGS, value);

	CPU_SET_N(state->ra);
	CPU_SET_Z(state->ra);
}

CPU_FUNC(6502_and) {
	uint16_t addr = CPU_ADDR(false);
	uint8_t value = CPU_READ(addr);
	state->ra &= value;
	CPU_UPDATE_NZ(state->ra);
}

CPU_FUNC(6502_asl) {
	uint16_t addr = CPU_ADDR(true);
	uint8_t value;

	value = CPU_READ(addr);
	CPU_CLEAR_FLAGS(FLAG_N | FLAG_Z | FLAG_C);

	if (value & 0x80) { state->flag |= FLAG_C; }
	value <<= 1;
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);

	CPU_SET_N(value);
	CPU_SET_Z(value);
	CPU_WRITE(addr, value);
}

CPU_FUNC(6502_bit) {
	uint16_t addr = CPU_ADDR(false);
	uint8_t value = CPU_READ(addr);
	CPU_CLEAR_FLAGS(FLAG_N | FLAG_Z | FLAG_V);
	CPU_SET_Z(state->ra & value);
	state->flag |= (value & 0xC0);
}

CPU_FUNC(6502_cmp) {
	uint16_t addr = CPU_ADDR(false);
	uint8_t value = CPU_READ(addr);
	CPU_CLEAR_FLAGS(FLAG_N | FLAG_Z | FLAG_C);

	if (state->ra >= value) {
		state->flag |= FLAG_C;
		if (state->ra == value) { state->flag |= FLAG_Z; }
	}

	CPU_SET_N((uint8_t) (state->ra - value));
}

CPU_FUNC(6502_cpx) {
	uint16_t addr = CPU_ADDR(false);
	uint8_t value = CPU_READ(addr);
	CPU_CLEAR_FLAGS(FLAG_N | FLAG_Z | FLAG_C);

	if (state->rx >= value) {
		state->flag |= FLAG_C;
		if (state->rx == value) { state->flag |= FLAG_Z; }
	}

	CPU_SET_N((uint8_t) (state->rx - value));
}

CPU_FUNC(6502_cpy) {
	uint16_t addr = CPU_ADDR(false);
	uint8_t value = CPU_READ(addr);
	CPU_CLEAR_FLAGS(FLAG_N | FLAG_Z | FLAG_C);

	if (state->ry >= value) {
		state->flag |= FLAG_C;
		if (state->ry == value) { state->flag |= FLAG_Z; }
	}

	CPU_SET_N((uint8_t) (state->ry - value));
}

CPU_FUNC(6502_dec) {
	uint16_t addr = CPU_ADDR(true);
	uint8_t value = CPU_READ(addr);

	value--;
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);

	CPU_UPDATE_NZ(value);
	CPU_WRITE(addr, value);
}

CPU_FUNC(6502_eor) {
	uint16_t addr = CPU_ADDR(false);
	uint8_t value = CPU_READ(addr);
	state->ra ^= value;
	CPU_UPDATE_NZ(state->ra);
}

CPU_FUNC(6502_inc) {
	uint16_t addr = CPU_ADDR(true);
	uint8_t value = CPU_READ(addr);

	value++;
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);

	CPU_UPDATE_NZ(value);
	CPU_WRITE(addr, value);
}

CPU_FUNC(6502_lda) {
	uint16_t addr = CPU_ADDR(false);
	state->ra = CPU_READ(addr);
	CPU_UPDATE_NZ(state->ra);
}

CPU_FUNC(6502_ldx) {
	uint16_t addr = CPU_ADDR(false);
	state->rx = CPU_READ(addr);
	CPU_UPDATE_NZ(state->rx);
}

CPU_FUNC(6502_ldy) {
	uint16_t addr = CPU_ADDR(false);
	state->ry = CPU_READ(addr);
	CPU_UPDATE_NZ(state->ry);
}

CPU_FUNC(6502_lsr) {
	uint16_t addr = CPU_ADDR(true);
	uint8_t value;

	value = CPU_READ(addr);
	CPU_CLEAR_FLAGS(FLAG_N | FLAG_Z | FLAG_C);

	if (value & 1) { state->flag |= FLAG_C; }
	value >>= 1;
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);

	CPU_SET_N(value);
	CPU_SET_Z(value);
	CPU_WRITE(addr, value);
}

CPU_FUNC(6502_ora) {
	uint16_t addr = CPU_ADDR(false);
	uint8_t value = CPU_READ(addr);
	state->ra |= value;
	CPU_UPDATE_NZ(state->ra);
}

CPU_FUNC(6502_rol) {
	uint16_t addr = CPU_ADDR(true);
	uint16_t value;
	uint8_t result;

	value = ((uint16_t) CPU_READ(addr)) << 1;
	value |= (state->flag) & 0x1;

	CPU_CLEAR_FLAGS(FLAG_N | FLAG_Z | FLAG_C);
	if (value & 0x100) { state->flag |= FLAG_C; }
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);

	result = (uint8_t) value;
	CPU_SET_N(result);
	CPU_SET_Z(result);
	CPU_WRITE(addr, result);
}

CPU_FUNC(6502_ror) {
	uint16_t addr = CPU_ADDR(true);
	uint16_t value;
	uint8_t result;

	value = CPU_READ(addr);
	value |= ((uint16_t) (state->flag & 0x1) << 8);
	CPU_CLEAR_FLAGS(FLAG_N | FLAG_Z | FLAG_C);

	if (value & 1) { state->flag |= FLAG_C; }
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);

	result = (uint8_t) (value >> 1);
	CPU_SET_N(result);
	CPU_SET_Z(result);
	CPU_WRITE(addr, result);
}

CPU_FUNC(6502_sta) {
	uint16_t addr = CPU_ADDR(false);
	CPU_WRITE(addr, state->ra);
}

CPU_FUNC(6502_stx) {
	uint16_t addr = CPU_ADDR(false);
	CPU_WRITE(addr, state->rx);
}

CPU_FUNC(6502_sty) {
	uint16_t addr = CPU_ADDR(false);
	CPU_WRITE(addr, state->ry);
}

CPU_FUNC(6502_jmp) {
	state->pc = CPU_ADDR(false);
}

// 6502i - illegals

CPU_FUNC(6502i_lax) { // LDA & LDX
	uint16_t addr = CPU_ADDR(false);
	state->ra = state->rx = CPU_READ(addr);
	CPU_UPDATE_NZ(state->ra);
}

CPU_FUNC(6502i_las) { // A,X,S=val&S
	uint16_t addr = CPU_ADDR(false);
	state->ra = state->rx = state->sp = state->sp & CPU_READ(addr);
	CPU_UPDATE_NZ(state->ra);
}

CPU_FUNC(6502i_tas) {
	uint16_t addr = CPU_ADDR(false);
	CPU_READ(addr);
	// TODO
}

CPU_FUNC(6502i_dcp) {
	uint16_t addr = CPU_ADDR(true);
	uint8_t value = CPU_READ(addr);

	value--;
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);

	CPU_WRITE(addr, value);
	CPU_CLEAR_FLAGS(FLAG_N | FLAG_Z | FLAG_C);

	if (state->ra >= value) {
		state->flag |= FLAG_C;
		if (state->ra == value) { state->flag |= FLAG_Z; }
	}

	CPU_SET_N((uint8_t) (state->ra - value));
}

CPU_FUNC(6502i_isc) {
	uint16_t addr = CPU_ADDR(true);
	uint8_t value = CPU_READ(addr);

	value++;
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);

	CPU_CLEAR_FLAGS(FLAG_N | FLAG_Z | FLAG_V);
	CPU_WRITE(addr, value);

	cpu_6502_internal_sbc(CPU_STATE_ARGS, value);
	CPU_SET_N(state->ra);
	CPU_SET_Z(state->ra);
}

CPU_FUNC(6502i_ahx) {
	uint16_t addr = CPU_ADDR(false);
	CPU_READ(addr);
	// TODO
}

CPU_FUNC(6502i_shx) {
	uint16_t addr = CPU_ADDR(false);
	CPU_READ(addr);
	// TODO
}

CPU_FUNC(6502i_shy) {
	uint16_t addr = CPU_ADDR(false);
	CPU_READ(addr);
	// TODO
}

CPU_FUNC(6502i_axs) {
	uint16_t addr = CPU_ADDR(false);
	CPU_READ(addr);
	// TODO
}

CPU_FUNC(6502i_sax) { // ST(A & X)
	uint16_t addr = CPU_ADDR(false);
	CPU_WRITE(addr, state->ra & state->rx);
}

CPU_FUNC(6502i_slo) { // ASL + ORA
	uint16_t addr = CPU_ADDR(true);
	uint8_t value;

	value = CPU_READ(addr);
	CPU_CLEAR_FLAGS(FLAG_N | FLAG_Z | FLAG_C);

	if (value & 0x80) { state->flag |= FLAG_C; }
	value <<= 1;
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);
	CPU_WRITE(addr, value);

	state->ra |= value;
	CPU_SET_N(state->ra);
	CPU_SET_Z(state->ra);
}

CPU_FUNC(6502i_sre) { // LSR + EOR
	uint16_t addr = CPU_ADDR(true);
	uint8_t value;

	value = CPU_READ(addr);
	CPU_CLEAR_FLAGS(FLAG_N | FLAG_Z | FLAG_C);

	if (value & 1) { state->flag |= FLAG_C; }
	value >>= 1;
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);
	CPU_WRITE(addr, value);

	state->ra ^= value;
	CPU_SET_N(state->ra);
	CPU_SET_Z(state->ra);
}

CPU_FUNC(6502i_anc) { // AND + carry
	uint16_t addr = CPU_ADDR(false);
	uint8_t value = CPU_READ(addr);
	state->ra &= value;

	CPU_CLEAR_FLAGS(FLAG_N | FLAG_Z | FLAG_C);
	CPU_SET_Z(state->ra);
	if (state->ra >= 0x80) { state->flag |= FLAG_N | FLAG_C; }
}

CPU_FUNC(6502i_rla) { // ROL + AND
	uint16_t addr = CPU_ADDR(true);
	uint16_t value;
	uint8_t result;

	value = ((uint16_t) CPU_READ(addr)) << 1;
	value |= (state->flag) & 0x1;
	
	CPU_CLEAR_FLAGS(FLAG_N | FLAG_Z | FLAG_C);
	if (value & 0x100) { state->flag |= FLAG_C; }
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);
	CPU_WRITE(addr, value);

	state->ra &= (uint8_t) value;
	CPU_SET_N(state->ra);
	CPU_SET_Z(state->ra);
}

CPU_FUNC(6502i_alr) { // AND + LSR
	uint16_t addr = CPU_ADDR(true);
	uint8_t value;

	state->ra &= CPU_READ(addr);
	CPU_CLEAR_FLAGS(FLAG_N | FLAG_Z | FLAG_C);

	if (state->ra & 1) { state->flag |= FLAG_C; }
	state->ra >>= 1;

	CPU_SET_N(state->ra);
	CPU_SET_Z(state->ra);
}

CPU_FUNC(6502i_rra) { // ROR + ADC
	uint16_t addr = CPU_ADDR(true);
	uint16_t value;
	uint8_t result;

	value = CPU_READ(addr);
	value |= ((uint16_t) (state->flag & 0x1) << 8);
	CPU_CLEAR_FLAGS(FLAG_N | FLAG_Z | FLAG_C | FLAG_V);

	if (value & 1) { state->flag |= FLAG_C; }
	result = value >> 1;
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);

	CPU_WRITE(addr, result);
	cpu_6502_internal_adc(CPU_STATE_ARGS, result);
	CPU_SET_N(state->ra);
	CPU_SET_Z(state->ra);
}

CPU_FUNC(6502i_arr) { // AND + ROR + weird C/V flags
	// TODO
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);
}

CPU_FUNC(6502i_xaa) { // what
	uint16_t addr = CPU_ADDR(false);
	state->ra = (state->ra | CPU_6502I_XAA_MAGIC) & state->rx & CPU_READ(addr);
	CPU_UPDATE_NZ(state->ra);
}

// 65c02

CPU_FUNC(65c02_stz) {
	uint16_t addr = CPU_ADDR(false);
	CPU_WRITE(addr, 0);
}