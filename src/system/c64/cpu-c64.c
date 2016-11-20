#include "system/c64/all.h"

#define CPU_MEMORY_PARAMS struct C64Global *H, struct C64 *Hstate
#define CPU_MEMORY_ARGS H, Hstate
#define CPU_6502_ENABLE_BCD

#define ROM_KERNAL_ENABLED (Hstate->cpu_io0 & 0x02)
#define ROM_BASIC_ENABLED (Hstate->cpu_io0 & 0x01)
#define ROM_CHAR_ENABLED (!(Hstate->cpu_io0 & 0x04))

uint8_t cpu_6502_read_mem(struct C64Global *H, struct C64 *Hstate, uint16_t addr);

#define CIA_TIME_DSEC (int)((63 * 312 * 50.125) / 10)

uint8_t cia1_read_port_a(struct CIA *cia, struct C64Global *H, struct C64 *state, uint8_t rw_mask) {
	return (cia->port_a_rw & rw_mask) | (cia->port_a_r & (~rw_mask));
}

uint8_t cia1_read_port_b(struct CIA *cia, struct C64Global *H, struct C64 *state, uint8_t rw_mask) {
	return (cia->port_b_rw & rw_mask) | (cia->port_b_r & (~rw_mask));
}

uint8_t cia2_read_port_a(struct CIA *cia, struct C64Global *H, struct C64 *state, uint8_t rw_mask) {
	return (cia->port_a_rw & rw_mask) | (cia->port_a_r & (~rw_mask));
}

uint8_t cia2_read_port_b(struct CIA *cia, struct C64Global *H, struct C64 *state, uint8_t rw_mask) {
	return (cia->port_b_rw & rw_mask) | (cia->port_b_r & (~rw_mask));
}

void cia1_write_port_a(struct CIA *cia, struct C64Global *H, struct C64 *state, uint8_t rw_mask, uint8_t value) {
}

void cia1_write_port_b(struct CIA *cia, struct C64Global *H, struct C64 *state, uint8_t rw_mask, uint8_t value) {
}

void cia2_write_port_a(struct CIA *cia, struct C64Global *H, struct C64 *state, uint8_t rw_mask, uint8_t value) {
}

void cia2_write_port_b(struct CIA *cia, struct C64Global *H, struct C64 *state, uint8_t rw_mask, uint8_t value) {
}

void cia1_interrupt(struct CIA *cia, struct C64Global *H, struct C64 *state) {
	cpu_6502_irq(&(state->cpu));
}

void cia2_interrupt(struct CIA *cia, struct C64Global *H, struct C64 *state) {
	cpu_6502_nmi(&(state->cpu));
}

#define CIA_NAME(n) cia1_##n
#include "system/c64/cia/cia.c"
#undef CIA_NAME
#define CIA_NAME(n) cia2_##n
#include "system/c64/cia/cia.c"
#undef CIA_NAME

#include "video/vicii/core.c"
#define CPU_ADD_CYCLES_EXTRA(H, Hstate, state, c) { \
	vic_run(&(Hstate->vic), H, Hstate, Hstate->H.timestamp); \
	cia1_run(&(Hstate->cia1), H, Hstate, Hstate->H.timestamp); \
	cia2_run(&(Hstate->cia2), H, Hstate, Hstate->H.timestamp); \
}

uint8_t cpu_6502_read_mem(struct C64Global *H, struct C64 *Hstate, uint16_t addr) {
	if (addr == 0x0000)
		return Hstate->cpu_io0;
	if (addr == 0x0001)
		return Hstate->cpu_io1;
	if (H->rom_cartridge_present && addr >= 0x8000 && addr <= 0xBFFF)
		return H->rom_cartridge[addr & 0x3FFF];
	if (ROM_BASIC_ENABLED && addr >= 0xA000 && addr <= 0xBFFF)
		return H->rom_basic[addr - 0xA000];
	if (ROM_KERNAL_ENABLED && addr >= 0xE000)
		return H->rom_kernal[addr - 0xE000];
	if (addr >= 0xD000 && addr <= 0xDFFF) {
		if (ROM_CHAR_ENABLED)
			return H->rom_char[addr - 0xD000];
		else {
			if (addr <= 0xD3FF || (addr >= 0xD800 && addr <= 0xDBFF))
				return vic_read_mem(H, Hstate, addr);
			else if (addr >= 0xDC00 && addr <= 0xDCFF)
				return cia1_read_mem(&(Hstate->cia1), H, Hstate, addr);
			else if (addr >= 0xDD00 && addr <= 0xDDFF)
				return cia2_read_mem(&(Hstate->cia2), H, Hstate, addr);
			else
				return 0xFF;
		}
	}

	return Hstate->ram[addr];
}

void cpu_6502_write_mem(struct C64Global *H, struct C64 *Hstate, uint16_t addr, uint8_t value) {
	if (addr == 0x0000)
		Hstate->cpu_io0 = 0xC0 | (value & 0x3F);
	else if (addr == 0x0001)
		Hstate->cpu_io1 = 0xC0 | (Hstate->cpu_io1 & 0x30) | (value & 0x0F);
	else if ((addr >= 0xD000 && addr <= 0xD3FF) || (addr >= 0xD800 && addr <= 0xDBFF))
		vic_write_mem(H, Hstate, addr, value);
	else if (addr >= 0xDC00 && addr <= 0xDCFF)
		cia1_write_mem(&(Hstate->cia1), H, Hstate, addr, value);
	else if (addr >= 0xDD00 && addr <= 0xDDFF)
		cia2_write_mem(&(Hstate->cia2), H, Hstate, addr, value);
	else
		Hstate->ram[addr] = value;
}

#include "cpu/6502/core.c"
