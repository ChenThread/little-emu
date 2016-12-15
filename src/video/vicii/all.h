#if (USE_NTSC) != 0
	#define FRAME_CYCLES 65
	#define FRAME_SCANLINES 263
	#define FRAME_WAIT (1000000/59.825)
	#define DRAW_AREA_START_X 40
	#define DRAW_AREA_START_Y 48
#else
	#define FRAME_CYCLES 63
	#define FRAME_SCANLINES 312
	#define FRAME_WAIT (1000000/50.125)
	#define DRAW_AREA_START_X 40
	#define DRAW_AREA_START_Y 48
#endif

#define FRAME_SCANLINE_WIDTH (FRAME_CYCLES << 3)
#define SCREEN_WIDTH 400
#define SCREEN_HEIGHT FRAME_SCANLINES
#define SCREEN_START_Y 0

#define VIC_CONTROL_1 0x11
#define VIC_CONTROL_2 0x16
#define VIC_RASTER_POS 0x12
#define VIC_BORDER_COLOR 0x0
#define VIC_BACKGROUND_COLOR 0x1
#define VIC_SPRITE_MULTICOLOR 0x5
#define VIC_SPRITE_COLOR 0x7

#define VIC_IRQ_RASTER (1 << 0)
#define VIC_IRQ_COL_BG (1 << 1)
#define VIC_IRQ_COL_SPRITE (1 << 2)
#define VIC_IRQ_LIGHTPEN (1 << 3)

struct VIC {
	struct EmuState H;
	int16_t raster_y, raster_x;

	// debug data
	int frame_counter;

	// row data
	uint8_t row_pixels[40];
	uint8_t row_colors[40];
	uint16_t row_addr;
	uint8_t row_ypos;
	uint8_t border_on, picture_on;
	bool row_fetch;

	// registers
	uint8_t raster_irq;
	
	uint16_t sprite_x[0x08];
	uint8_t sprite_x_msb;
	uint8_t sprite_enable, sprite_prio, sprite_multicolor, sprite_xsize, sprite_ysize;
	uint8_t sprite_y[0x08];
	uint8_t col_sprite_sprite, col_sprite_data;

	uint8_t memory_pointers;
	uint8_t control1, control2;
	uint8_t irq_status, irq_enable;

	uint8_t colors[0x0F];

	// color ram
	uint8_t color_ram[0x400];
} __attribute__((__packed__));