void vic_init(struct EmuGlobal *G, struct VIC *vic)
{
	*vic = (struct VIC){ .H={.timestamp=0,}, };
}

static inline uint8_t vic_get_pixel(struct VIC *vic, struct C64Global *H, struct C64 *state, int x, int y) {
	// char mode
	uint8_t chr_ind = cpu_6502_read_mem(H, state, 0x0400 + (x >> 3) + ((y >> 3) * 40));
	uint8_t chr_pxl = H->rom_char[(chr_ind << 3 | (y & 7))];
	if ((chr_pxl << (x & 7)) & 0x80) {
		uint8_t fg_color = cpu_6502_read_mem(H, state, 0xD800 + (x >> 3) + ((y >> 3) * 40));
		return fg_color;
	}

	return vic->background_color;
}

void vic_run(struct VIC *vic, struct C64Global *H, struct C64 *state, uint64_t timestamp)
{
	if(!TIME_IN_ORDER(vic->H.timestamp, timestamp)) {
		return;
	}

	vic->H.timestamp_end = timestamp;
	while(TIME_IN_ORDER(vic->H.timestamp, vic->H.timestamp_end)) {
		int rpy = (vic->raster_pos / FRAME_SCANLINE_WIDTH) - SCREEN_START_Y;
		if (rpy >= 0 && rpy < SCREEN_HEIGHT) {
			int ipy = rpy - DRAW_AREA_START_Y;
			int rpx = vic->raster_pos % FRAME_SCANLINE_WIDTH;
			for (int i = 0; i < 8; i++) {
				if (rpx >= SCREEN_WIDTH) break;
				int ipx = rpx - DRAW_AREA_START_X;
				if (ipx >= 0 && ipy >= 0 && ipx < 320 && ipy < 200)
					H->frame_data[rpy * SCREEN_WIDTH + rpx] = vic_get_pixel(vic, H, state, ipx, ipy);
				else
					H->frame_data[rpy * SCREEN_WIDTH + rpx] = vic->border_color;
				rpx++;
			}
		}

		vic->raster_pos = (vic->raster_pos + 8) % (FRAME_SCANLINE_WIDTH * FRAME_SCANLINES);
		vic->H.timestamp++;
	}
}

uint8_t vic_read_mem(struct C64Global *H, struct C64 *Hstate, uint16_t addr) {
	struct VIC *vic = &Hstate->vic;
	switch (addr) {
		case 0xD012:
			return (vic->raster_pos / FRAME_SCANLINE_WIDTH);
		case 0xD020:
			return 0xF0 | vic->border_color;
		case 0xD021:
			return 0xF0 | vic->background_color;
		default:
			return 0;
	}
}

void vic_write_mem(struct C64Global *H, struct C64 *Hstate, uint16_t addr, uint8_t value) {
	struct VIC *vic = &Hstate->vic;
	switch (addr) {
		case 0xD012:
			vic->raster_irq = value; break;
		case 0xD020:
			vic->border_color = value & 0x0F; break;
		case 0xD021:
			vic->background_color = value & 0x0F; break;
	}
}