#include "common.h"

void *botlib = NULL;
void (*botlib_init)(struct SMSGlobal *G, int argc, char *argv[]) = NULL;
void (*botlib_update)(struct SMSGlobal *G) = NULL;
uint8_t (*botlib_hook_input)(struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp, int port) = NULL;

#ifndef DEDI
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;
#endif

struct SMSGlobal Gsms;

void bot_update()
{
	if(botlib_update != NULL) {
		botlib_update(&Gsms);
	}
}

uint8_t input_fetch(struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp, int port)
{
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
	memset(Gsms.rom, 0xFF, sizeof(Gsms.rom));
	int rsiz = fread(Gsms.rom, 1, sizeof(Gsms.rom), fp);
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

	Gsms.rom_is_banked = false;

	// Check if this is an SGC file
	if(!memcmp(Gsms.rom, "SGC\x1A", 4)) {
		// It is - read header
		printf("SGC file detected - creating player\n");
		assert(Gsms.rom[0x04] == 0x01);
		// ignore PAL/NTSC flag
		// ignore scanline flag
		// ignore reserved byte

		uint8_t load_lo = Gsms.rom[0x08];
		uint8_t load_hi = Gsms.rom[0x09];
		uint8_t init_lo = Gsms.rom[0x0A];
		uint8_t init_hi = Gsms.rom[0x0B];
		uint8_t play_lo = Gsms.rom[0x0C];
		uint8_t play_hi = Gsms.rom[0x0D];
		uint8_t sp_lo = Gsms.rom[0x0E];
		uint8_t sp_hi = Gsms.rom[0x0F];

		uint8_t rst_ptrs[8][2]; // first is ignored.
		memcpy(rst_ptrs[0], Gsms.rom+0x10, 16);
		uint8_t mapper_init_vals[4];
		memcpy(mapper_init_vals, Gsms.rom+0x20, 4);
		uint8_t song_beg = Gsms.rom[0x24];
		uint8_t song_total = Gsms.rom[0x25];
		//uint8_t sfx_beg = Gsms.rom[0x26];
		//uint8_t sfx_end = Gsms.rom[0x27];

		uint8_t sgc_sys_type = Gsms.rom[0x28];
		assert(sgc_sys_type == 0x00 || sgc_sys_type == 0x01); // SMS/GG ONLY!

		// Load
		size_t load_addr = ((size_t)load_lo)+(((size_t)load_hi)<<8);
		printf("Load address: %04X\n", (unsigned int)load_addr);
		assert(load_addr >= 0x00400);
		assert(load_addr < sizeof(Gsms.rom));
		assert(load_addr+rsiz-0xA0 <= sizeof(Gsms.rom));
		memmove(&Gsms.rom[load_addr], &Gsms.rom[0xA0], rsiz-0xA0);
		memset(Gsms.rom, 0x00, load_addr);
		memset(Gsms.rom+load_addr+(rsiz-0xA0), 0x00, sizeof(Gsms.rom)-(load_addr+(rsiz-0xA0)));
		rsiz -= 0xA0;
		rsiz += load_addr;

		//
		// Build stub
		//

		uint8_t loader_stub_init[] = {
			0xF3, // DI
			0xED, 0x56, // IM 1
			0x06, song_beg, // LD B, $nn
			0xC3, 0x80, 0x00, // JP $0080
		};
		memcpy(&Gsms.rom[0x0000], loader_stub_init, sizeof(loader_stub_init));
		// XXX: currently not supporting RST $38
		for(int i = 1; i < 7; i++) {
			Gsms.rom[i*8+0x00] = 0xC3; // JP $nnnn
			Gsms.rom[i*8+0x01] = rst_ptrs[i][0];
			Gsms.rom[i*8+0x02] = rst_ptrs[i][1];
		}
		// TODO: make NMI do something
		Gsms.rom[0x0066] = 0xED; Gsms.rom[0x0067] = 0x45; // RETN

		// Interrupt handler
		uint8_t loader_stub_irq[] = {
			0xF5, // PUSH AF
			0xC5, // PUSH BC
			0xD5, // PUSH DE
			0xE5, // PUSH HL
			0xDD, 0xE5, // PUSH IX
			0xFD, 0xE5, // PUSH IY
			0xDB, 0xBF, // IN A, ($BF)
			0xCD, play_lo, play_hi, // CALL play
			0xFD, 0xE1, // POP IY
			0xDD, 0xE1, // POP IX
			0xE1, // POP HL
			0xD1, // POP DE
			0xC1, // POP BC
			0xF1, // POP AF
			//0xFB, // EI
			0xED, 0x4D, // RETI
		};
		memcpy(&Gsms.rom[0x0038], loader_stub_irq, sizeof(loader_stub_irq));

		// Init sequence
		// TODO: clear RAM
		uint8_t loader_stub_maininit[] = {
			0xF3, // DI
			0xED, 0x56, // IM 1
			0x31, sp_lo, sp_hi, // LD SP, $nnnn

			0x3E, mapper_init_vals[0], // LD A, $nn
			0x32, 0xFC, 0xFF, // LD ($FFFC), A
			0x3E, mapper_init_vals[1], // LD A, $nn
			0x32, 0xFD, 0xFF, // LD ($FFFD), A
			0x3E, mapper_init_vals[2], // LD A, $nn
			0x32, 0xFE, 0xFF, // LD ($FFFE), A
			0x3E, mapper_init_vals[3], // LD A, $nn
			0x32, 0xFF, 0xFF, // LD ($FFFF), A

			0x3E, 0x00, // LD A, $00
			0x32, 0xF7, 0xDF, // LD ($DFF7), A
			0x78, // LD A, B
			0x32, 0xF6, 0xDF, // LD ($DFF6), A

			0x21, 0x00, 0xC0, // LD HL, $C000
			0x11, 0x01, 0xC0, // LD DE, $C001
			0x01, 0xF0, 0x1F, // LD BC, $1FF0
			0x36, 0x00, // LD (HL), $00
			0xED, 0xB0, // LDIR

			0x3A, 0xF6, 0xDF, // LD A, ($DFF6)
			0xCD, init_lo, init_hi, // CALL init

			0xDB, 0xBF, // IN A, ($BF)
			0x3E, 0x04, // LD A, $nn
			0xD3, 0xBF, // OUT ($BF), A
			0x3E, 0x80, // LD A, $nn
			0xD3, 0xBF, // OUT ($BF), A
			0x3E, 0x60, // LD A, $nn
			0xD3, 0xBF, // OUT ($BF), A
			0x3E, 0x81, // LD A, $nn
			0xD3, 0xBF, // OUT ($BF), A
			0x3E, 0xFF, // LD A, $nn
			0xD3, 0xBF, // OUT ($BF), A
			0x3E, 0x8A, // LD A, $nn
			0xD3, 0xBF, // OUT ($BF), A
			0xDB, 0xBF, // IN A, ($BF)

			0xC3, 0x00, 0x02, // JP $0200
		};

		uint8_t loader_stub_loop[] = {
			//
			// MAIN LOOP
			//
			0xFB, // EI

			0x76, // HALT
			0x3A, 0xF7, 0xDF, // LD A, ($DFF7)
			0x47, // LD B, A - before
			0xDB, 0xDC, // IN ($DC), A
			0x32, 0xF7, 0xDF, // LD ($DFF7), A
			0x4F, // LD C, A - current

			// Perform comparison
			0x2F, // CPL
			0xA0, // AND B
			0xCB, 0x57, // BIT 2, A
			0x28, 0x08, // JR Z, +

			// LEFT
			0x3A, 0xF6, 0xDF, // LD A, ($DFF6)
			0x3D, // DEC A
			0x47, // LD B, A
			0xC3, 0x80, 0x00, // JP $0080

			0xCB, 0x5F, // BIT 3, A
			0x28, 0x08, // JR Z, +

			// RIGHT
			0x3A, 0xF6, 0xDF, // LD A, ($DFF6)
			0x3C, // INC A
			0x47, // LD B, A
			0xC3, 0x80, 0x00, // JP $0080

			0xC3, 0x00, 0x02, // JP $0200
		};
		memcpy(&Gsms.rom[0x0080], loader_stub_maininit, sizeof(loader_stub_maininit));
		memcpy(&Gsms.rom[0x0200], loader_stub_loop, sizeof(loader_stub_loop));
		Gsms.rom[0x007C] = song_beg;
		Gsms.rom[0x007D] = 0xFF;
		Gsms.rom[0x007E] = song_beg;
		Gsms.rom[0x007F] = song_beg+song_total-1;

		Gsms.rom_is_banked = true;
	}

	// TODO: handle other sizes
	printf("ROM size: %08X\n", rsiz);
	if(rsiz <= 48*1024) {
		// Unbanked
		printf("Fill unbanked\n");
		//memset(&Gsms.rom[rsiz], 0xFF, sizeof(Gsms.rom)-rsiz);

	} else {
		// Banked
		Gsms.rom_is_banked = true;
		if(rsiz <= 128*1024) {
			printf("Copy 128KB -> 256KB\n");
			memcpy(&Gsms.rom[128*1024], Gsms.rom, 128*1024);
		}
		if(rsiz <= 256*1024) {
			printf("Copy 256KB -> 512KB\n");
			memcpy(&Gsms.rom[256*1024], Gsms.rom, 256*1024);
		}
		if(rsiz <= 512*1024) {
			printf("Copy 512KB -> 1MB\n");
			memcpy(&Gsms.rom[512*1024], Gsms.rom, 512*1024);
		}
		if(rsiz <= 1*1024*1024) {
			printf("Copy 1MB -> 2MB\n");
			memcpy(&Gsms.rom[1*1024*1024], Gsms.rom, 1*1024*1024);
		}
		if(rsiz <= 2*1024*1024) {
			printf("Copy 2MB -> 4MB\n");
			memcpy(&Gsms.rom[2*1024*1024], Gsms.rom, 2*1024*1024);
		}
	}

	// Set up SMS
	sms_init(&Gsms, &Gsms.current);

#ifndef DEDI
	// Set up SDL
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
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
	Gsms.current.ram[0] = 0xAB;
	if(botlib_init != NULL) {
		botlib_init(&Gsms, argc-2, argv+2);
	}

#ifndef DEDI
	SDL_PauseAudio(0);
#endif
	Gsms.twait = time_now();
	for(;;) {
		struct SMS *sms = &Gsms.current;
		struct SMS sms_ndsim;
		sms->joy[0] = botlib_hook_input(&Gsms, sms, sms->timestamp, 0);
		sms->joy[1] = botlib_hook_input(&Gsms, sms, sms->timestamp, 1);
		bot_update();
		sms_copy(&sms_ndsim, sms);
		sms_run_frame(&Gsms, sms);
		/*
		sms_ndsim.no_draw = true;
		sms_run_frame(&sms_ndsim);
		sms_ndsim.no_draw = false;
		assert(memcmp(&sms_ndsim, sms, sizeof(struct SMS)) == 0);
		*/

		uint64_t tnow = time_now();
		Gsms.twait += FRAME_WAIT;
		if(sms->no_draw) {
			Gsms.twait = tnow;
		} else {
			if(TIME_IN_ORDER(tnow, Gsms.twait)) {
				usleep((useconds_t)(Gsms.twait-tnow));
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
					uint32_t v = Gsms.frame_data[y][x];
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

