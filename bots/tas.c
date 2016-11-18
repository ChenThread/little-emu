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
struct SMS backlog0[BACKLOG0_CAP];
struct SMS backlog1[BACKLOG1_CAP];
uint8_t input_log[INPUT_CAP][2];
int backlog_idx = 0;

static void drop_frame(struct SMSGlobal *G)
{
	// Rewind one frame
	int i0 = backlog_idx/BACKLOG0_CAP;
	//int b0 = i0*BACKLOG0_CAP;
	backlog_idx--;
	assert(backlog_idx >= 0);
	assert(backlog_idx < INPUT_CAP);
	int i1 = backlog_idx/BACKLOG0_CAP;
	int b1 = i1*BACKLOG0_CAP;

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
			sms_run_frame(G, &backlog0[i]);
			backlog0[i].no_draw = false;
			backlog0[i].joy[0] = input_log[i+b1][0];
			backlog0[i].joy[1] = input_log[i+b1][1];
		}
	}

	// Copy state
	sms_copy(&G->current, &backlog0[backlog_idx % BACKLOG0_CAP]);
}

static void save_frame(struct SMSGlobal *G)
{
	sms_copy(&backlog0[backlog_idx % BACKLOG0_CAP], &G->current);
	if((backlog_idx % BACKLOG0_CAP) == 0) {
		sms_copy(&backlog1[backlog_idx / BACKLOG0_CAP], &G->current);
	}
	backlog_idx++;
}

void bot_update(struct SMSGlobal *G)
{
	bool shift_left = (SDL_GetModState() & KMOD_LSHIFT);

	uint8_t frame_j0 = G->current.joy[0];
	uint8_t frame_j1 = G->current.joy[1];

	if(shift_left && backlog_idx > 0) {
		drop_frame(G);
		if(backlog_idx > 0) {
			drop_frame(G);
		}
	}

	assert(backlog_idx >= 0);
	assert(backlog_idx < INPUT_CAP);
	input_log[backlog_idx][0] = frame_j0;
	input_log[backlog_idx][1] = frame_j1;
	G->current.joy[0] = frame_j0;
	G->current.joy[1] = frame_j1;
	//printf("STATE %9d %016llX\n", backlog_idx, (unsigned long long)(G->current.timestamp));
	save_frame(G);

	//twait = time_now()-20000;
}

