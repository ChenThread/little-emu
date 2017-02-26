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

#include "littleemu.h"
#include <citro3d.h>
#include "shader_shbin.h"

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

#define TEXTURE_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(1) | GX_TRANSFER_OUT_TILED(1) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

void *botlib = NULL;
void (*botlib_init)(struct EmuGlobal *G, int argc, char *argv[]) = NULL;
void (*botlib_update)(struct EmuGlobal *G) = NULL;
void (*botlib_hook_input)(struct EmuGlobal *G, void *state, uint64_t timestamp) = NULL;

#ifndef DEDI
struct EmuSurface *Gsurface = NULL;
#endif

struct EmuGlobal *Gbase = NULL;

static DVLB_s* shader_dvlb;
static shaderProgram_s shader_prog;
static int uLoc_projection;
static int realWidth, realHeight;
static float tLeft, tTop, tRight, tBottom;
static C3D_Mtx projection;
static C3D_Tex render_tex;

static u8* audio_buffer;
static ndspWaveBuf ndsp_buffer[2];
static int buffer_size;
static bool soundFillBlock = false;
static bool keepRunning = true;

#define OFFSET_X -24
#define OFFSET_Y 46

void bot_update()
{
	if(botlib_update != NULL) {
		botlib_update(Gbase);
	}
}

void input_fetch(struct EmuGlobal *G, void *state, uint64_t timestamp)
{
#ifndef DEDI
	if(!Gbase->no_draw) {
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_UP) lemu_handle_input(G, state, 0, 0, true);
		if (kDown & KEY_DOWN) lemu_handle_input(G, state, 0, 1, true);
		if (kDown & KEY_LEFT) lemu_handle_input(G, state, 0, 2, true);
		if (kDown & KEY_RIGHT) lemu_handle_input(G, state, 0, 3, true);
		if (kDown & (KEY_A | KEY_X)) lemu_handle_input(G, state, 0, 4, true);
		if (kDown & (KEY_B | KEY_Y)) lemu_handle_input(G, state, 0, 5, true);

		u32 kUp = hidKeysUp();
		if (kUp & KEY_UP) lemu_handle_input(G, state, 0, 0, false);
		if (kUp & KEY_DOWN) lemu_handle_input(G, state, 0, 1, false);
		if (kUp & KEY_LEFT) lemu_handle_input(G, state, 0, 2, false);
		if (kUp & KEY_RIGHT) lemu_handle_input(G, state, 0, 3, false);
		if (kUp & (KEY_A | KEY_X)) lemu_handle_input(G, state, 0, 4, false);
		if (kUp & (KEY_B | KEY_Y)) lemu_handle_input(G, state, 0, 5, false);
		if (kUp & KEY_START) keepRunning = false;
	}
#endif
}

#ifndef DEDI
void audio_callback_3ds(void *ud)
{
	if (ndsp_buffer[soundFillBlock].status == NDSP_WBUF_DONE) {
		lemu_audio_callback(Gbase, Gbase->current_state, (u8*) ndsp_buffer[soundFillBlock].data_pcm16, buffer_size);
		DSP_FlushDataCache(ndsp_buffer[soundFillBlock].data_pcm16, buffer_size);
		ndspChnWaveBufAdd(0, &ndsp_buffer[soundFillBlock]);
		soundFillBlock = !soundFillBlock;
	}
}
#endif

int main(int argc, char *argv[])
{
	FILE *fp = fopen("/sms/chaos.sms", "rb");
	assert(fp != NULL);
	static uint8_t rom_buffer[1024*1024*4];
	int rsiz = fread(rom_buffer, 1, sizeof(rom_buffer), fp);

	if(botlib_hook_input == NULL) {
		botlib_hook_input = input_fetch;
	}

	// Set up global + state
	Gbase = lemu_global_new(argv[1], rom_buffer, rsiz);
	assert(Gbase != NULL);
	lemu_state_init(Gbase, Gbase->current_state);

#ifndef DEDI
	// Set up graphics
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	consoleInit(GFX_BOTTOM, NULL);

	C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetClear(target, C3D_CLEAR_ALL, 0x000000FF, 0);
	C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

	// Set up shader
	shader_dvlb = DVLB_ParseFile((u32*) shader_shbin, shader_shbin_size);
	shaderProgramInit(&shader_prog);
	shaderProgramSetVsh(&shader_prog, &shader_dvlb->DVLE[0]);
	C3D_BindProgram(&shader_prog);

	C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3);
	AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2);

	Mtx_OrthoTilt(&projection, 0.0, 400.0, 240.0, 0.0, 0.0, 1.0, true);

	C3D_BufInfo* bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);

	uLoc_projection = shaderInstanceGetUniformLocation(shader_prog.vertexShader, "projection");

	// Set up texture
	Gsurface = lemu_surface_new(Gbase);
	realWidth = 512;
	realHeight = 512;

	Gsurface->pitch = realWidth * 4;
	Gsurface->pixels = linearAlloc(realWidth * realHeight * 4);

	C3D_TexInitVRAM(&render_tex, realWidth, realHeight, GPU_RGBA8);
	C3D_TexSetFilter(&render_tex, GPU_NEAREST, GPU_NEAREST);

	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, 0, 0);
	C3D_TexEnvOp(env, C3D_Both, 0, 0, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

	tLeft = OFFSET_X / (float) realWidth;
	tRight = (OFFSET_X + 400) / (float) realWidth;
	tTop = OFFSET_Y / (float) realWidth;
	tBottom = (OFFSET_Y + 240) / (float) realWidth;

	// Set up audio
	float mix[12];

	ndspInit();
	memset(mix, 0, sizeof(mix));
	mix[0] = mix[1] = 1.0f;
	ndspSetOutputMode(NDSP_OUTPUT_MONO);
	ndspSetOutputCount(1);
	ndspSetMasterVol(1.0f);

	buffer_size = 8192;
	audio_buffer = linearAlloc(buffer_size * 2);
	memset(audio_buffer, 0, buffer_size * 2);
	ndspSetCallback(audio_callback_3ds, audio_buffer);

	memset(&ndsp_buffer[0], 0, sizeof(ndspWaveBuf));
	memset(&ndsp_buffer[1], 0, sizeof(ndspWaveBuf));
	ndspChnReset(0);
	ndspChnInitParams(0);
	ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
	ndspChnSetRate(0, 48000);
	ndspChnSetFormat(0, NDSP_CHANNELS(1) | NDSP_ENCODING(NDSP_ENCODING_PCM16));
	ndspChnSetMix(0, mix);

	ndsp_buffer[0].data_vaddr = &audio_buffer[0];
	ndsp_buffer[0].nsamples = buffer_size >> 1;
	ndsp_buffer[1].data_vaddr = &audio_buffer[buffer_size];
	ndsp_buffer[1].nsamples = buffer_size >> 1;

	ndspChnWaveBufAdd(0, &ndsp_buffer[0]);
	ndspChnWaveBufAdd(0, &ndsp_buffer[1]);

	// Set up speedups
	osSetSpeedupEnable(1);
	APT_SetAppCpuTimeLimit(80);
#endif

	// Run
	if(botlib_init != NULL) {
		botlib_init(Gbase, argc-2, argv+2);
	}

#ifndef DEDI
//	SDL_PauseAudio(0);
#endif
	Gbase->twait = time_now();
	uint64_t frames = 0;
	uint64_t frameLast = time_now();
	while (aptMainLoop() && keepRunning) {
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
		frames++;
		if (frames == 25) {
			uint64_t avgFrameDur = (tnow - frameLast) / frames;
			float frameCount = 1000000 / (float) avgFrameDur;
			printf("%.2f FPS\n", frameCount);
			frameLast = tnow;
			frames = 0;
		}
		Gbase->twait += lemu_frame_wait_get();
		if(Gbase->no_draw) {
			Gbase->twait = tnow;
		} else {
			if(TIME_IN_ORDER(tnow, Gbase->twait)) {
				svcSleepThread(1000 * (useconds_t)(Gbase->twait-tnow));
			} else {
				// don't speed up the game if it slows down
				Gbase->twait = tnow;
			}
		}

#ifndef DEDI
		if(!Gbase->no_draw)
		{
			// Draw
			GSPGPU_FlushDataCache(Gsurface->pixels, realWidth*realHeight*4);
			lemu_video_callback(Gbase, Gsurface);
			GX_DisplayTransfer((u32*) Gsurface->pixels, GX_BUFFER_DIM(realWidth, realHeight),
				(u32*) render_tex.data, GX_BUFFER_DIM(realWidth, realHeight), TEXTURE_TRANSFER_FLAGS);
			gspWaitForPPF();
			GSPGPU_FlushDataCache((u32*) render_tex.data, realWidth*realHeight*4);

			C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			C3D_FrameDrawOn(target);

			C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);
			C3D_TexBind(0, &render_tex);

			C3D_ImmDrawBegin(GPU_TRIANGLE_STRIP);
				C3D_ImmSendAttrib(0.0f, 0.0f, 0.5f, 0.0f);
				C3D_ImmSendAttrib(tLeft, tTop, 0.0f, 0.0f);
				C3D_ImmSendAttrib(0.0f, 240.0f, 0.5f, 0.0f);
				C3D_ImmSendAttrib(tLeft, tBottom, 0.0f, 0.0f);
				C3D_ImmSendAttrib(400.0f, 0.0f, 0.5f, 0.0f);
				C3D_ImmSendAttrib(tRight, tTop, 0.0f, 0.0f);
				C3D_ImmSendAttrib(400.0f, 240.0f, 0.5f, 0.0f);
				C3D_ImmSendAttrib(tRight, tBottom, 0.0f, 0.0f);
			C3D_ImmDrawEnd();

			C3D_FrameEnd(0);
		}
#endif
	}

	linearFree(audio_buffer);
	ndspExit();

	C3D_Fini();
	gfxExit();

#ifndef DEDI
	lemu_surface_free(Gsurface);
#endif
	lemu_global_free(Gbase);
	
	return 0;
}

