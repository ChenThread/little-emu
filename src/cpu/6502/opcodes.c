// NOTE: ONLY CALL THESE FUNCTIONS IN OPCODES, AND KEEP IN MIND THEY COUNT TOWARDS THE CYCLE COUNT!!!

#define PAGE_MATCH(a, b) (((a) & 0xFF00) == ((b) & 0xFF00))

// 6502 addr. modes

static inline uint16_t cpu_6502_addr_ra(CPU_STATE_PARAMS, bool ignore_page_boundary) { return 0; }
static inline uint8_t cpu_6502_read_ra(CPU_STATE_PARAMS, uint16_t addr) { return state->ra; }
static inline void cpu_6502_write_ra(CPU_STATE_PARAMS, uint16_t addr, uint8_t v) { state->ra = v; }

static inline uint16_t cpu_6502_addr_imm(CPU_STATE_PARAMS, bool ignore_page_boundary) { return state->pc++; }
static inline uint16_t cpu_6502_addr_zp(CPU_STATE_PARAMS, bool ignore_page_boundary) { return cpu_6502_next_cycled(CPU_STATE_ARGS); }

static inline uint16_t cpu_6502_addr_zpx(CPU_STATE_PARAMS, bool ignore_page_boundary) {
	uint8_t base = cpu_6502_next_cycled(CPU_STATE_ARGS);
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);
	return (base + state->rx) & 0xFF;
}

static inline uint16_t cpu_6502_addr_zpy(CPU_STATE_PARAMS, bool ignore_page_boundary) {
	uint8_t base = cpu_6502_next_cycled(CPU_STATE_ARGS);
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);
	return (base + state->ry) & 0xFF;
}

static inline uint16_t cpu_6502_addr_abs(CPU_STATE_PARAMS, bool ignore_page_boundary) {
	return cpu_6502_next_word_cycled(CPU_STATE_ARGS);
}

static inline uint16_t cpu_6502_addr_absx(CPU_STATE_PARAMS, bool ignore_page_boundary) {
	uint16_t base = cpu_6502_next_word_cycled(CPU_STATE_ARGS);
	if (ignore_page_boundary || !PAGE_MATCH(base, base + state->rx)) {
		CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);
	}
	return base + state->rx;
}

static inline uint16_t cpu_6502_addr_absy(CPU_STATE_PARAMS, bool ignore_page_boundary) {
	uint16_t base = cpu_6502_next_word_cycled(CPU_STATE_ARGS);
	if (ignore_page_boundary || !PAGE_MATCH(base, base + state->ry)) {
		CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);
	}
	return base + state->ry;
}

static inline uint16_t cpu_6502_addr_ind(CPU_STATE_PARAMS, bool ignore_page_boundary) {
	return cpu_6502_read_mem_word_cycled(CPU_STATE_ARGS, cpu_6502_next_word_cycled(CPU_STATE_ARGS));
}

static inline uint16_t cpu_6502_addr_ind_page(CPU_STATE_PARAMS, bool ignore_page_boundary) {
	return cpu_6502_read_mem_word_cycled_page(CPU_STATE_ARGS, cpu_6502_next_word_cycled(CPU_STATE_ARGS));
}

static inline uint16_t cpu_6502_addr_zpindx(CPU_STATE_PARAMS, bool ignore_page_boundary) {
	uint8_t addr = (cpu_6502_next_cycled(CPU_STATE_ARGS) + state->rx) & 0xFF;
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);
	return cpu_6502_read_mem_word_cycled_page(CPU_STATE_ARGS, addr);
}
static inline uint16_t cpu_6502_addr_zpindy(CPU_STATE_PARAMS, bool ignore_page_boundary) {
	uint16_t base = cpu_6502_read_mem_word_cycled_page(CPU_STATE_ARGS, cpu_6502_next_cycled(CPU_STATE_ARGS));
	if (ignore_page_boundary || !PAGE_MATCH(base, base + state->ry)) {
		CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);
	}
	return base + state->ry;
}

// 65c02 addr. modes

static inline uint16_t cpu_6502_addr_absxind(CPU_STATE_PARAMS, bool ignore_page_boundary) {
	// TODO: verify timing/behaviour
	uint16_t base = cpu_6502_next_word_cycled(CPU_STATE_ARGS);
	if (ignore_page_boundary || !PAGE_MATCH(base, base + state->rx)) {
		CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);
	}
	return cpu_6502_read_mem_word_cycled_page(CPU_STATE_ARGS, base + state->rx);
}

static inline uint16_t cpu_6502_addr_zpind(CPU_STATE_PARAMS, bool ignore_page_boundary) {
	uint8_t addr = cpu_6502_next_cycled(CPU_STATE_ARGS);
	return cpu_6502_read_mem_word_cycled_page(CPU_STATE_ARGS, addr);
}

// opcode generators

#define CPU_FUNC_TRANSFER(name, from, to) \
	static void cpu_##name(CPU_STATE_PARAMS) { \
		CPU_ADD_CYCLES(CPU_STATE_ARGS, 1); \
		CPU_UPDATE_NZ((from)); (to) = (from); \
	}

#define CPU_FUNC_SETFLAG(name, v) \
	static void cpu_##name(CPU_STATE_PARAMS) { \
		CPU_ADD_CYCLES(CPU_STATE_ARGS, 1); \
		state->flag |= (v); \
	}
#define CPU_FUNC_CLEARFLAG(name, v) \
	static void cpu_##name(CPU_STATE_PARAMS) { \
		CPU_ADD_CYCLES(CPU_STATE_ARGS, 1); \
		state->flag &= ~(v); \
	}

#define CPU_FUNC_PUSH(name, v) \
	static void cpu_##name(CPU_STATE_PARAMS) { \
		CPU_ADD_CYCLES(CPU_STATE_ARGS, 1); \
		cpu_6502_push_cycled(CPU_STATE_ARGS, (v)); \
	}

#define CPU_FUNC_POP(name, v) \
	static void cpu_##name(CPU_STATE_PARAMS) { \
		CPU_ADD_CYCLES(CPU_STATE_ARGS, 2); \
		(v) = cpu_6502_pop_cycled(CPU_STATE_ARGS); CPU_UPDATE_NZ((v)); \
	}

#define CPU_FUNC_BRANCH(name, cond) \
	static void cpu_##name(CPU_STATE_PARAMS) { \
		CPU_ADD_CYCLES(CPU_STATE_ARGS, 1); \
		if ((cond)) { \
			state->pc += ((int8_t) cpu_6502_next_cycled(CPU_STATE_ARGS)); \
		} else { \
			state->pc++; \
		} \
	}

// opcodes

static void cpu_6502_dex(CPU_STATE_PARAMS) {
	(state->rx)--;
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);
	CPU_UPDATE_NZ(state->rx);
}

static void cpu_6502_dey(CPU_STATE_PARAMS) {
	(state->ry)--;
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);
	CPU_UPDATE_NZ(state->ry);
}

static void cpu_6502_inx(CPU_STATE_PARAMS) {
	(state->rx)++;
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);
	CPU_UPDATE_NZ(state->rx);
}

static void cpu_6502_iny(CPU_STATE_PARAMS) {
	(state->ry)++;
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);
	CPU_UPDATE_NZ(state->ry);
}

static void cpu_6502_jsr(CPU_STATE_PARAMS) {
	uint8_t low = cpu_6502_next_cycled(CPU_STATE_ARGS);
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);
	cpu_6502_push_word_cycled(CPU_STATE_ARGS, state->pc);
	state->pc = low | (cpu_6502_next_cycled(CPU_STATE_ARGS) << 8);
}

static void cpu_6502_rti(CPU_STATE_PARAMS) {
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 2);
	state->flag = cpu_6502_pop_cycled(CPU_STATE_ARGS);
	state->pc = cpu_6502_pop_word_cycled(CPU_STATE_ARGS);
}

static void cpu_6502_rts(CPU_STATE_PARAMS) {
	cpu_6502_next_cycled(CPU_STATE_ARGS);
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 3);
	state->pc = cpu_6502_pop_word_cycled(CPU_STATE_ARGS) + 1;
}

static void cpu_6502_brk(CPU_STATE_PARAMS) {
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 6);
	// TODO
}

static void cpu_6502_hlt(CPU_STATE_PARAMS) {
	// TODO
}

static void cpu_6502_nop(CPU_STATE_PARAMS) {
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);
}

CPU_FUNC_CLEARFLAG(6502_clc, FLAG_C);
CPU_FUNC_CLEARFLAG(6502_cld, FLAG_D);
CPU_FUNC_CLEARFLAG(6502_cli, FLAG_I);
CPU_FUNC_CLEARFLAG(6502_clv, FLAG_V);
CPU_FUNC_SETFLAG(6502_sec, FLAG_C);
CPU_FUNC_SETFLAG(6502_sed, FLAG_D);
CPU_FUNC_SETFLAG(6502_sei, FLAG_I);

CPU_FUNC_TRANSFER(6502_tax, state->ra, state->rx);
CPU_FUNC_TRANSFER(6502_tay, state->ra, state->ry);
CPU_FUNC_TRANSFER(6502_tsx, state->sp, state->rx);
CPU_FUNC_TRANSFER(6502_txa, state->rx, state->ra);
CPU_FUNC_TRANSFER(6502_tya, state->ry, state->ra);

static void cpu_6502_txs(CPU_STATE_PARAMS) {
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);
	state->sp = state->rx;
}

CPU_FUNC_PUSH(6502_pha, state->ra);
CPU_FUNC_PUSH(6502_php, state->flag);
CPU_FUNC_POP(6502_pla, state->ra);

static void cpu_6502_plp(CPU_STATE_PARAMS) {
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 2);
	state->flag = cpu_6502_pop_cycled(CPU_STATE_ARGS) | (1 << 5);
}

CPU_FUNC_BRANCH(6502_bcc, (state->flag & FLAG_C) == 0);
CPU_FUNC_BRANCH(6502_bcs, (state->flag & FLAG_C) != 0);
CPU_FUNC_BRANCH(6502_beq, (state->flag & FLAG_Z) != 0);
CPU_FUNC_BRANCH(6502_bne, (state->flag & FLAG_Z) == 0);
CPU_FUNC_BRANCH(6502_bpl, (state->flag & FLAG_N) == 0);
CPU_FUNC_BRANCH(6502_bmi, (state->flag & FLAG_N) != 0);
CPU_FUNC_BRANCH(6502_bvc, (state->flag & FLAG_V) == 0);
CPU_FUNC_BRANCH(6502_bvs, (state->flag & FLAG_V) != 0);

CPU_FUNC_BRANCH(65c02_bra, true);
CPU_FUNC_PUSH(65c02_phx, state->rx);
CPU_FUNC_PUSH(65c02_phy, state->ry);
CPU_FUNC_POP(65c02_plx, state->rx);
CPU_FUNC_POP(65c02_ply, state->ry);

static void cpu_huc6280_clx(CPU_STATE_PARAMS) {
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);
	state->rx = 0;
}

static void cpu_huc6280_cly(CPU_STATE_PARAMS) {
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);
	state->ry = 0;
}

static void cpu_huc6280_sax(CPU_STATE_PARAMS) {
	uint8_t tmp;
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 2);
	tmp = state->ra; state->ra = state->rx; state->rx = tmp;
}

static void cpu_huc6280_say(CPU_STATE_PARAMS) {
	uint8_t tmp;
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 2);
	tmp = state->ry; state->ry = state->ra; state->ra = tmp;
}

static void cpu_huc6280_sxy(CPU_STATE_PARAMS) {
	uint8_t tmp;
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 2);
	tmp = state->ry; state->ry = state->rx; state->rx = tmp;
}

// TODO: decimal mode is prob. buggy, fix it

static inline void cpu_6502_internal_adc(CPU_STATE_PARAMS, int8_t value) {
#ifdef CPU_6502_ENABLE_BCD
	if ((state->flag) & FLAG_D) {
		uint16_t a;
		int8_t a_v;
		uint8_t a_low = (state->ra & 0x0F) + (value & 0x0F);
		if ((state->flag) & FLAG_C)
			a_low++;
		if (a_low >= 0x0A)
			a_low = ((a_low + 0x06) & 0x0F) + 0x10;

		// calculate A+C
		a = (state->ra & 0xF0) + (value & 0xF0) + a_low;
		if (a >= 0xA0)
			a += 0x60;
		state->ra = a & 0xFF;
		if (a >= 0x100) state->flag |= FLAG_C;
		else state->flag &= ~FLAG_C;

		// calculate V
		a_v = (int8_t)(state->ra & 0xF0) + (int8_t)(value & 0xF0) + a_low;
		CPU_SET_V(a_v);
	} else {
#else
	if (true) {
#endif
		uint8_t old_ra = state->ra;
		int16_t result = ((int8_t) old_ra);

		if ((state->flag) & FLAG_C) {
			result += value + 1;
			state->ra = result & 0xFF;
			if (state->ra <= old_ra) { state->flag |= FLAG_C; }
			else { state->flag &= ~(FLAG_C); }
		} else {
			result += value;
			state->ra = result & 0xFF;
			if (state->ra < old_ra) { state->flag |= FLAG_C; }
			else { state->flag &= ~(FLAG_C); }
		}

		CPU_SET_V(result);
	}
}

static inline void cpu_6502_internal_sbc(CPU_STATE_PARAMS, int8_t value) {
#ifdef CPU_6502_ENABLE_BCD
	if ((state->flag) & FLAG_D) {
		int16_t a;
		uint8_t old_ra = state->ra;
		int8_t a_low = (state->ra & 0x0F) - (value & 0x0F);
		if (!((state->flag) & FLAG_C))
			a_low--;
		if (a_low < 0)
			a_low = ((a_low - 0x06) & 0x0F) - 0x10;

		// calculate A+C
		a = (state->ra & 0xF0) - (value & 0xF0) + a_low;
		if (a < 0)
			a -= 0x60;
		state->ra = a & 0xFF;
		if ((a & 0x100)) state->flag |= FLAG_C;
		else state->flag &= ~FLAG_C;
		if (a < -128 || a > 127) state->flag |= FLAG_V; // TODO: verify
	} else {
#else
	if (true) {
#endif
		uint8_t old_ra = state->ra;
		int16_t result = ((int8_t) old_ra);

		if ((state->flag) & FLAG_C) {
			result -= value;
			state->ra = result & 0xFF;
			if (state->ra <= old_ra) { state->flag |= FLAG_C; }
			else { state->flag &= ~(FLAG_C); }
		} else {
			result -= value + 1;
			state->ra = result & 0xFF;
			if (state->ra < old_ra) { state->flag |= FLAG_C; }
			else { state->flag &= ~(FLAG_C); }
		}

		CPU_SET_V(result);
	}
}

#define CPU_ADDR_MODE ra
#define CPU_READ(addr) (state->ra)
#define CPU_WRITE(addr, v) (state->ra = (v))
#include "opcodes_addressed.h"
#undef CPU_ADDR_MODE
#undef CPU_READ
#undef CPU_WRITE
#define CPU_ADDR_MODE imm
#define CPU_READ(addr) cpu_6502_read_mem_cycled(CPU_STATE_ARGS, (addr))
#define CPU_WRITE(addr, v) cpu_6502_write_mem_cycled(CPU_STATE_ARGS, (addr), (v))
#include "opcodes_addressed.h"
#undef CPU_ADDR_MODE
#define CPU_ADDR_MODE zp
#include "opcodes_addressed.h"
#undef CPU_ADDR_MODE
#define CPU_ADDR_MODE zpx
#include "opcodes_addressed.h"
#undef CPU_ADDR_MODE
#define CPU_ADDR_MODE zpy
#include "opcodes_addressed.h"
#undef CPU_ADDR_MODE
#define CPU_ADDR_MODE zpindx
#include "opcodes_addressed.h"
#undef CPU_ADDR_MODE
#define CPU_ADDR_MODE zpindy
#include "opcodes_addressed.h"
#undef CPU_ADDR_MODE
#define CPU_ADDR_MODE abs
#include "opcodes_addressed.h"
#undef CPU_ADDR_MODE
#define CPU_ADDR_MODE absx
#include "opcodes_addressed.h"
#undef CPU_ADDR_MODE
#define CPU_ADDR_MODE absy
#include "opcodes_addressed.h"
#undef CPU_ADDR_MODE
#define CPU_ADDR_MODE ind
#include "opcodes_addressed.h"

#undef CPU_ADDR_MODE
#define CPU_ADDR_MODE ind_page
#include "opcodes_addressed.h"

#undef CPU_ADDR_MODE
#define CPU_ADDR_MODE absxind
#include "opcodes_addressed.h"
#undef CPU_ADDR_MODE
#define CPU_ADDR_MODE zpind
#include "opcodes_addressed.h"

#undef CPU_ADDR_MODE

#include "opcodes_tables.h"