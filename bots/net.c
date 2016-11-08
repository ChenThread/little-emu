#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include <SDL.h>
#include "littlesms.h"

#define BACKLOG_CAP 1024
static struct SMS backlog[BACKLOG_CAP];
static uint8_t input_log[BACKLOG_CAP][2];
static uint32_t backlog_idx = 0;

static int player_control = 1;

uint8_t bot_hook_input(struct SMS *sms, uint64_t timestamp, int port)
{
	SDL_Event ev;
	if(!sms->no_draw) {
	while(SDL_PollEvent(&ev)) {
		switch(ev.type) {
			case SDL_KEYDOWN:
				if((player_control&1) != 0) {
				switch(ev.key.keysym.sym)
				{
					case SDLK_w: sms->joy[0] &= ~0x01; break;
					case SDLK_s: sms->joy[0] &= ~0x02; break;
					case SDLK_a: sms->joy[0] &= ~0x04; break;
					case SDLK_d: sms->joy[0] &= ~0x08; break;
					case SDLK_KP_2: sms->joy[0] &= ~0x10; break;
					case SDLK_KP_3: sms->joy[0] &= ~0x20; break;
					default:
						break;
				}
				}

				if((player_control&2) != 0) {
				switch(ev.key.keysym.sym)
				{
					case SDLK_w: sms->joy[0] &= ~0x40; break;
					case SDLK_s: sms->joy[0] &= ~0x80; break;
					case SDLK_a: sms->joy[1] &= ~0x01; break;
					case SDLK_d: sms->joy[1] &= ~0x02; break;
					case SDLK_KP_2: sms->joy[1] &= ~0x04; break;
					case SDLK_KP_3: sms->joy[1] &= ~0x08; break;
					default:
						break;
				}
				}
				break;

			case SDL_KEYUP:
				if((player_control&1) != 0) {
				switch(ev.key.keysym.sym)
				{
					case SDLK_w: sms->joy[0] |= 0x01; break;
					case SDLK_s: sms->joy[0] |= 0x02; break;
					case SDLK_a: sms->joy[0] |= 0x04; break;
					case SDLK_d: sms->joy[0] |= 0x08; break;
					case SDLK_KP_2: sms->joy[0] |= 0x10; break;
					case SDLK_KP_3: sms->joy[0] |= 0x20; break;
					default:
						break;
				}
				}

				if((player_control&2) != 0) {
				switch(ev.key.keysym.sym)
				{
					case SDLK_w: sms->joy[0] |= 0x40; break;
					case SDLK_s: sms->joy[0] |= 0x80; break;
					case SDLK_a: sms->joy[1] |= 0x01; break;
					case SDLK_d: sms->joy[1] |= 0x02; break;
					case SDLK_KP_2: sms->joy[1] |= 0x04; break;
					case SDLK_KP_3: sms->joy[1] |= 0x08; break;
					default:
						break;
				}
				}
				break;

			case SDL_QUIT:
				exit(0);
				break;
			default:
				break;
		}
	}
	}

	return sms->joy[port&1];
}

void bot_update()
{
	// Make note of inputs
	uint8_t frame_j0 = sms_current.joy[0];
	uint8_t frame_j1 = sms_current.joy[1];

	// Restore inputs
	sms_current.joy[0] = frame_j0;
	sms_current.joy[1] = frame_j1;

	// Save frame
	uint32_t idx = (backlog_idx&(BACKLOG_CAP-1));
	sms_copy(&backlog[idx], &sms_current);
	input_log[idx][0] = frame_j0;
	input_log[idx][1] = frame_j1;
	backlog_idx++;

	//twait = time_now()-20000;
}

void bot_init(int argc, char *argv[])
{
	printf("Args (%d):", argc);
	for(int i = 0; i < argc; i++) {
		printf(" [%s]", argv[i]);
	}
	printf("\n");

	sms_current.joy[0] = 0xFF;
	sms_current.joy[1] = 0xFF;
}

