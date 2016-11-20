void vic_init(struct EmuGlobal *G, struct VIC *vic)
{
	*vic = (struct VIC){
		.H={.timestamp=0,},
	};
}

static inline uint8_t vic_read_bank_mem(struct C64Global *H, struct C64 *state, int addr) {
	uint8_t bank = (state->cia2.port_a_rw ^ 3) & 3;
	if ((bank == 0 || bank == 2) && addr >= 0x1000 && addr <= 0x1FFF)
		return H->rom_char[addr & 0x0FFF];
	return cpu_6502_read_mem(H, state, addr | (bank << 14));
}

static inline uint8_t vic_get_pixel(struct VIC *vic, struct C64Global *H, struct C64 *state, int x) {
	uint8_t color = 0xFF;
	uint8_t data_byte;

	if (x >= 0 && x < 320) {
		color = vic->row_colors[x >> 3] & 0x0F;
		data_byte = vic->row_pixels[x >> 3];
	}

	if (color != 0xFF && (data_byte << (x & 7)) & 0x80) {
		return color;
	} else {
		return vic->colors[vic->border_on ? VIC_BORDER_COLOR : VIC_BACKGROUND_COLOR];
	}
}

static inline void vic_fetch_row(struct VIC *vic, struct C64Global *H, struct C64 *state) {
	// TODO: emulate the 40 cycles stall
	int y = vic->raster_y - DRAW_AREA_START_Y;
	if (y < 0 || y >= 200) {
		for (int i = 0; i < 40; i++) {
			vic->row_colors[i] = 0xFF;
		}
	} else {
		for (int i = 0; i < 40; i++) {
			// char mode
			uint8_t chr_ind = vic_read_bank_mem(H, state, ((uint16_t) (vic->memory_pointers & 0xF0) << 6) + i + ((y >> 3) * 40));
			uint8_t data_byte = vic_read_bank_mem(H, state, ((uint16_t) (vic->memory_pointers & 0x0E) << 10) + (chr_ind << 3) + (y & 7));
			vic->row_pixels[i] = data_byte;
			vic->row_colors[i] = vic->color_ram[i + ((y >> 3) * 40)];
		}
	}
};

static void vic_irq(struct VIC *vic, struct C64Global *H, struct C64 *state, uint8_t irq) {
	vic->irq_status |= 0x80 | irq;
	cpu_6502_irq(&(state->cpu));
}

static inline bool vic_irq_ready(struct VIC *vic, uint8_t irq) {
	return !(vic->irq_status & 0x80) && (vic->irq_enable & irq);
}

void vic_run(struct VIC *vic, struct C64Global *H, struct C64 *state, uint64_t timestamp)
{
	if(!TIME_IN_ORDER(vic->H.timestamp, timestamp)) {
		return;
	}

	vic->H.timestamp_end = timestamp;
	while(TIME_IN_ORDER(vic->H.timestamp, vic->H.timestamp_end)) {
		if (vic->raster_y >= 0 && vic->raster_y < SCREEN_HEIGHT) {
			int ipy = vic->raster_y - DRAW_AREA_START_Y;
			int ipx = vic->raster_x - DRAW_AREA_START_X;
			uint8_t *frame_ptr = &(H->frame_data[vic->raster_y * SCREEN_WIDTH + vic->raster_x]);
			for (int i = 0; i < 8; i++) {
				*frame_ptr = vic_get_pixel(vic, H, state, ipx);
				ipx++;
				frame_ptr++;
			}
		}

		vic->raster_x += 8;
		if (vic->raster_x == DRAW_AREA_START_X)
			vic->border_on &= ~1;
		else if (vic->raster_x == DRAW_AREA_START_X + 320)
			vic->border_on |= 1;

		if (vic->raster_x >= FRAME_SCANLINE_WIDTH) {
			vic->raster_x = 0;
			vic->raster_y = (vic->raster_y + 1) % FRAME_SCANLINES;
			vic->control1 = (vic->control1 & 0x7F) | ((vic->raster_y >> 1) & 0x80);
			if (vic->raster_y == DRAW_AREA_START_Y)
				vic->border_on &= ~2;
			else if (vic->raster_y == DRAW_AREA_START_Y + 200)
				vic->border_on |= 2;
		
			vic_fetch_row(vic, H, state);

			if (vic_irq_ready(vic, VIC_IRQ_RASTER) && (vic->raster_irq == (vic->raster_y & 0xFF))) {
				vic_irq(vic, H, state, VIC_IRQ_RASTER);
			}
		}

		vic->H.timestamp++;
	}
}

uint8_t vic_read_mem(struct C64Global *H, struct C64 *Hstate, uint16_t addr) {
	struct VIC *vic = &Hstate->vic;
	uint8_t tmp;

	if (addr >= 0xD800 && addr < 0xDC00)
		return vic->color_ram[addr & 0x3FF];
	else {
		addr &= 0x3F;
		switch (addr) {
			case 0x00:
			case 0x02:
			case 0x04:
			case 0x06:
			case 0x08:
			case 0x0A:
			case 0x0C:
			case 0x0E:
				return vic->sprite_x[addr >> 1];
			case 0x01:
			case 0x03:
			case 0x05:
			case 0x07:
			case 0x09:
			case 0x0B:
			case 0x0D:
			case 0x0F:
				return vic->sprite_y[addr >> 1];
			case 0x10:
				return vic->sprite_x_msb;
			case 0x11:
				return vic->control1;
			case 0x12:
				return vic->raster_y;
			case 0x15:
				return vic->sprite_enable;
			case 0x16:
				return vic->control2;
			case 0x17:
				return vic->sprite_ysize;
			case 0x18:
				return vic->memory_pointers | 0x01;
			case 0x19:
				return vic->irq_status | 0x70;
			case 0x1A:
				return vic->irq_enable | 0xF0;
			case 0x1B:
				return vic->sprite_prio;
			case 0x1C:
				return vic->sprite_multicolor;
			case 0x1D:
				return vic->sprite_xsize;
			case 0x1E:
				tmp = vic->col_sprite_sprite;
				vic->col_sprite_sprite = 0;
				return tmp;
			case 0x1F:
				tmp = vic->col_sprite_data;
				vic->col_sprite_data = 0;
				return tmp;
			case 0x20:
			case 0x21:
			case 0x22:
			case 0x23:
			case 0x24:
			case 0x25:
			case 0x26:
			case 0x27:
			case 0x28:
			case 0x29:
			case 0x2A:
			case 0x2B:
			case 0x2C:
			case 0x2D:
			case 0x2E:
				return vic->colors[addr & 0x0F];
			default:
				return 0xFF;
		}
	}
}

void vic_write_mem(struct C64Global *H, struct C64 *Hstate, uint16_t addr, uint8_t value) {
	struct VIC *vic = &Hstate->vic;
	if (addr >= 0xD800 && addr < 0xDC00)
		vic->color_ram[addr & 0x3FF] = value;
	else {
		addr &= 0x3F;
		// fprintf(stderr, "vicw:%02X:%02X\n", addr, value);
		switch (addr) {
			case 0x00:
			case 0x02:
			case 0x04:
			case 0x06:
			case 0x08:
			case 0x0A:
			case 0x0C:
			case 0x0E:
				vic->sprite_x[addr >> 1] = (vic->sprite_x[addr >> 1] & 0x100) | value;
				break;
			case 0x01:
			case 0x03:
			case 0x05:
			case 0x07:
			case 0x09:
			case 0x0B:
			case 0x0D:
			case 0x0F:
				vic->sprite_y[addr >> 1] = value;
				break;
			case 0x10:
				vic->sprite_x_msb = value;
				for (int i = 0; i < 8; i++) {
					vic->sprite_x[i << 1] = (vic->sprite_x[i << 1] & 0xFF) | ((value & (1 << i)) ? 0x100 : 0x000);
				}
				break;
			case 0x11:
				vic->control1 = (vic->control1 & 0x80) | (value & 0x7F);
				break;
			case 0x12:
				vic->raster_irq = value; // TODO
				break;
			case 0x15:
				vic->sprite_enable = value;
				break;
			case 0x16:
				vic->control2 = value;
				break;
			case 0x17:
				vic->sprite_ysize = value;
				break;
			case 0x18:
				vic->memory_pointers = value;
				break;
			case 0x19:
				vic->irq_status &= (~(value) & 0x7F);
				break;
			case 0x1A:
				vic->irq_enable = value;
				break;
			case 0x1B:
				vic->sprite_prio = value;
				break;
			case 0x1C:
				vic->sprite_multicolor = value;
				break;
			case 0x1D:
				vic->sprite_xsize = value;
				break;
			case 0x20:
			case 0x21:
			case 0x22:
			case 0x23:
			case 0x24:
			case 0x25:
			case 0x26:
			case 0x27:
			case 0x28:
			case 0x29:
			case 0x2A:
			case 0x2B:
			case 0x2C:
			case 0x2D:
			case 0x2E:
				vic->colors[addr & 0x0F] = value | 0xF0;
				break;
		}
	}
}