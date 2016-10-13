#include "common.h"

uint8_t sms_rom[512*1024];
bool sms_rom_is_banked = false;

struct SMS sms_current;
struct SMS sms_prev;

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;

uint64_t time_now(void)
{
	struct timeval ts;
	gettimeofday(&ts, NULL);

	uint64_t sec = ts.tv_sec;
	uint64_t usec = ts.tv_usec;
	sec *= 1000000ULL;
	usec += sec;
	return usec;
}

int main(int argc, char *argv[])
{
	assert(argc > 1);
	FILE *fp = fopen(argv[1], "rb");
	assert(fp != NULL);
	memset(sms_rom, 0xFF, sizeof(sms_rom));
	int rsiz = fread(sms_rom, 1, sizeof(sms_rom), fp);
	assert(rsiz > 0);

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
	sms_copy(&sms_prev, &sms_current);

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
	const int pt_VINT = 684*(70+192) + (47-17); // for PAL

	uint64_t twait = time_now();
	for(;;) {
		struct SMS *sms = &sms_current;

		// Run a frame
		sms_run(sms, sms->timestamp + pt_VINT);

		// VINT
		if((sms->vdp.regs[0x01]&0x20) != 0) {
			z80_irq(&sms->z80, sms, 0xFF);
			sms->vdp.status |= 0x80;
		}
		sms_run(sms, sms->timestamp + 684*SCANLINES-pt_VINT);

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

		//sms_copy(&sms_prev, &sms_current);
		// Update
		SDL_UpdateWindowSurface(window);
		uint64_t tnow = time_now();
		twait += 20000;
		if(TIME_IN_ORDER(tnow, twait)) {
			usleep((useconds_t)(twait-tnow));
		}
	}
	
	return 0;
}

