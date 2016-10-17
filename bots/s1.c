#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include <SDL.h>
#include "littlesms.h"

#define BACKLOG_CAP 16384
struct SMS backlog[BACKLOG_CAP];
int backlog_idx = 0;
int backlog_len = 0;

static const uint8_t BTN_ORDER[32] = {
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

static const uint8_t TOGGLE_ORDER[12] = {
	0x00, 0x01, 0x02, 0x04, 0x08, 0x10,
	0x03, 0x06, 0x0C, 0x09, 0x05, 0x0A,
	//0x11, 0x12, 0x14, 0x18,
};

// Sonic 1
const uint8_t F18_GROUNDED = 0x80;
const uint8_t F18_DEAD = 0x20;
const uint8_t F18_IN_WATER = 0x10;
const uint8_t F18_CAN_JUMP = 0x08;
const uint8_t F18_AIR_ROLL = 0x04;
const uint8_t F18_LEFT = 0x02;
const uint8_t F18_ROLLING = 0x01;

const uint8_t E14_WALKING = 0x01;
const uint8_t E14_RUNNING = 0x02;
const uint8_t E14_STANDING = 0x05;
const uint8_t E14_ROLLING = 0x09;
const uint8_t E14_BRAKING = 0x0A;
const uint8_t E14_DYING = 0x0B;
const uint8_t E14_STUNNED = 0x10;

static bool bot_state_is_bad(struct SMS *sms)
{
	uint8_t f18 = sms->ram[0x13FC + 0x18];
	uint8_t e14 = sms->ram[0x13FC + 0x14];
	if((f18 & F18_DEAD) != 0) {
		return true;
	}
	if((e14 == E14_STUNNED) != 0) {
		return true;
	}

	return false;
}

void bot_update()
{
	//sms_current.ram[0xD23E & 0x1FFF] = 17;

	bool shift_left = (SDL_GetModState() & KMOD_LSHIFT);
	uint8_t old_j0 = sms_current.joy[0];

	//printf("%3d\n", backlog_len);
	if(shift_left && backlog_len > 3) {
		backlog_len -= 2;
		backlog_idx -= 2;
		backlog_idx += BACKLOG_CAP;
		backlog_idx %= BACKLOG_CAP;
		uint8_t j0 = sms_current.joy[0];
		uint8_t j1 = sms_current.joy[1];
		sms_copy(&sms_current, &backlog[backlog_idx]);
		sms_current.joy[0] = j0;
		sms_current.joy[1] = j1;
	}

	sms_copy(&backlog[backlog_idx], &sms_current);
	backlog_len++;
	backlog_idx++;
	backlog_idx %= BACKLOG_CAP;
	if(backlog_len > BACKLOG_CAP) {
		backlog_len = BACKLOG_CAP;
	}

	if(bot_state_is_bad(&sms_current)) {
		printf("BAD STATE! PERFORMING RETROSPECTIVE FIX\n");

		for(int dist = 1; ; dist+=1) {
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

				// Set joypad + disable video
				sms_temp.no_draw = true;

				// Select thing to toggle
				uint8_t j0 = (1<<jset);

				// Probe
				bool this_state_is_safe = true;
				for(int soffs = 0; soffs < dist; soffs++) {
					int nsidx = (sidx+soffs)%BACKLOG_CAP;
					sms_temp.joy[0] = backlog[nsidx].joy[0] ^ j0;
					sms_run_frame(&sms_temp);

					if(bot_state_is_bad(&sms_temp)) {
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
					//int32_t px1 = (*(int32_t *)(&sms_current.ram[0x13FD])<<8)>>8;
					//int32_t px2 = (*(int32_t *)(&sms_temp.ram[0x13FD])<<8)>>8;
					// Sonic 2 [v1]
					int32_t px1 = (*(int32_t *)(&sms_current.ram[0x1510])<<8)>>8;
					int32_t px2 = (*(int32_t *)(&sms_temp.ram[0x1510])<<8)>>8;
					diff = px2-px1;
					/*
					for(int i = 0; i < 8192; i++) {
						int b0 = sms_temp.ram[i];
						int b1 = sms_current.ram[i];
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

				// Set joypad + disable video
				sms_temp.no_draw = true;

				// Select thing to toggle
				uint8_t j0 = best_j0;
				for(int soffs = 0; soffs < dist; soffs++) {
					int nsidx = (sidx+soffs)%BACKLOG_CAP;
					sms_temp.joy[0] = backlog[nsidx].joy[0] ^ j0;
					sms_copy(&backlog[nsidx], &sms_temp);
					backlog[nsidx].no_draw = false;
					sms_run_frame(&sms_temp);
					assert(!bot_state_is_bad(&sms_temp));
				}
				sms_copy(&sms_current, &sms_temp);
				sms_current.no_draw = false;
				sms_current.joy[0] = old_j0;
				twait = time_now()-20000;
				printf("RECREATE %d\n", dist);
				break;
			}
		}
	}

	//printf("%02X\n", sms_current.ram[0x12AA]);
	for(int i = 0; i < 0x1A; i++) { printf(" %02X", sms_current.ram[i+0x13FC]); }
	printf("\n");

	//bool shift_left = (SDL_GetModState() & KMOD_LSHIFT);
	//bool shift_right = (SDL_GetModState() & KMOD_RSHIFT);
	//if(!(shift_left || shift_right)) { return; }

	//int32_t px1 = (*(int32_t *)(&sms->ram[0x13FD])<<8)>>8;
	//twait = time_now()-20000;
}

