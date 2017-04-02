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

#include "littleemu.h"

void *botlib = NULL;
void (*botlib_init)(struct EmuGlobal *G, int argc, char *argv[]) = NULL;
void (*botlib_update)(struct EmuGlobal *G) = NULL;
void (*botlib_hook_input)(struct EmuGlobal *G, void *state, uint64_t timestamp) = NULL;

#ifndef DEDI
#ifndef TARGET_PSX
#define TARGET_PSX 0
#endif

// TODO: unhardcode
#if TARGET_PSX
static SDL_Keycode keymap[] = {
	SDLK_BACKSLASH, SDLK_LALT, SDLK_RALT, SDLK_RETURN,
	SDLK_w, SDLK_d, SDLK_s, SDLK_a,
	SDLK_LSHIFT, SDLK_RSHIFT, SDLK_LCTRL, SDLK_RCTRL,
	SDLK_i, SDLK_l, SDLK_k, SDLK_j,
};
#else
static SDL_Keycode keymap[] = {
	SDLK_w, SDLK_s, SDLK_a, SDLK_d, SDLK_KP_2, SDLK_KP_3
};
#endif

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;
struct EmuSurface *Gsurface = NULL;
#endif

struct EmuGlobal *Gbase = NULL;

void bot_update()
{
	if(botlib_update != NULL) {
		botlib_update(Gbase);
	}
}

void input_fetch(struct EmuGlobal *G, void *state, uint64_t timestamp)
{
#ifndef DEDI
	int i;
	SDL_Event ev;

	//printf("input %016llX %d\n", (unsigned long long)timestamp, port);

	if(!Gbase->no_draw) {
	while(SDL_PollEvent(&ev)) {
		switch(ev.type) {
			case SDL_KEYDOWN:
				for (i = 0; i < G->input_button_count; i++)
					if (ev.key.keysym.sym == keymap[i])
						lemu_handle_input(G, state, 0, i, true);
				break;
			case SDL_KEYUP:
				for (i = 0; i < G->input_button_count; i++)
					if (ev.key.keysym.sym == keymap[i])
						lemu_handle_input(G, state, 0, i, false);
				break;
			case SDL_QUIT:
				exit(0);
				break;
			default:
				break;
		}
	}
	}
#endif
}

#ifndef DEDI
void audio_callback_sdl(void *ud, Uint8 *stream, int len)
{
	lemu_audio_callback(Gbase, Gbase->current_state, stream, len);
}
#endif

#ifdef ROM_IS_PROVIDED
extern uint8_t rom_buffer[];
extern uint8_t rom_buffer_end[];
#endif

int main(int argc, char *argv[])
{
#ifndef ROM_IS_PROVIDED
	assert(argc > 1);
	FILE *fp = fopen(argv[1], "rb");
	assert(fp != NULL);
#ifdef ROM_BUFFER_SIZE
	static uint8_t rom_buffer[ROM_BUFFER_SIZE];
#else
	static uint8_t rom_buffer[1024*1024*4];
#endif
	int rsiz = fread(rom_buffer, 1, sizeof(rom_buffer), fp);
	assert(rsiz > 0);
	fclose(fp);
#else
	int rsiz = (int)(rom_buffer_end - rom_buffer);
#endif

#ifndef STATIC_VER
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
#endif

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

	// Set up video
	Gsurface = lemu_surface_new(Gbase);

	int gscale_w = (Gsurface->width*2 >= 1360 ? 1 : 2);
	int gscale_h = (Gsurface->height*2 >= 768 ? 1 : 2);
	window = SDL_CreateWindow(window_title_buf,
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		Gsurface->width * gscale_w,
		Gsurface->height * gscale_h,
		0);
	renderer = SDL_CreateSoftwareRenderer(SDL_GetWindowSurface(window));

	texture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_BGRX8888, // TODO: more pixel formats
		SDL_TEXTUREACCESS_STREAMING,
		Gsurface->width, Gsurface->height);

	// Tell SDL to get stuffed
	signal(SIGINT,  SIG_DFL);
	signal(SIGTERM, SIG_DFL);

	// Set up audio
	// TODO: Add audio format negotiation?
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
		struct EmuState *state = (struct EmuState *)Gbase->current_state;
		//struct SMS *sms = (struct SMS *)Gbase->current_state;
		botlib_hook_input(Gbase, state, state->timestamp);
		bot_update();
		//lemu_copy(Gbase, &sms_ndsim, sms);
		lemu_run_frame(Gbase, state, false);

		// useful snippet
		/*
		struct SMS sms_ndsim;
		sms_ndsim.no_draw = true;
		lemu_run_frame(Gbase, &sms_ndsim, true);
		sms_ndsim.no_draw = false;
		assert(memcmp(&sms_ndsim, sms, sizeof(struct SMS)) == 0);
		*/

		uint64_t tnow = time_now();
		Gbase->twait += lemu_frame_wait_get();
		if(Gbase->no_draw) {
			Gbase->twait = tnow;
		} else {
			if(TIME_IN_ORDER(tnow, Gbase->twait)) {
				usleep((useconds_t)(Gbase->twait-tnow));
			}
		}

#ifndef DEDI
		if(!Gbase->no_draw)
		{
			// Draw + upscale
			SDL_LockTexture(texture, NULL, &(Gsurface->pixels), &(Gsurface->pitch));
			lemu_video_callback(Gbase, Gsurface);
			SDL_UnlockTexture(texture);
			SDL_RenderCopy(renderer, texture, NULL, NULL);

			// Update
			SDL_UpdateWindowSurface(window);
		}
#endif
	}

#ifndef DEDI
	lemu_surface_free(Gsurface);
#endif
	lemu_global_free(Gbase);
	
	return 0;
}

