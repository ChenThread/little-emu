#include "system/c64/all.h"

#define RGB_TO_BGRA(r, g, b) (((b) << 24) | ((g) << 16) | ((r) << 8))

uint32_t vicii_colors[] = {
	0x00000000,
	0xFFFFFF00,
	RGB_TO_BGRA(104, 55, 43),
	RGB_TO_BGRA(112, 164, 178),
	RGB_TO_BGRA(111, 61, 134),
	RGB_TO_BGRA(88, 141, 67),
	RGB_TO_BGRA(53, 40, 121),
	RGB_TO_BGRA(184, 199, 111),
	RGB_TO_BGRA(111, 79, 37),
	RGB_TO_BGRA(67, 57, 0),
	RGB_TO_BGRA(154, 103, 89),
	RGB_TO_BGRA(68, 68, 68),
	RGB_TO_BGRA(108, 108, 108),
	RGB_TO_BGRA(154, 210, 132),
	RGB_TO_BGRA(108, 94, 181),
	RGB_TO_BGRA(149, 149, 149),
};

const char lemu_core_name[] = "Commodore 64 (PAL)";
const uint64_t lemu_core_frame_wait = FRAME_WAIT;

void c64_init(struct C64Global *G, struct C64 *c64)
{
	*c64 = (struct C64){
		.H={.timestamp = 0,},
		.cpu_io0 = 0xFF,
		.cpu_io1 = 0xC3,
		.key_matrix = 0,
	};
	cpu_6502_init(&(G->H), &(c64->H), &(c64->cpu));
	vic_init(&(G->H), &(c64->vic));
	cia1_init(&(G->H), &(c64->cia1));
	cia2_init(&(G->H), &(c64->cia2));
}

static void c64_bin_load(uint8_t* ptr, const char *fname, size_t len) {
	FILE* file;
	file = fopen(fname, "rb");
	if (file != NULL) {
		fread(ptr, 1, len, file);
		fclose(file);
	} else {
		fprintf(stderr, "File not found! %s",fname);
	}
}

static void c64_rom_load(struct C64Global *G, const char *fname, const void *data, size_t len)
{
	char *result = strstr(fname, ".prg");
	memset(G->rom_cartridge, 0xFF, sizeof(G->rom_cartridge));

	if (result != NULL) {
		uint8_t *data8 = (uint8_t*) data;
		uint16_t addr = data8[0] | (data8[1] << 8);
		memcpy(&G->current.ram[addr], &data8[2], len - 2);
	} else {
		memcpy(G->rom_cartridge, data, len);
		G->rom_cartridge_present = true;
	}
}

static void c64_init_global(struct C64Global *G, const char *fname, const void *data, size_t len)
{
	*G = (struct C64Global){
		.H = {
			.core_name = lemu_core_name,
			.common_header_len = sizeof(struct EmuGlobal),
			.full_global_len = sizeof(struct C64Global),
			.state_len = sizeof(struct C64),

			.ram_count = 1,
			.rom_count = 4,

			.chicken_pointer_count = 0,
			.chicken_pointers = NULL,

			.input_button_count = 64,
			.player_count = 1,

			.no_draw = false,
		},
	};

	G->H.current_state = &(G->current);
	G->H.twait = 0;

	G->ram_heads[0] = (struct EmuRamHead){
		.len = 0x10000,
		.ptr = G->current.ram,
		.flags = 0,
		.name = "RAM",
	};

	G->rom_heads[0] = (struct EmuRomHead){
		.len = 0x2000,
		.ptr = G->rom_basic,
		.flags = 0,
		.name = "BASIC ROM",
	};
	G->rom_heads[1] = (struct EmuRomHead){
		.len = 0x2000,
		.ptr = G->rom_kernal,
		.flags = 0,
		.name = "Kernal ROM",
	};
	G->rom_heads[2] = (struct EmuRomHead){
		.len = 0x1000,
		.ptr = G->rom_char,
		.flags = 0,
		.name = "Character ROM",
	};
	G->rom_heads[3] = (struct EmuRomHead){
		.len = 0x4000,
		.ptr = G->rom_cartridge,
		.flags = 0,
		.name = "Cartridge ROM",
	};

	c64_bin_load(G->rom_basic, "basic.901226-01.bin", 0x2000);
	c64_bin_load(G->rom_kernal, "kernal.901227-03.bin", 0x2000);
	c64_bin_load(G->rom_char, "characters.901225-01.bin", 0x1000);
	c64_rom_load(G, fname, data, len);

	c64_init(G, &(G->current));
}

void c64_run(struct C64Global *G, struct C64 *c64, uint64_t timestamp)
{
	if(!TIME_IN_ORDER(c64->H.timestamp, timestamp)) {
		return;
	}

	while(TIME_IN_ORDER(c64->cpu.H.timestamp_end, timestamp)) {
		c64->cpu.H.timestamp_end = timestamp;
		cpu_6502_run(&(G->H), &(c64->H), &(c64->cpu), c64->cpu.H.timestamp_end);
	}

	c64->H.timestamp = timestamp;
}

void c64_run_frame(struct C64Global *G, struct C64 *c64)
{
	c64_run(G, c64, c64->H.timestamp + (63 * 312));
}

struct EmuGlobal *lemu_core_global_new(const char *fname, const void *data, size_t len)
{
	assert(data != NULL);
	assert(len > 0);

	struct C64Global *G = malloc(sizeof(struct C64Global));
	assert(G != NULL);
	c64_init_global(G, fname, data, len);

	return &(G->H);
}

void lemu_core_global_free(struct EmuGlobal *G)
{
	free(G);
}

void lemu_core_run_frame(struct EmuGlobal *G, void *state, bool no_draw)
{
	bool old_no_draw = no_draw;
	G->no_draw = no_draw;
	c64_run_frame((struct C64Global *) G, (struct C64 *) state);
	G->no_draw = old_no_draw;
}

void lemu_core_state_init(struct EmuGlobal *G, void *state)
{
	c64_init((struct C64Global *)G, (struct C64 *)state);
}

void lemu_core_audio_callback(struct EmuGlobal *G, void *state, uint8_t *stream, int len)
{

}

void lemu_core_surface_configure(struct EmuGlobal *G, struct EmuSurface *S)
{
	S->width = SCREEN_WIDTH;
	S->height = SCREEN_HEIGHT;
	S->format = EMU_SURFACE_FORMAT_BGRA_32;
}

void lemu_core_video_callback(struct EmuGlobal *G, struct EmuSurface *S)
{
	for(int y = 0; y < SCREEN_HEIGHT; y++) {
		uint32_t *pp = (uint32_t *)(((uint8_t *)S->pixels) + S->pitch*y);
		for(int x = 0; x < SCREEN_WIDTH; x++) {
			uint8_t v = ((struct C64Global *)G)->frame_data[y * SCREEN_WIDTH + x];
			pp[x] = vicii_colors[v & 0x0F];
		}
	}
}

void lemu_core_handle_input(struct EmuGlobal *G, void *state, int player_id, int input_id, bool down)
{
	struct C64* c64 = (struct C64*) state;
	if (input_id < 64)
		if (down)
			c64->key_matrix |= ((uint64_t) 1 << input_id);
		else
			c64->key_matrix &= ~((uint64_t) 1 << input_id);
	else {
		// other keys
	}
}