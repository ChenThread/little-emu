#include "system/psx/all.h"

const uint64_t lemu_core_frame_wait = FRAME_WAIT;

const char lemu_core_name[] = "Sony PlayStation ("
#if USE_NTSC
"NTSC"
#else
"PAL"
#endif
", "

"SCPH-550"
#if USE_NTSC
"1"
#else
"2"
#endif
")";

#if USE_NTSC
#define PSX_BIOS_FILE "sysroms/scph5501.bin"
#else
#define PSX_BIOS_FILE "sysroms/scph5502.bin"
#endif

#if 0
// I love this hack. --GM
// Disabled because clang now crashes. --GM
uint32_t psx_bios_data[512<<8];
extern const uint32_t psx_bios_data_src[512<<8];
//extern const uint32_t psx_bios_data[];
asm (
	".data\n"
	".global psx_bios_data_src\n"
	"psx_bios_data_src: .incbin \"" PSX_BIOS_FILE "\"\n"
	".text\n"
);
#else
uint32_t psx_bios_data[512<<8];
#endif

const char *lemu_core_inputs_global[1] = {
	NULL
};
const char *lemu_core_inputs_player[17] = {
	"Select", "L3", "R3", "Start",
	"Up", "Right", "Down", "Left",
	"L2", "R2", "L1", "R1",
	"Triangle", "Circle", "X", "Square",
	NULL
};

struct PSXGlobal psx_glob;
void (*psx_hook_poll_input)(struct PSXGlobal *G, struct PSX *psx, int controller, uint64_t timestamp) = NULL;

void psx_plant_exe(struct PSXGlobal *G, struct PSX *psx)
{
	uint32_t init_pc = *(uint32_t *)(G->rom+0x010);
	uint32_t init_gp = *(uint32_t *)(G->rom+0x014);
	uint32_t dest_beg = *(uint32_t *)(G->rom+0x018);
	uint32_t fsize = *(uint32_t *)(G->rom+0x01C);
	printf("%08X %08X %08X %08X\n\n\n", init_pc, init_gp, dest_beg, fsize);
	assert((fsize & 0x7FF) == 0);
	uint32_t init_sp = *(uint32_t *)(G->rom+0x030);

	dest_beg &= 0x1FFFFFFF;
	uint32_t dest_end = dest_beg + fsize;
	assert(dest_beg >= 0);
	assert(dest_beg < sizeof(psx->ram));
	assert(dest_end <= sizeof(psx->ram));

	memcpy(((uint8_t *)(psx->ram))+dest_beg, G->rom+0x800, fsize);
	(void)init_sp;
	(void)init_gp;
	//
	// HLE EXE load
	//
	psx->mips.exe_init_pc = init_pc;
	psx->mips.exe_init_gp = init_gp;
	psx->mips.exe_init_sp = init_sp;
	psx->mips.is_in_bios = false;
	psx->mips.pc = init_pc;
	psx->mips.gpr[GPR_GP] = init_gp;
	psx->mips.gpr[GPR_SP] = init_sp;
}

void psx_init(struct PSXGlobal *G, struct PSX *psx)
{
	*psx = (struct PSX){ .H={.timestamp = 0,}, };
	psx->joy[0].buttons = 0xFFFF;
	psx->joy[1].buttons = 0xFFFF;
	psx_mips_init(&(G->H), &(psx->mips));
	psx_gpu_init(&(G->H), &(psx->gpu));
	psx->mips.is_in_bios = true;
	//psx_spu_init(&(G->H), &(psx->spu));
	//psx->mips.H.timestamp = 1;
	//psx->gpu.H.timestamp = 0;

	if(psx_bios_data[0] == 0) {
		FILE *fp = fopen(PSX_BIOS_FILE, "rb");
		assert(fp != NULL);
		fread(psx_bios_data, 1, sizeof(psx_bios_data), fp);
		fclose(fp);
		//memcpy(psx_bios_data, psx_bios_data_src, sizeof(psx_bios_data));

#if 1
		// HACK: Force-enable the UART
		// (only tested on SCPH-5502 BIOS - may break on SCPH-5501!)
		// This is basically nocash's patch.
		// Sorry for the backwards endianness, it's binutils's fault.
		// 6f04:       9806f00f        jal     0xfc01a60
		// 6f08:       0e000424        li      a0,14
		// 6f0c:       01a0013c        lui     at,0xa001
		// 6f10:       e119f00f        jal     0xfc06784
		// 6f14:       b0b920ac        sw      zero,-18000(at)

		if(true
			&& psx_bios_data[0x6F0C>>2] == 0x3C01A001
			&& psx_bios_data[0x6F10>>2] == 0x0FF019E1
			&& psx_bios_data[0x6F14>>2] == 0xAC20B9B0
			)
		{
			psx_bios_data[0x6F08>>2] |= 0x10;
			psx_bios_data[0x6F0C>>2] = 0x34010001;
			psx_bios_data[0x6F10>>2] = 0x0FF019E1;
			psx_bios_data[0x6F14>>2] = 0xAFA1A9C0;
			printf("UART BIOS patch applied! (PAL ver 1)\n");

		} else {
			printf("Cannot apply UART BIOS patch.\n");
		}
#endif
	}

#if 0
	psx_plant_exe(G, psx);
#endif
}

void psx_copy(struct PSX *dest, struct PSX *src)
{
	memcpy(dest, src, sizeof(struct PSX));
}

void psx_run(struct PSXGlobal *G, struct PSX *psx, uint64_t timestamp)
{
	if(!TIME_IN_ORDER(psx->H.timestamp, timestamp)) {
		return;
	}

	//uint64_t dt = timestamp - psx->H.timestamp;
	while(TIME_IN_ORDER(psx->mips.H.timestamp_end, timestamp)) {
		if((psx->i_mask & psx->i_stat) != 0) {
			psx->mips.cop0reg[0x0D] |= 0x0400;
		} else {
			psx->mips.cop0reg[0x0D] &= ~0x0400;
		}
		psx->mips.H.timestamp_end = timestamp;
		psx_mips_run(&(psx->mips), &(G->H), &(psx->H), psx->mips.H.timestamp_end);
		psx_gpu_run(&(psx->gpu), &(G->H), &(psx->H), psx->mips.H.timestamp_end);
		//psx_spu_run(&(psx->spu), &(G->H), &(psx->H), psx->mips.H.timestamp_end);
	}
	psx->i_stat |= (1<<0); // VBLANK

	psx->H.timestamp = timestamp;
}

void psx_run_frame(struct PSXGlobal *G, struct PSX *psx)
{
	// Run a frame
	//
	//psx_run(G, psx, psx->H.timestamp + 684*SCANLINES-pt_VINT1);
	psx_run(G, psx, psx->H.timestamp + VCLKS_WIDE*7*SCANLINES);

	//psx_copy(&psx_prev, &psx_current);
}

static void psx_rom_load(struct PSXGlobal *G, const char *fname, const void *data, size_t len)
{
	G->rom_len = len;

	memset(G->rom, 0xFF, sizeof(G->rom));
	assert(len <= sizeof(G->rom));

	// Copy ROM
	memcpy(G->rom, data, len);
	printf("ROM size: %08X\n", (int)G->rom_len);

	// Parse EXE
	// TODO: ISO support (needs CD-ROM emulation!)
	if(memcmp(G->rom+0, "PS-X EXE", 8)) {
		fprintf(stderr, "FATAL ERROR: expected \"PS-X EXE\"\n");
		fflush(stderr); abort();
	}

	uint32_t fsize = *(uint32_t *)(G->rom+0x01C);
	assert((fsize & 0x7FF) == 0);
}

static void psx_init_global(struct PSXGlobal *G, const char *fname, const void *data, size_t len)
{
	*G = (struct PSXGlobal){
		.H = {
			.core_name = lemu_core_name,
			.common_header_len = sizeof(struct EmuGlobal),
			.full_global_len = sizeof(struct PSXGlobal),
			.state_len = sizeof(struct PSX),

			.ram_count = 3,
			.rom_count = 2,

			.chicken_pointer_count = 0,
			.chicken_pointers = NULL,

			.input_button_count = 16,
			.player_count = 2,

			.no_draw = false,
		},
	};

	G->H.current_state = &(G->current);
	G->H.twait = 0;

	psx_rom_load(G, fname, data, len);

	G->ram_heads[0] = (struct EmuRamHead){
		.len = sizeof(G->current.ram),
		.ptr = G->current.ram,
		.flags = 0,
		.name = "MIPS RAM",
	};
	G->ram_heads[1] = (struct EmuRamHead){
		//.len = sizeof(G->current.vram),
		//.ptr = G->current.vdp.vram,
		.len = 0,
		.ptr = NULL,
		.flags = 0,
		.name = "GPU VRAM",
	};
	G->ram_heads[2] = (struct EmuRamHead){
		//.len = sizeof(G->current.sram),
		//.ptr = G->current.vdp.cram,
		.len = 0,
		.ptr = NULL,
		.flags = 0,
		.name = "SPU RAM",
	};

	G->rom_heads[0] = (struct EmuRomHead){
		.len = G->rom_len,
		.ptr = G->rom,
		.flags = 0,
		.name = "Boot EXE/CD",
	};
	G->rom_heads[1] = (struct EmuRomHead){
		.len = 0,
		.ptr = NULL,
		.flags = 0,
		.name = "BIOS ROM",
	};

	psx_init(G, &(G->current));
}

struct EmuGlobal *lemu_core_global_new(const char *fname, const void *data, size_t len)
{
	assert(data != NULL);
	assert(len > 0);

	//printf("Global size %08X\n", (uint32_t)sizeof(struct PSXGlobal));
	//printf("State  size %08X\n", (uint32_t)sizeof(struct PSX));
	struct PSXGlobal *G = malloc(sizeof(struct PSXGlobal));
	assert(G != NULL);
	psx_init_global(G, fname, data, len);

	return &(G->H);
}

void lemu_core_global_free(struct EmuGlobal *G)
{
	free(G);
}

void lemu_core_run_frame(struct EmuGlobal *G, void *psx, bool no_draw)
{
	bool old_no_draw = no_draw;
	G->no_draw = no_draw;
	psx_run_frame((struct PSXGlobal *)G, (struct PSX *)psx);
	G->no_draw = old_no_draw;
}

void lemu_core_state_init(struct EmuGlobal *G, void *state)
{
	psx_init((struct PSXGlobal *)G, (struct PSX *)state);
}

void lemu_core_audio_callback(struct EmuGlobal *G, void *state, uint8_t *stream, int len)
{
	//psx_psg_pop_16bit_mono((int16_t *)stream, len >> 1);
	memset(stream, 0, len);
}

void lemu_core_surface_configure(struct EmuGlobal *G, struct EmuSurface *S)
{
	S->width = PIXELS_WIDE;
	S->height = SCANLINES;
	S->format = EMU_SURFACE_FORMAT_BGRA_32;
}

void lemu_core_video_callback(struct EmuGlobal *G, struct EmuSurface *S)
{
	for(int y = 0; y < SCANLINES; y++) {
		uint32_t *pp = (uint32_t *)(((uint8_t *)S->pixels) + S->pitch*y);
		for(int x = 0; x < PIXELS_WIDE; x++) {
			uint32_t v = ((struct PSXGlobal *)G)->frame_data[y][x];
			//uint32_t v = 0x00FFFFFF;
			uint32_t r = ((v>>0)&0xFF);
			uint32_t g = ((v>>8)&0xFF);
			uint32_t b = ((v>>16)&0xFF);
			pp[x] = (b<<24)|(g<<16)|(r<<8);
			//pp[x] = v;
		}
	}
}

void lemu_core_handle_input(struct EmuGlobal *G, void *state, int player_id, int input_id, bool down)
{
	int joy_id = (player_id * 6) + input_id;
	struct PSX *psx = (struct PSX*) state;

	if (down) psx->joy[(joy_id >> 4)&1].buttons &= ~(1 << (joy_id & 15));
	else psx->joy[(joy_id >> 4)&1].buttons |= (1 << (joy_id & 15));
}

