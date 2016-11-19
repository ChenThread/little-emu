#include "system/sms/all.h"

const uint64_t lemu_core_frame_wait = FRAME_WAIT;

const char lemu_core_name[] = "Sega Master System ("
#if USE_NTSC
"NTSC"
#else
"PAL"
#endif
", "
"SMS2 "
"VDP)";

const char *lemu_core_inputs_global[1] = {
	NULL
};
const char *lemu_core_inputs_player[7] = {
	"Up", "Down", "Left", "Right",
	"1", "2",
	NULL
};

struct SMSGlobal sms_glob;
void (*sms_hook_poll_input)(struct SMSGlobal *G, struct SMS *sms, int controller, uint64_t timestamp) = NULL;

void sms_init(struct SMSGlobal *G, struct SMS *sms)
{
	*sms = (struct SMS){ .H={.timestamp = 0,}, };
	sms->paging[3] = 0; // 0xFFFC
	sms->paging[0] = 0; // 0xFFFD
	sms->paging[1] = 1; // 0xFFFE
	sms->paging[2] = 2; // 0xFFFF
	sms->joy[0] = 0xFF;
	sms->joy[1] = 0xFF;
	sms->memcfg = 0xAB;
	sms->iocfg = 0xFF;
	sms->hlatch = 0x80; // TODO: find out what this is on reset
	sms_z80_init(&(G->H), &(sms->z80));
	sms_vdp_init(&(G->H), &(sms->vdp));
	sms_psg_init(&(G->H), &(sms->psg));
	//sms->z80.H.timestamp = 1;
	//sms->vdp.H.timestamp = 0;

	sms->ram[0] = 0xAB;
}

void sms_copy(struct SMS *dest, struct SMS *src)
{
	memcpy(dest, src, sizeof(struct SMS));
}

void sms_run(struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp)
{
	if(!TIME_IN_ORDER(sms->H.timestamp, timestamp)) {
		return;
	}

	//uint64_t dt = timestamp - sms->H.timestamp;
	while(TIME_IN_ORDER(sms->z80.H.timestamp_end, timestamp)) {
		sms->z80.H.timestamp_end = timestamp;
		sms_vdp_estimate_line_irq(&(sms->vdp), &(G->H), &(sms->H), sms->vdp.H.timestamp);
		//printf("%016lX %016lX %016lX %016lX %016lX\n", timestamp, sms->z80.H.timestamp, sms->z80.H.timestamp_end, sms->vdp.H.timestamp, sms->vdp.H.timestamp_end);
		sms_z80_run(&(sms->z80), &(G->H), &(sms->H), sms->z80.H.timestamp_end);
		sms_vdp_run(&(sms->vdp), &(G->H), &(sms->H), sms->z80.H.timestamp_end);
		sms_psg_run(&(sms->psg), &(G->H), &(sms->H), sms->z80.H.timestamp_end);
	}

	sms->H.timestamp = timestamp;
}

void sms_run_frame(struct SMSGlobal *G, struct SMS *sms)
{
	const int pt_VINT1 = 684*(FRAME_START_Y+0xC1) + (94-18*2);
#if !USE_NTSC
	const int pt_VINT2 = 684*(FRAME_START_Y+0xE1) + (94-18*2);
	//const int pt_VINT3 = 684*(FRAME_START_Y+0xF1) + (94-18*2);
#endif

	// Run a frame
	if(sms->H.timestamp == 0) {
		sms->z80.H.timestamp = pt_VINT1;// - (pt_VINT1%684);
		sms_run(G, sms, sms->H.timestamp + pt_VINT1);
	}
#if USE_NTSC
	sms_run(G, sms, sms->H.timestamp + 684*SCANLINES-pt_VINT1);
#else
	sms_run(G, sms, sms->H.timestamp + pt_VINT2-pt_VINT1);
	sms_run(G, sms, sms->H.timestamp + 684*SCANLINES-pt_VINT2);
	// FIXME: V-centre the frame properly so this doesn't break
	//sms_run(G, sms, sms->H.timestamp + pt_VINT3-pt_VINT2);
	//sms_run(G, sms, sms->H.timestamp + 684*SCANLINES-pt_VINT3);
#endif
	sms_run(G, sms, sms->H.timestamp + pt_VINT1);

	//sms_copy(&sms_prev, &sms_current);
}

static void sms_rom_load(struct SMSGlobal *G, const char *fname, const void *data, size_t len)
{
	memset(G->rom, 0xFF, sizeof(G->rom));

	assert(len <= sizeof(G->rom));

	// Copy ROM
	G->rom_is_banked = false;
	G->rom_len = len;
	memcpy(G->rom, data, len);

	// Check if this is an SGC file
	if(!memcmp(G->rom, "SGC\x1A", 4)) {
		// It is - read header
		printf("SGC file detected - creating player\n");
		assert(G->rom[0x04] == 0x01);
		// ignore PAL/NTSC flag
		// ignore scanline flag
		// ignore reserved byte

		uint8_t load_lo = G->rom[0x08];
		uint8_t load_hi = G->rom[0x09];
		uint8_t init_lo = G->rom[0x0A];
		uint8_t init_hi = G->rom[0x0B];
		uint8_t play_lo = G->rom[0x0C];
		uint8_t play_hi = G->rom[0x0D];
		uint8_t sp_lo = G->rom[0x0E];
		uint8_t sp_hi = G->rom[0x0F];

		uint8_t rst_ptrs[8][2]; // first is ignored.
		memcpy(rst_ptrs[0], G->rom+0x10, 16);
		uint8_t mapper_init_vals[4];
		memcpy(mapper_init_vals, G->rom+0x20, 4);
		uint8_t song_beg = G->rom[0x24];
		uint8_t song_total = G->rom[0x25];
		//uint8_t sfx_beg = G->rom[0x26];
		//uint8_t sfx_end = G->rom[0x27];

		uint8_t sgc_sys_type = G->rom[0x28];
		assert(sgc_sys_type == 0x00 || sgc_sys_type == 0x01); // SMS/GG ONLY!

		// Load
		size_t load_addr = ((size_t)load_lo)+(((size_t)load_hi)<<8);
		printf("Load address: %04X\n", (unsigned int)load_addr);
		assert(load_addr >= 0x00400);
		assert(load_addr < sizeof(G->rom));
		assert(load_addr+G->rom_len-0xA0 <= sizeof(G->rom));
		memmove(&G->rom[load_addr], &G->rom[0xA0], G->rom_len-0xA0);
		memset(G->rom, 0x00, load_addr);
		memset(G->rom+load_addr+(G->rom_len-0xA0), 0x00, sizeof(G->rom)-(load_addr+(G->rom_len-0xA0)));
		G->rom_len -= 0xA0;
		G->rom_len += load_addr;

		//
		// Build stub
		//

		uint8_t loader_stub_init[] = {
			0xF3, // DI
			0xED, 0x56, // IM 1
			0x06, song_beg, // LD B, $nn
			0xC3, 0x80, 0x00, // JP $0080
		};
		memcpy(&G->rom[0x0000], loader_stub_init, sizeof(loader_stub_init));
		// XXX: currently not supporting RST $38
		for(int i = 1; i < 7; i++) {
			G->rom[i*8+0x00] = 0xC3; // JP $nnnn
			G->rom[i*8+0x01] = rst_ptrs[i][0];
			G->rom[i*8+0x02] = rst_ptrs[i][1];
		}
		// TODO: make NMI do something
		G->rom[0x0066] = 0xED; G->rom[0x0067] = 0x45; // RETN

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
		memcpy(&G->rom[0x0038], loader_stub_irq, sizeof(loader_stub_irq));

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
		memcpy(&G->rom[0x0080], loader_stub_maininit, sizeof(loader_stub_maininit));
		memcpy(&G->rom[0x0200], loader_stub_loop, sizeof(loader_stub_loop));
		G->rom[0x007C] = song_beg;
		G->rom[0x007D] = 0xFF;
		G->rom[0x007E] = song_beg;
		G->rom[0x007F] = song_beg+song_total-1;

		G->rom_is_banked = true;
	}

	// TODO: handle other sizes
	printf("ROM size: %08X\n", (int)G->rom_len);
	if(G->rom_len <= 48*1024) {
		// Unbanked
		printf("Fill unbanked\n");
		//memset(&G->rom[G->rom_len], 0xFF, sizeof(G->rom)-G->rom_len);

	} else {
		// Banked
		G->rom_is_banked = true;
		if(G->rom_len <= 128*1024) {
			printf("Copy 128KB -> 256KB\n");
			memcpy(&G->rom[128*1024], G->rom, 128*1024);
		}
		if(G->rom_len <= 256*1024) {
			printf("Copy 256KB -> 512KB\n");
			memcpy(&G->rom[256*1024], G->rom, 256*1024);
		}
		if(G->rom_len <= 512*1024) {
			printf("Copy 512KB -> 1MB\n");
			memcpy(&G->rom[512*1024], G->rom, 512*1024);
		}
		if(G->rom_len <= 1*1024*1024) {
			printf("Copy 1MB -> 2MB\n");
			memcpy(&G->rom[1*1024*1024], G->rom, 1*1024*1024);
		}
		if(G->rom_len <= 2*1024*1024) {
			printf("Copy 2MB -> 4MB\n");
			memcpy(&G->rom[2*1024*1024], G->rom, 2*1024*1024);
		}
	}
}

static void sms_init_global(struct SMSGlobal *G, const char *fname, const void *data, size_t len)
{
	*G = (struct SMSGlobal){
		.H = {
			.core_name = lemu_core_name,
			.common_header_len = sizeof(struct EmuGlobal),
			.full_global_len = sizeof(struct SMSGlobal),
			.state_len = sizeof(struct SMS),

			.ram_count = 3,
			.rom_count = 2,

			.chicken_pointer_count = 0,
			.chicken_pointers = NULL,

			.input_button_count = 6,
			.player_count = 2,

			.no_draw = false,
		},
	};

	G->H.current_state = &(G->current);
	G->H.twait = 0;

	sms_rom_load(G, fname, data, len);

	G->ram_heads[0] = (struct EmuRamHead){
		.len = sizeof(G->current.ram),
		.ptr = G->current.ram,
		.flags = 0,
		.name = "Z80 RAM",
	};
	G->ram_heads[1] = (struct EmuRamHead){
		.len = sizeof(G->current.vdp.vram),
		.ptr = G->current.vdp.vram,
		.flags = 0,
		.name = "VDP VRAM",
	};
	G->ram_heads[2] = (struct EmuRamHead){
		.len = sizeof(G->current.vdp.cram),
		.ptr = G->current.vdp.cram,
		.flags = 0,
		.name = "VDP CRAM",
	};

	G->rom_heads[0] = (struct EmuRomHead){
		.len = G->rom_len,
		.ptr = G->rom,
		.flags = 0,
		.name = "Cartridge ROM",
	};
	G->rom_heads[1] = (struct EmuRomHead){
		.len = 0,
		.ptr = NULL,
		.flags = 0,
		.name = "BIOS ROM",
	};

	sms_init(G, &(G->current));
}

struct EmuGlobal *lemu_core_global_new(const char *fname, const void *data, size_t len)
{
	assert(data != NULL);
	assert(len > 0);

	struct SMSGlobal *G = malloc(sizeof(struct SMSGlobal));
	assert(G != NULL);
	sms_init_global(G, fname, data, len);

	return &(G->H);
}

void lemu_core_global_free(struct EmuGlobal *G)
{
	free(G);
}

void lemu_core_run_frame(struct EmuGlobal *G, void *sms, bool no_draw)
{
	bool old_no_draw = no_draw;
	G->no_draw = no_draw;
	sms_run_frame((struct SMSGlobal *)G, (struct SMS *)sms);
	G->no_draw = old_no_draw;
}

void lemu_core_state_init(struct EmuGlobal *G, void *state)
{
	sms_init((struct SMSGlobal *)G, (struct SMS *)state);
}

void lemu_core_audio_callback(struct EmuGlobal *G, void *state, uint8_t *stream, int len)
{
	sms_psg_pop_16bit_mono((int16_t *)stream, len >> 1);
}

void lemu_core_surface_configure(struct EmuGlobal *G, struct EmuSurface *S)
{
	S->width = 342;
	S->height = SCANLINES;
	S->format = EMU_SURFACE_FORMAT_BGRA_32;
}

void lemu_core_video_callback(struct EmuGlobal *G, struct EmuSurface *S)
{
	for(int y = 0; y < SCANLINES; y++) {
		uint32_t *pp = (uint32_t *)(((uint8_t *)S->pixels) + S->pitch*y);
		for(int x = 0; x < 342; x++) {
			uint32_t v = ((struct SMSGlobal *)G)->frame_data[y][x];
			uint32_t r = ((v>>0)&3)*0x55;
			uint32_t g = ((v>>2)&3)*0x55;
			uint32_t b = ((v>>4)&3)*0x55;
			pp[x] = (b<<24)|(g<<16)|(r<<8);
		}
	}
}

void lemu_core_handle_input(struct EmuGlobal *G, void *state, int player_id, int input_id, bool down)
{
	int joy_id = (player_id * 6) + input_id;
	struct SMS *sms = (struct SMS*) state;

	if (down) sms->joy[joy_id >> 3] &= ~(1 << (joy_id & 7));
	else sms->joy[joy_id >> 3] |= (1 << (joy_id & 7));
}

