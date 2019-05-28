#if USE_NTSC
#define SCANLINES 263
#define VCLKS_WIDE 3413
#else
#define SCANLINES 314
#define VCLKS_WIDE 3406
#endif

#define PIXELS_STEP (4)
#define PIXELS_WIDE ((VCLKS_WIDE+PIXELS_STEP-1)/(PIXELS_STEP))
#define FRAME_WAIT ((int)((44100*0x300*11)/(7*VCLKS_WIDE*SCANLINES)))

struct GPU
{
	struct EmuState H;
	union {
		uint32_t w[1024<<8];
		uint16_t p[1024<<9];
	} vram;
	union {
		uint32_t w[2048>>2];
		uint16_t p[2048>>1];
	} tcache;
	union {
		uint32_t w[64>>1];
		uint16_t p[64>>0];
	} ccache;

	uint32_t cmd_fifo[16];
	uint32_t cmd_count;

	// Transfer stuff
	enum {
		PSX_GPU_XFER_NONE = 0,
		PSX_GPU_XFER_FROM_GPU,
		PSX_GPU_XFER_TO_GPU,
	} xfer_mode;
	enum {
		PSX_GPU_DMA_OFF = 0,
		PSX_GPU_DMA_FIFO,
		PSX_GPU_DMA_CPU_TO_GP0,
		PSX_GPU_DMA_GPUREAD_TO_CPU,
	} xfer_dma;
	uint32_t xfer_addr;
	uint32_t xfer_width;
	uint32_t xfer_stride;
	uint32_t xfer_xrem;
	uint32_t xfer_yrem;

	// Display rectangle
	uint32_t screen_x0, screen_y0;
	uint32_t screen_x1, screen_y1;
	uint32_t screen_div;
	uint32_t disp_addr;

	// Drawing rectangle
	uint32_t draw_x0, draw_y0;
	uint32_t draw_x1, draw_y1;
	int32_t draw_ox, draw_oy;

	uint32_t status;
} __attribute__((__packed__));

