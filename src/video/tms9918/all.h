#if (USE_NTSC) != 0
#define SCANLINES 262
#define FRAME_START_Y 43
#define FRAME_BORDER_TOP 27
#define FRAME_BORDER_BOTTOM 24
#define FRAME_WAIT (1000000/60)
#else
#define SCANLINES 313
#define FRAME_START_Y 70
#define FRAME_BORDER_TOP 54
#define FRAME_BORDER_BOTTOM 48
#define FRAME_WAIT (1000000/50)
#endif

struct VDP
{
	// VDP state
	uint8_t sprd[8][4];
	uint8_t regs[16];
	uint8_t sprx[8];
	uint16_t ctrl_addr;
	uint8_t ctrl_latch;
	uint8_t read_buf;
	uint8_t status;
	uint8_t status_latches;

	uint8_t line_counter;
	uint8_t scx, scy;

	uint8_t irq_out;
	uint8_t irq_mask;
	
	// Tracking state
	uint64_t timestamp;
	uint64_t timestamp_end;
} __attribute__((__packed__));

