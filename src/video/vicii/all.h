#define FRAME_CYCLES 63
#define FRAME_SCANLINE_WIDTH (FRAME_CYCLES << 3)
#define FRAME_SCANLINES 312
#define FRAME_WAIT (1000000/50)
#define SCREEN_WIDTH 400
#define SCREEN_HEIGHT 284
#define SCREEN_START_Y 15
#define DRAW_AREA_START_X 40
#define DRAW_AREA_START_Y 42

struct VIC {
	struct EmuState H;
	int32_t raster_pos;
	int16_t raster_irq;
	uint8_t border_color, background_color;
};