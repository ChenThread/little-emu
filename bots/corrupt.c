#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include <SDL.h>
#include "littlesms.h"

#define BACKLOG0_CAP 64
#define BACKLOG1_CAP 16384
#define INPUT_CAP (BACKLOG0_CAP*BACKLOG1_CAP)
static struct SMS backlog0[BACKLOG0_CAP];
static struct SMS backlog1[BACKLOG1_CAP];
static uint8_t input_log[INPUT_CAP][2];
static int backlog_idx = 0;

#define BOT_ROM_SIZE (512*1024)
static uint8_t rom_backup[BOT_ROM_SIZE];
static uint8_t rom_main[BOT_ROM_SIZE];
static uint8_t rom_corruption_mask[BOT_ROM_SIZE];
static uint8_t rom_protection_mask[BOT_ROM_SIZE];
static int rom_size = 512*1024;
static uint32_t corruption_log[INPUT_CAP];

FILE *fp_random = NULL;

static uint32_t get_random(void)
{
	uint32_t ret;
	fread(&ret, 4, 1, fp_random);
	return ret;
}

static void drop_frame(void)
{
	// Rewind one frame
	int i0 = backlog_idx/BACKLOG0_CAP;
	//int b0 = i0*BACKLOG0_CAP;
	backlog_idx--;
	assert(backlog_idx >= 0);
	assert(backlog_idx < INPUT_CAP);
	int i1 = backlog_idx/BACKLOG0_CAP;
	int b1 = i1*BACKLOG0_CAP;

	// Undo corruption
	int c = corruption_log[backlog_idx];
	if(c >= 0) {
		if((rom_corruption_mask[c>>3]&(1<<(c&7))) != 0) {
			sms_rom[c>>3] ^= (1<<(c&7));
			rom_main[c>>3] ^= (1<<(c&7));
			rom_corruption_mask[c>>3] ^= (1<<(c&7));
		}
	}

	// If we are copying from a different block, expand the block
	if(i1 != i0) {
		// Copy from suitable backlog1
		sms_copy(&backlog0[0], &backlog1[i1]);

		// Complete backlog0
		for(int i = 1; i < BACKLOG0_CAP /* && i+b1 < backlog_idx */; i++) {
			sms_copy(&backlog0[i], &backlog0[i-1]);
			backlog0[i].joy[0] = input_log[i-1+b1][0];
			backlog0[i].joy[1] = input_log[i-1+b1][1];
			backlog0[i].no_draw = true;
			/*
			printf("%2d %9d %02X %02X\n", i, i+b1
				, backlog0[i].joy[0]
				, backlog0[i].joy[1]
				);
			*/
			sms_run_frame(&backlog0[i]);
			backlog0[i].no_draw = false;
			backlog0[i].joy[0] = input_log[i+b1][0];
			backlog0[i].joy[1] = input_log[i+b1][1];
		}
	}

	// Copy state
	sms_copy(&sms_current, &backlog0[backlog_idx % BACKLOG0_CAP]);
}

static void save_frame(void)
{
	sms_copy(&backlog0[backlog_idx % BACKLOG0_CAP], &sms_current);
	if((backlog_idx % BACKLOG0_CAP) == 0) {
		sms_copy(&backlog1[backlog_idx / BACKLOG0_CAP], &sms_current);
	}

	// Apply corruption
	int c = get_random() & (rom_size*8-1);
	if(get_random()%100 < 50 && (rom_protection_mask[c>>3] & (1<<(c&7))) == 0) {
		sms_rom[c>>3] ^= (1<<(c&7));
		rom_main[c>>3] ^= (1<<(c&7));
		rom_corruption_mask[c>>3] |= (1<<(c&7));
		rom_protection_mask[c>>3] |= (1<<(c&7));
	} else {
		c = -1;
	}
	corruption_log[backlog_idx] = c;

	backlog_idx++;
}

void bot_update()
{
	bool shift_left = (SDL_GetModState() & KMOD_LSHIFT);

	uint8_t frame_j0 = sms_current.joy[0];
	uint8_t frame_j1 = sms_current.joy[1];

	if(shift_left && backlog_idx > 0) {
		drop_frame();
		if(backlog_idx > 0) {
			drop_frame();
		}
	}

	assert(backlog_idx >= 0);
	assert(backlog_idx < INPUT_CAP);
	input_log[backlog_idx][0] = frame_j0;
	input_log[backlog_idx][1] = frame_j1;
	sms_current.joy[0] = frame_j0;
	sms_current.joy[1] = frame_j1;
	//printf("STATE %9d %016llX\n", backlog_idx, (unsigned long long)(sms_current.timestamp));
	save_frame();

	struct SMS safe_state;
	struct SMS broken_state;
	sms_copy(&safe_state, &sms_current);
	sms_copy(&broken_state, &sms_current);
	safe_state.no_draw = true;
	broken_state.no_draw = true;
	memcpy(sms_rom, rom_backup, sizeof(rom_backup));
	sms_run_frame(&safe_state);
	memcpy(sms_rom, rom_main, sizeof(rom_main));
	sms_run_frame(&broken_state);
	int rdiff = 0;
	int vdiff = 0;
	for(int i = 0; i < 8192; i++) {
		if(safe_state.ram[i] != broken_state.ram[i]) {
			rdiff++;
		}
	}
	for(int i = 0; i < 16384; i++) {
		if(safe_state.vram[i] != broken_state.vram[i]) {
			vdiff++;
		}
	}

	bool pc_mismatch = (safe_state.z80.pc != broken_state.z80.pc);
	bool iff1_slip = (safe_state.z80.iff1 != 0 && broken_state.z80.iff1 == 0);
	if(rdiff != 0 || vdiff != 0 || pc_mismatch || iff1_slip) {
		printf("%9d - Corruption: %3.3f%% RAM, %3.3f%% VRAM%s%s\n"
			, backlog_idx-1
			, (rdiff/8192.0)*100.0
			, (vdiff/16384.0)*100.0
			, (pc_mismatch ? ", PC mismatch" : "")
			, (iff1_slip ? ", IFF1 slip" : "")
			);
	}

	// Autocorrect on PC mismatch
	if(pc_mismatch || iff1_slip) {
		uint32_t initial_step = rom_size/2;
		uint32_t step = initial_step;
		uint32_t offs = 0;
		while(step >= 1) {
			// Fix first half of slice
			sms_copy(&broken_state, &sms_current);
			memcpy(sms_rom, rom_main, sizeof(rom_main));
			memcpy(sms_rom+offs, rom_backup+offs, step);
			sms_run_frame(&broken_state);
			bool s0_mismatch = (safe_state.z80.pc != broken_state.z80.pc);
			bool s0_slip = (safe_state.z80.iff1 != 0 && broken_state.z80.iff1 == 0);

			// Fix second half of slice
			sms_copy(&broken_state, &sms_current);
			memcpy(sms_rom, rom_main, sizeof(rom_main));
			memcpy(sms_rom+offs+step, rom_backup+offs+step, step);
			sms_run_frame(&broken_state);
			bool s1_mismatch = (safe_state.z80.pc != broken_state.z80.pc);
			bool s1_slip = (safe_state.z80.iff1 != 0 && broken_state.z80.iff1 == 0);

			s0_mismatch = s0_mismatch || s0_slip;
			s1_mismatch = s1_mismatch || s1_slip;

			if(s0_mismatch && s1_mismatch) {
				// IMPORTANT CHECK.
				assert(step != initial_step);

				// Do not subdivide this.
				break;
			}

			if(s0_mismatch) {
				offs += step;
			}

			step >>= 1;
		}

		// Apply patch
		step = (step==0?1:step*2);
		memcpy(rom_main+offs, rom_backup+offs, step);
		memset(rom_corruption_mask+offs, 0, step);
		memcpy(sms_rom, rom_main, sizeof(rom_main));
		printf("ROM patched: %08X size %08X\n", offs, step);

	}

	//twait = time_now()-20000;
}

void bot_init()
{
	fp_random = fopen("/dev/urandom", "rb");
	memcpy(rom_backup, sms_rom, sizeof(rom_backup));
	memcpy(rom_main, sms_rom, sizeof(rom_main));
}

