#include "system/md/all.h"

const uint64_t lemu_core_frame_wait = FRAME_WAIT;

const char lemu_core_name[] = "Sega Mega Drive ("
#if USE_NTSC
"NTSC"
#else
"PAL"
#endif
", "
"MD1 "
"VDP)";

const char *lemu_core_inputs_global[1] = {
	NULL
};
const char *lemu_core_inputs_player[9] = {
	"Up", "Down", "Left", "Right",
	"Start", "A", "B", "C",
	NULL
};

struct MDGlobal md_glob;
void (*md_hook_poll_input)(struct MDGlobal *G, struct MD *md, int controller, uint64_t timestamp) = NULL;

void md_init(struct MDGlobal *G, struct MD *md)
{
	*md = (struct MD){ .H={.timestamp = 0,}, };
	md_m68k_init(&(G->H), &(md->m68k));
}

void md_copy(struct MD *dest, struct MD *src)
{
	memcpy(dest, src, sizeof(struct MD));
}

void md_run(struct MDGlobal *G, struct MD *md, uint64_t timestamp)
{
	if(!TIME_IN_ORDER(md->H.timestamp, timestamp)) {
		return;
	}

	//uint64_t dt = timestamp - md->H.timestamp;
	while(TIME_IN_ORDER(md->m68k.H.timestamp_end, timestamp)) {
		md->m68k.H.timestamp_end = timestamp;
		//md_vdp_estimate_line_irq(&(md->vdp), &(G->H), &(md->H), md->vdp.H.timestamp);
		md_m68k_run(&(md->m68k), &(G->H), &(md->H), md->m68k.H.timestamp_end);
		//md_vdp_run(&(md->vdp), &(G->H), &(md->H), md->m68k.H.timestamp_end);
		//md_psg_run(&(md->psg), &(G->H), &(md->H), md->m68k.H.timestamp_end);
	}

	md->H.timestamp = timestamp;
}

void md_run_frame(struct MDGlobal *G, struct MD *md)
{
	// XXX: need proper frame timings
	const int pt_VINT1 = 684*(FRAME_START_Y+0xC1) + (94-18*2);
#if !USE_NTSC
	const int pt_VINT2 = 684*(FRAME_START_Y+0xE1) + (94-18*2);
	//const int pt_VINT3 = 684*(FRAME_START_Y+0xF1) + (94-18*2);
#endif

	// Run a frame
	if(md->H.timestamp == 0) {
		md->m68k.H.timestamp = pt_VINT1;// - (pt_VINT1%684);
		md_run(G, md, md->H.timestamp + pt_VINT1);
	}
#if USE_NTSC
	md_run(G, md, md->H.timestamp + 684*SCANLINES-pt_VINT1);
#else
	md_run(G, md, md->H.timestamp + pt_VINT2-pt_VINT1);
	md_run(G, md, md->H.timestamp + 684*SCANLINES-pt_VINT2);
	// FIXME: V-centre the frame properly so this doesn't break
	//md_run(G, md, md->H.timestamp + pt_VINT3-pt_VINT2);
	//md_run(G, md, md->H.timestamp + 684*SCANLINES-pt_VINT3);
#endif
	md_run(G, md, md->H.timestamp + pt_VINT1);

	//md_copy(&md_prev, &md_current);
}

static void md_rom_load(struct MDGlobal *G, const char *fname, const void *data, size_t len)
{
	memset(G->rom, 0xFF, sizeof(G->rom));

	assert(len <= sizeof(G->rom));

	// Copy ROM
	G->rom_len = len;
	memcpy(G->rom, data, len);

	printf("ROM size: %08X\n", (int)G->rom_len);

	// Byteswap ROM
	printf("Byteswapping\n");
	for(int i = 0; i < G->rom_len; i += 2) {
		uint8_t b0 = G->rom[i+0];
		uint8_t b1 = G->rom[i+1];
		G->rom[i+0] = b1;
		G->rom[i+1] = b0;
	}
	//memset(&G->rom[G->rom_len], 0xFF, sizeof(G->rom)-G->rom_len);
}

static void md_init_global(struct MDGlobal *G, const char *fname, const void *data, size_t len)
{
	*G = (struct MDGlobal){
		.H = {
			.core_name = lemu_core_name,
			.common_header_len = sizeof(struct EmuGlobal),
			.full_global_len = sizeof(struct MDGlobal),
			.state_len = sizeof(struct MD),

			.ram_count = 1,
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

	md_rom_load(G, fname, data, len);

	G->ram_heads[0] = (struct EmuRamHead){
		.len = sizeof(G->current.ram),
		.ptr = G->current.ram,
		.flags = 0,
		.name = "M68K RAM",
	};
	G->ram_heads[1] = (struct EmuRamHead){
		.len = sizeof(G->current.zram),
		.ptr = G->current.zram,
		.flags = 0,
		.name = "Z80 RAM",
	};
	/*
	G->ram_heads[2] = (struct EmuRamHead){
		.len = sizeof(G->current.vdp.vram),
		.ptr = G->current.vdp.vram,
		.flags = 0,
		.name = "VDP VRAM",
	};
	G->ram_heads[3] = (struct EmuRamHead){
		.len = sizeof(G->current.vdp.cram),
		.ptr = G->current.vdp.cram,
		.flags = 0,
		.name = "VDP CRAM",
	};
	G->ram_heads[4] = (struct EmuRamHead){
		.len = sizeof(G->current.vdp.vsram),
		.ptr = G->current.vdp.vsram,
		.flags = 0,
		.name = "VDP VSRAM",
	};
	*/

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

	md_init(G, &(G->current));
}

struct EmuGlobal *lemu_core_global_new(const char *fname, const void *data, size_t len)
{
	assert(data != NULL);
	assert(len > 0);

	struct MDGlobal *G = malloc(sizeof(struct MDGlobal));
	assert(G != NULL);
	md_init_global(G, fname, data, len);

	return &(G->H);
}

void lemu_core_global_free(struct EmuGlobal *G)
{
	free(G);
}

void lemu_core_run_frame(struct EmuGlobal *G, void *md, bool no_draw)
{
	bool old_no_draw = no_draw;
	G->no_draw = no_draw;
	md_run_frame((struct MDGlobal *)G, (struct MD *)md);
	G->no_draw = old_no_draw;
}

void lemu_core_state_init(struct EmuGlobal *G, void *state)
{
	md_init((struct MDGlobal *)G, (struct MD *)state);
}

void lemu_core_audio_callback(struct EmuGlobal *G, void *state, uint8_t *stream, int len)
{
	memset(stream, 0x00, len);
	//md_psg_pop_16bit_mono((int16_t *)stream, len >> 1);
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
		memset(pp, 0xFF, S->width*4);
		/*
		for(int x = 0; x < 342; x++) {
			uint32_t v = ((struct MDGlobal *)G)->frame_data[y][x];
			uint32_t r = ((v>>0)&3)*0x55;
			uint32_t g = ((v>>2)&3)*0x55;
			uint32_t b = ((v>>4)&3)*0x55;
			pp[x] = (b<<24)|(g<<16)|(r<<8);
		}
		*/
	}
}

void lemu_core_handle_input(struct EmuGlobal *G, void *state, int player_id, int input_id, bool down)
{
	int joy_id = (player_id * 8) + input_id;
	struct MD *md = (struct MD*) state;

	if (down) md->joy[joy_id >> 3] &= ~(1 << (joy_id & 7));
	else md->joy[joy_id >> 3] |= (1 << (joy_id & 7));
}

