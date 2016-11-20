#ifndef CPU_6502I_XAA_MAGIC
#define CPU_6502I_XAA_MAGIC 0xFF
#endif

#ifndef CPU_6502_RESET_ADDRESS
#define CPU_6502_RESET_ADDRESS 0xFFFC
#endif

#ifndef CPU_6502_NMI_ADDRESS
#define CPU_6502_NMI_ADDRESS 0xFFFA
#endif

#ifndef CPU_6502_IRQ_ADDRESS
#define CPU_6502_IRQ_ADDRESS 0xFFFE
#endif

#ifndef CPU_6502_CYCLE_MULTIPLIER
#define CPU_6502_CYCLE_MULTIPLIER 1
#endif

#ifndef CPU_MEMORY_PARAMS
	#define CPU_MEMORY_PARAMS struct EmuGlobal *H, struct EmuState *Hstate
	#define CPU_MEMORY_ARGS H, Hstate
#endif

#define CPU_STATE_PARAMS CPU_MEMORY_PARAMS, struct CPU_6502 *state
#define CPU_STATE_ARGS CPU_MEMORY_ARGS, state

typedef void (cpu_6502_opcode)(CPU_STATE_PARAMS);

#define CPU_CLEAR_FLAGS(v) state->flag &= ~(v)
#define CPU_SET_Z(v) if ((v) == 0) { state->flag |= FLAG_Z; }
#define CPU_SET_N(v) if ((v) >= 0x80) { state->flag |= FLAG_N; }
#define CPU_SET_V(v) if (((v) < -0x80) || ((v) >= 0x80)) { state->flag |= FLAG_V; }
#define CPU_UPDATE_NZ(v) { CPU_CLEAR_FLAGS(FLAG_N | FLAG_Z); CPU_SET_N((v)); CPU_SET_Z((v)); }
#define CPU_ADD_CYCLES(CPU_STATE_PARAMS, c) { (state)->H.timestamp += ((c)*CPU_6502_CYCLE_MULTIPLIER); CPU_ADD_CYCLES_EXTRA(H, Hstate, state, c); }

// To use this, implement:
// uint8_t cpu_6502_read_mem(struct EmuGlobal *H, struct EmuState *Hstate, uint16_t addr);
// void cpu_6502_write_mem(struct EmuGlobal *H, struct EmuState *Hstate, uint16_t addr, uint8_t value);

static inline uint8_t cpu_6502_read_mem_cycled(CPU_STATE_PARAMS, uint16_t addr) {
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);
	return cpu_6502_read_mem(CPU_MEMORY_ARGS, addr);
}

static inline void cpu_6502_write_mem_cycled(CPU_STATE_PARAMS, uint16_t addr, uint8_t value) {
	CPU_ADD_CYCLES(CPU_STATE_ARGS, 1);
	cpu_6502_write_mem(CPU_MEMORY_ARGS, addr, value);
}

static inline void cpu_6502_push_cycled(CPU_STATE_PARAMS, uint8_t v) {
	cpu_6502_write_mem_cycled(CPU_STATE_ARGS, 0x100 + ((state->sp)--), v);
}

static inline uint8_t cpu_6502_pop_cycled(CPU_STATE_PARAMS) {
	return cpu_6502_read_mem_cycled(CPU_STATE_ARGS, 0x100 + ((++(state->sp)) & 0xFF));
}

static inline void cpu_6502_push_word_cycled(CPU_STATE_PARAMS, uint16_t v) {
	cpu_6502_push_cycled(CPU_STATE_ARGS, v >> 8);
	cpu_6502_push_cycled(CPU_STATE_ARGS, v & 0xFF);
}

static inline uint16_t cpu_6502_pop_word_cycled(CPU_STATE_PARAMS) {
	uint8_t low = cpu_6502_pop_cycled(CPU_STATE_ARGS);
	return low | (cpu_6502_pop_cycled(CPU_STATE_ARGS) << 8);
}

static inline uint8_t cpu_6502_next_cycled(CPU_STATE_PARAMS) {
	return cpu_6502_read_mem_cycled(CPU_STATE_ARGS, state->pc++);
}

static inline uint16_t cpu_6502_read_mem_word(CPU_STATE_PARAMS, uint16_t addr) {
	uint8_t low = cpu_6502_read_mem(CPU_MEMORY_ARGS, addr);
	return low | (cpu_6502_read_mem(CPU_MEMORY_ARGS, addr+1) << 8);
}

static inline uint16_t cpu_6502_read_mem_word_page(CPU_STATE_PARAMS, uint16_t addr) {
	uint8_t low = cpu_6502_read_mem(CPU_MEMORY_ARGS, addr);
	return low | (cpu_6502_read_mem(CPU_MEMORY_ARGS, ((addr+1) & 0xFF) | (addr & 0xFF00)) << 8);
}

static inline uint16_t cpu_6502_read_mem_word_cycled(CPU_STATE_PARAMS, uint16_t addr) {
	uint8_t low = cpu_6502_read_mem_cycled(CPU_STATE_ARGS, addr);
	return low | (cpu_6502_read_mem_cycled(CPU_STATE_ARGS, addr+1) << 8);
}

static inline uint16_t cpu_6502_read_mem_word_cycled_page(CPU_STATE_PARAMS, uint16_t addr) {
	uint8_t low = cpu_6502_read_mem_cycled(CPU_STATE_ARGS, addr);
	return low | (cpu_6502_read_mem_cycled(CPU_STATE_ARGS, ((addr+1) & 0xFF) | (addr & 0xFF00)) << 8);
}

static inline uint16_t cpu_6502_next_word_cycled(CPU_STATE_PARAMS) {
	uint8_t low = cpu_6502_next_cycled(CPU_STATE_ARGS);
	return low | (cpu_6502_next_cycled(CPU_STATE_ARGS) << 8);
}

#include "opcodes.c"

void cpu_6502_reset(CPU_STATE_PARAMS) {
	state->ra = state->rx = state->ry = 0;
	state->flag = FLAG_I | (1 << 5);
	state->sp = 0xFF;
	state->pc = cpu_6502_read_mem_word(CPU_STATE_ARGS, CPU_6502_RESET_ADDRESS);
}

void cpu_6502_init(CPU_STATE_PARAMS) {
	*state = (struct CPU_6502) { .H = {.timestamp = 0,}, };
	cpu_6502_reset(CPU_STATE_ARGS);
}

void cpu_6502_run(CPU_STATE_PARAMS, uint64_t timestamp) {
	if(!TIME_IN_ORDER(state->H.timestamp, timestamp)) {
		return;
	}

	state->H.timestamp_end = timestamp;
	while(TIME_IN_ORDER(state->H.timestamp, state->H.timestamp_end)) {
		if (state->nmi_request) {
			cpu_6502_push_word_cycled(CPU_STATE_ARGS, state->pc);
			cpu_6502_push_cycled(CPU_STATE_ARGS, state->flag);
			state->pc = cpu_6502_read_mem_word(CPU_STATE_ARGS, CPU_6502_NMI_ADDRESS);
			state->nmi_request = state->irq_request = false;
		} else if (state->irq_request && !(state->flag & FLAG_I)) {
			cpu_6502_push_word_cycled(CPU_STATE_ARGS, state->pc);
			cpu_6502_push_cycled(CPU_STATE_ARGS, state->flag);
			state->pc = cpu_6502_read_mem_word(CPU_STATE_ARGS, CPU_6502_IRQ_ADDRESS);
			state->irq_request = false;
		}

		uint8_t op = cpu_6502_next_cycled(CPU_STATE_ARGS);
#ifdef CPU_6502_DEBUG
		printf("%04X [%02X]: A%02X X%02X Y%02X SP%02X F%02X\n", state->pc, op, state->ra, state->rx, state->ry, state->sp, state->flag);
#endif
		cpu_6502_opcode* ophdl = cpu_6502_opcodes[op];
		ophdl(CPU_STATE_ARGS);
	}
}

void cpu_6502_irq(struct CPU_6502 *state) {
	state->irq_request = true;
}

void cpu_6502_nmi(struct CPU_6502 *state) {
	state->nmi_request = true;
}