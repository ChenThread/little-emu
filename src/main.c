#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

#ifdef DEDI
#include <dlfcn.h>
#else
#include <SDL.h>
#endif

// TODO: get more than one thing working (needs API!)
#include "system/sms/all.h"

void *botlib = NULL;
void (*botlib_init)(struct EmuGlobal *G, int argc, char *argv[]) = NULL;
void (*botlib_update)(struct EmuGlobal *G) = NULL;
uint8_t (*botlib_hook_input)(struct EmuGlobal *G, void *sms, uint64_t timestamp, int port) = NULL;

#ifndef DEDI
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;
#endif

struct EmuGlobal *Gbase = NULL;

void bot_update()
{
	if(botlib_update != NULL) {
		botlib_update(Gbase);
	}
}

// FIXME make + use generic API
uint8_t input_fetch(struct EmuGlobal *globals, void *state, uint64_t timestamp, int port)
{
	//struct SMSGlobal *G = (struct SMSGlobal *)globals;
	struct SMS *sms = (struct SMS *)state;
#ifndef DEDI
	//printf("input %016llX %d\n", (unsigned long long)timestamp, port);

	SDL_Event ev;
	if(!sms->no_draw) {
	while(SDL_PollEvent(&ev)) {
		switch(ev.type) {
			case SDL_KEYDOWN:
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
				} break;

			case SDL_KEYUP:
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
				} break;

			case SDL_QUIT:
				exit(0);
				break;
			default:
				break;
		}
	}
	}
#endif
	//printf("OUTPUT: %02X\n", sms->joy[port&1]);
	return sms->joy[port&1];
}

#ifndef DEDI
void audio_callback_sdl(void *ud, Uint8 *stream, int len)
{
	psg_pop_16bit_mono((int16_t *)stream, len/2);
}
#endif

int main(int argc, char *argv[])
{
	assert(argc > 1);
	FILE *fp = fopen(argv[1], "rb");
	assert(fp != NULL);
	static uint8_t rom_buffer[1024*1024*4];
	int rsiz = fread(rom_buffer, 1, sizeof(rom_buffer), fp);
	assert(rsiz > 0);

	// Load bot if available
	if(argc > 2) {
#ifdef DEDI
		botlib = dlopen(argv[2], RTLD_NOW);
		assert(botlib != NULL);
		botlib_init = dlsym(botlib, "bot_init");
		botlib_update = dlsym(botlib, "bot_update");
		botlib_hook_input = dlsym(botlib, "bot_hook_input");
#else
		botlib = SDL_LoadObject(argv[2]);
		assert(botlib != NULL);
		botlib_init = SDL_LoadFunction(botlib, "bot_init");
		botlib_update = SDL_LoadFunction(botlib, "bot_update");
		botlib_hook_input = SDL_LoadFunction(botlib, "bot_hook_input");
#endif
	}

	if(botlib_hook_input == NULL) {
		botlib_hook_input = input_fetch;
	}

	// Set up global + state
	Gbase = lemu_global_new(argv[1], rom_buffer, rsiz);
	assert(Gbase != NULL);
	lemu_state_init(Gbase, Gbase->current_state);

#ifndef DEDI
	// Set up SDL
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	char window_title_buf[256];
	snprintf(window_title_buf, sizeof(window_title_buf)-1,
		"little-emu - core: %s", Gbase->core_name);
	window_title_buf[255] = '\x00';
	window = SDL_CreateWindow(window_title_buf,
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		342*2,
		SCANLINES*2,
		0);
	renderer = SDL_CreateSoftwareRenderer(SDL_GetWindowSurface(window));
	texture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_BGRX8888,
		SDL_TEXTUREACCESS_STREAMING,
		342, SCANLINES);

	// Tell SDL to get stuffed
	signal(SIGINT,  SIG_DFL);
	signal(SIGTERM, SIG_DFL);

	// Set up audio
	SDL_AudioSpec au_want;
	au_want.freq = 48000;
	au_want.format = AUDIO_S16;
	au_want.channels = 1;
	au_want.samples = 2048;
	au_want.callback = audio_callback_sdl;
	SDL_OpenAudio(&au_want, NULL);
#endif

	// Run
	if(botlib_init != NULL) {
		botlib_init(Gbase, argc-2, argv+2);
	}

#ifndef DEDI
	SDL_PauseAudio(0);
#endif
	Gbase->twait = time_now();
	for(;;) {
		// FIXME make + use generic API
		struct SMS *sms = (struct SMS *)Gbase->current_state;
		struct SMS sms_ndsim;
		sms->joy[0] = botlib_hook_input(Gbase, sms, sms->timestamp, 0);
		sms->joy[1] = botlib_hook_input(Gbase, sms, sms->timestamp, 1);
		bot_update();
		lemu_copy(Gbase, &sms_ndsim, sms);
		lemu_run_frame(Gbase, sms, false);
		/*
		sms_ndsim.no_draw = true;
		lemu_run_frame(Gbase, &sms_ndsim, true);
		sms_ndsim.no_draw = false;
		assert(memcmp(&sms_ndsim, sms, sizeof(struct SMS)) == 0);
		*/

		uint64_t tnow = time_now();
		Gbase->twait += FRAME_WAIT;
		if(sms->no_draw) {
			Gbase->twait = tnow;
		} else {
			if(TIME_IN_ORDER(tnow, Gbase->twait)) {
				usleep((useconds_t)(Gbase->twait-tnow));
			}
		}

#ifndef DEDI
		if(!sms->no_draw)
		{
			// Draw + upscale
			void *pixels = NULL;
			int pitch = 0;
			SDL_LockTexture(texture, NULL, &pixels, &pitch);
			for(int y = 0; y < SCANLINES; y++) {
				uint32_t *pp = (uint32_t *)(((uint8_t *)pixels) + pitch*y);
				for(int x = 0; x < 342; x++) {
					// FIXME make API then use it
					uint32_t v = ((struct SMSGlobal *)Gbase)->frame_data[y][x];
					uint32_t r = ((v>>0)&3)*0x55;
					uint32_t g = ((v>>2)&3)*0x55;
					uint32_t b = ((v>>4)&3)*0x55;
					pp[x] = (b<<24)|(g<<16)|(r<<8);
				}
			}
			SDL_UnlockTexture(texture);
			SDL_RenderCopy(renderer, texture, NULL, NULL);

			// Update
			SDL_UpdateWindowSurface(window);
		}
#endif
	}
	
	return 0;
}

