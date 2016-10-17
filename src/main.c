#include "common.h"

void *botlib = NULL;
void (*botlib_update)(void) = NULL;

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;

/*
Sonic 1:
0x13FD = X
0x1400 = Y
0x1403 = Xvel
0x1406 = Yvel
*/

void bot_update()
{
	if(botlib_update != NULL) {
		botlib_update();
	}
}

int main(int argc, char *argv[])
{
	assert(argc > 1);
	FILE *fp = fopen(argv[1], "rb");
	assert(fp != NULL);
	memset(sms_rom, 0xFF, sizeof(sms_rom));
	int rsiz = fread(sms_rom, 1, sizeof(sms_rom), fp);
	assert(rsiz > 0);

	// Load bot if available
	if(argc > 2) {
		botlib = SDL_LoadObject(argv[2]);
		assert(botlib != NULL);
		botlib_update = SDL_LoadFunction(botlib, "bot_update");
	}

	// TODO: handle other sizes
	printf("ROM size: %08X\n", rsiz);
	if(rsiz <= 48*1024) {
		// Unbanked
		sms_rom_is_banked = false;
		printf("Fill unbanked\n");
		//memset(&sms_rom[rsiz], 0xFF, sizeof(sms_rom)-rsiz);

	} else {
		// Banked
		sms_rom_is_banked = true;
		if(rsiz <= 128*1024) {
			printf("Copy 128KB -> 256KB\n");
			memcpy(&sms_rom[128*1024], sms_rom, 128*1024);
		}
		if(rsiz <= 256*1024) {
			printf("Copy 256KB -> 512KB\n");
			memcpy(&sms_rom[256*1024], sms_rom, 256*1024);
		}
	}

	// Set up SMS
	sms_init(&sms_current);

	// Set up SDL
	SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow("littlesms",
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

	// Run
	twait = time_now();
	for(;;) {
		struct SMS *sms = &sms_current;
		bot_update();
		sms_run_frame(sms);

		uint64_t tnow = time_now();
		twait += 20000;
		if(sms->no_draw) {
			twait = tnow;
		} else {
			if(TIME_IN_ORDER(tnow, twait)) {
				usleep((useconds_t)(twait-tnow));
			}
		}

		if(!sms->no_draw)
		{
			// Draw + upscale
			void *pixels = NULL;
			int pitch = 0;
			SDL_LockTexture(texture, NULL, &pixels, &pitch);
			for(int y = 0; y < SCANLINES; y++) {
				uint32_t *pp = (uint32_t *)(((uint8_t *)pixels) + pitch*y);
				for(int x = 0; x < 342; x++) {
					uint32_t v = frame_data[y][x];
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
	}
	
	return 0;
}

