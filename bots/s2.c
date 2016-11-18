#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include <SDL.h>
#include "system/sms/all.h"

#define BACKLOG_CAP 16384
struct SMS backlog[BACKLOG_CAP];
int backlog_idx = 0;
int backlog_len = 0;

const uint8_t BTN_ORDER[32] = {
	0x00,
	0x01, 0x02, 0x04, 0x08, 0x10,
	0x03,
	0x05, 0x06,
	0x09, 0x0A, 0x0C,
	0x11, 0x12, 0x14, 0x18,
	0x07,
	0x0B, 0x0D,
	0x0E, 0x13, 0x15,
	0x16, 0x19, 0x1A, 0x1C,
	0x0F, 0x17, 0x1B, 0x1D, 0x1E,
	0x1F,
};

const uint8_t TOGGLE_ORDER[12] = {
	0x00, 0x01, 0x02, 0x04, 0x08, 0x10,
	0x03, 0x06, 0x0C, 0x09, 0x05, 0x0A,
	//0x11, 0x12, 0x14, 0x18,
};

// Sonic 2 [v1]
static bool bot_state_is_bad(struct SMSGlobal *G, struct SMS *sms)
{
	uint8_t state0 = sms->ram[0x1500 + 0x02];
	uint8_t state1 = sms->ram[0x1500 + 0x02];
	if(state0 == 0x1E || state0 == 0x1F || state0 == 0x19 || state0 == 0x28) {
		return true;
	}
	if(state1 == 0x1E || state1 == 0x1F || state1 == 0x19 || state0 == 0x28) {
		return true;
	}

	return false;
}

void bot_update(struct SMSGlobal *G)
{
	//G->current.ram[0xD23E & 0x1FFF] = 1;

	bool shift_left = (SDL_GetModState() & KMOD_LSHIFT);
	uint8_t old_j0 = G->current.joy[0];
	G->current.joy[1] = 0xF2;

	//printf("%3d\n", backlog_len);
	if(shift_left && backlog_len > 3) {
		backlog_len -= 2;
		backlog_idx -= 2;
		backlog_idx += BACKLOG_CAP;
		backlog_idx %= BACKLOG_CAP;
		uint8_t j0 = G->current.joy[0];
		uint8_t j1 = G->current.joy[1];
		sms_copy(&G->current, &backlog[backlog_idx]);
		G->current.joy[0] = j0;
		G->current.joy[1] = j1;
	}

	sms_copy(&backlog[backlog_idx], &G->current);
	backlog_len++;
	backlog_idx++;
	backlog_idx %= BACKLOG_CAP;
	if(backlog_len > BACKLOG_CAP) {
		backlog_len = BACKLOG_CAP;
	}

	if(bot_state_is_bad(G, &G->current)) {
		printf("BAD STATE! PERFORMING RETROSPECTIVE FIX\n");

		const int btn_len = 25;
		const int dist_step = 3;
		for(int dist = 1; ; dist+=dist_step) {
			assert(dist < BACKLOG_CAP-5);
			assert(dist < backlog_len-5);

			// Get starting index
			int sidx = backlog_idx-dist-1;
			sidx += BACKLOG_CAP*2;
			sidx %= BACKLOG_CAP;

			// Perform simulations
			struct SMS sms_temp;
			int best_diff = 0x7FFFFFFF;
			uint8_t best_j0 = 0;
			bool has_safe_state = false;

			for(int jset = 0; jset < 6; jset++) {
				// Copy state
				sms_copy(&sms_temp, &backlog[sidx]);

				// Select thing to toggle
				uint8_t j0 = (1<<jset);

				// Probe
				bool this_state_is_safe = true;
				for(int soffs = 0; soffs < dist; soffs++) {
					int nsidx = (sidx+soffs)%BACKLOG_CAP;
					sms_temp.joy[0] = backlog[nsidx].joy[0];
					if(soffs < btn_len) {
						sms_temp.joy[0] ^= j0;
					}
					lemu_run_frame(&(G->H), &sms_temp, true);

					if(bot_state_is_bad(G, &sms_temp)) {
						//printf("BAD %02X %d %d %d %d\n", j0, sidx, soffs, nsidx, dist);
						this_state_is_safe = false;
						break;
					}
				}

				// If state is safe, add as a proposal
				if(this_state_is_safe) {
					has_safe_state = true;

					int diff = 0;
					// Sonic 1
					//int32_t px1 = (*(int32_t *)(&G->current.ram[0x13FD])<<8)>>8;
					//int32_t px2 = (*(int32_t *)(&sms_temp.ram[0x13FD])<<8)>>8;
					// Sonic 2 [v1]
					int32_t px1 = (*(int32_t *)(&G->current.ram[0x1510])<<8)>>8;
					int32_t px2 = (*(int32_t *)(&sms_temp.ram[0x1510])<<8)>>8;
					diff = px2-px1;
					/*
					for(int i = 0; i < 8192; i++) {
						int b0 = sms_temp.ram[i];
						int b1 = G->current.ram[i];
						int d = (int)(int8_t)(b0-b1);
						diff += d*d;
					}
					*/

					if(diff < best_diff) {
						best_diff = diff;
						best_j0 = j0;
					}
				}
			}

			// If we have a safe state, use our best one
			if(has_safe_state) {
				// Copy state
				sms_copy(&sms_temp, &backlog[sidx]);

				// Select thing to toggle
				uint8_t j0 = best_j0;
				for(int soffs = 0; soffs < dist; soffs++) {
					int nsidx = (sidx+soffs)%BACKLOG_CAP;
					sms_temp.joy[0] = backlog[nsidx].joy[0];
					if(soffs < btn_len) {
						sms_temp.joy[0] ^= j0;
					}
					sms_copy(&backlog[nsidx], &sms_temp);
					lemu_run_frame(&(G->H), &sms_temp, true);
					assert(!bot_state_is_bad(G, &sms_temp));
				}
				sms_copy(&G->current, &sms_temp);
				G->current.joy[0] = old_j0;
				G->H.twait = time_now()-20000;
				printf("RECREATE %d\n", dist);
				break;
			}
		}
	}

	//printf("%02X\n", G->current.ram[0x12AA]);
	for(int i = 0; i < 0x3F; i++) { printf(" %02X", G->current.ram[i+0x1500]); }
	printf("\n");

	//bool shift_left = (SDL_GetModState() & KMOD_LSHIFT);
	//bool shift_right = (SDL_GetModState() & KMOD_RSHIFT);
	//if(!(shift_left || shift_right)) { return; }

	//int32_t px1 = (*(int32_t *)(&sms->ram[0x13FD])<<8)>>8;
	//G->H.twait = time_now()-20000;
}
