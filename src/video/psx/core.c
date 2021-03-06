//include "video/psx/all.h"

void GPUNAME(init)(struct EmuGlobal *G, struct GPU *gpu)
{
	*gpu = (struct GPU){ .H={.timestamp=0,}, };
	gpu->cmd_count = 0;

	gpu->xfer_mode = PSX_GPU_XFER_NONE;
	gpu->xfer_dma = PSX_GPU_DMA_OFF;

	gpu->screen_x0 = 0x200;
	gpu->screen_y0 = 0x010;
	gpu->screen_x1 = 0x200+2560;
	gpu->screen_y1 = 0x010+240;
	gpu->disp_addr = 0;
	gpu->screen_div = 4;

	gpu->draw_x0 = 0;
	gpu->draw_x1 = 0;
	gpu->draw_y0 = 0;
	gpu->draw_y1 = 0;
	gpu->draw_ox = 0;
	gpu->draw_oy = 0;
}

void GPUNAME(run)(struct GPU *gpu, struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp)
{
	timestamp &= ~1;
	if(!TIME_IN_ORDER(gpu->H.timestamp, timestamp)) {
		return;
	}

	uint32_t (*frame_data)[PIXELS_WIDE] = GPU_FRAME_DATA;

	uint64_t timediff = timestamp - gpu->H.timestamp;

	// Fetch vcounters/hcounters
	const uint64_t TICKS_PER_SCANLINE = VCLKS_WIDE*7;
	const uint32_t GRAN = 7*PIXELS_STEP;
	uint32_t vctr_beg = (gpu->H.timestamp/TICKS_PER_SCANLINE);
	uint32_t hctr_beg = (gpu->H.timestamp%TICKS_PER_SCANLINE);
	uint32_t vctr_end = (timestamp/TICKS_PER_SCANLINE);
	uint32_t hctr_end = (timestamp%TICKS_PER_SCANLINE);
	uint32_t vctr_remain = vctr_end - vctr_beg;
	vctr_beg %= (uint64_t)SCANLINES;
	vctr_end %= (uint64_t)SCANLINES;

	gpu->H.timestamp_end = timestamp;
	hctr_beg /= GRAN;
	hctr_end /= GRAN;

	// Draw screen section
	int vctr = vctr_beg;

	uint32_t FRAME_START_H = gpu->screen_x0;
	uint32_t FRAME_START_V = gpu->screen_y0;
	uint32_t FRAME_END_H = gpu->screen_x1;
	uint32_t FRAME_END_V = gpu->screen_y1;
	//printf("%d %d -> %d %d\n", FRAME_START_H, FRAME_START_V, FRAME_END_H, FRAME_END_V);

	uint32_t FRAME_BORDER_TOP = 0;
	uint32_t FRAME_BORDER_BOTTOM = FRAME_END_V - FRAME_START_V;

	uint32_t PIXEL_DIV = gpu->screen_div;

	// Loop
	const uint32_t BORDER_COLOR = 0x00111111;
	for(;;) {
		int hbeg = (vctr == vctr_beg ? hctr_beg : 0);
		int hend = (vctr_remain == 0 ? hctr_end : PIXELS_WIDE);
		int y = vctr - FRAME_START_V;

		// Set vblank IRQ
		// TODO!
		/*
		if(y == vint_line && (gpu->regs[0x01]&0x20) != 0) {
			if(hbeg < VINT_OFFS && VINT_OFFS <= hend) {
				gpu->irq_out |= 1;
			}
		}
		*/
		if(y < FRAME_BORDER_TOP || y >= FRAME_BORDER_BOTTOM || false) {
			if(y < FRAME_BORDER_TOP || y >= FRAME_BORDER_BOTTOM) {
				assert(vctr >= 0 && vctr < SCANLINES);
				for(int hctr = hbeg; hctr < hend; hctr++) {
					assert(hctr >= 0 && hctr < PIXELS_WIDE);
					frame_data[vctr][hctr] = BORDER_COLOR;
				}
			} else {
				assert(vctr >= 0 && vctr < SCANLINES);
				for(int hctr = hbeg; hctr < hend; hctr++) {
					int x = hctr*PIXELS_STEP;
					assert(hctr >= 0 && hctr < PIXELS_WIDE);
					frame_data[vctr][hctr] = (
						x >= FRAME_START_H && x < FRAME_END_H
						? 0x0055AAFF
						: BORDER_COLOR);
				}
			}

		} else if(H->no_draw && false) {
			// TODO!

		} else {
			uint16_t *vram = gpu->vram.p;
			uint32_t addr = ((y<<10)+gpu->disp_addr)&(1024*512-1);
			for(int hctr = hbeg; hctr < hend; hctr++) {
				int x = hctr*PIXELS_STEP;

				if(x >= FRAME_START_H && x < FRAME_END_H) {
					int sx = (x-FRAME_START_H)/PIXEL_DIV;
					uint16_t psrc = vram[(addr+sx)&(1024*512-1)];
					frame_data[vctr][hctr] = (0
						|((psrc&0x7C00)<<9)
						|((psrc&0x03E0)<<6)
						|((psrc&0x001F)<<3));
				} else {
					assert(hctr >= 0 && hctr < PIXELS_WIDE);
					frame_data[vctr][hctr] = BORDER_COLOR;
				}
			}
		}

		// Next line (if it exists)
		if(vctr_remain == 0) { break; }
		vctr++;
		vctr_remain--;
		if(vctr == SCANLINES) { vctr = 0; }
		assert(vctr < SCANLINES);
		timediff -= (hend-hbeg)*7*PIXELS_STEP;
	}

	//printf("%03d.%03d -> %03d.%03d\n" , vctr_beg, hctr_beg , vctr_end, hctr_end;

	// Update timestamp
	gpu->H.timestamp = gpu->H.timestamp_end;
}

uint32_t GPUNAME(read_gp0)(struct GPU *gpu, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp)
{
	GPUNAME(run)(gpu, G, state, timestamp);

	switch(gpu->xfer_mode) {
		case PSX_GPU_XFER_NONE:
		case PSX_GPU_XFER_TO_GPU:
			printf("!!! GPU Read in wrong mode!\n");
			break;

		case PSX_GPU_XFER_FROM_GPU: {
			uint32_t val = gpu->vram.w[(gpu->xfer_addr & (1024*512-1))>>1];
			//printf("... GPU Read %08X: %08X\n", gpu->xfer_addr, val);
			gpu->xfer_addr += 2;
			gpu->xfer_xrem -= 2;
			if(gpu->xfer_xrem == 0) {
				gpu->xfer_addr += gpu->xfer_stride;
				gpu->xfer_xrem = gpu->xfer_width;
				gpu->xfer_yrem -= 1;
				if(gpu->xfer_yrem == 0) {
					gpu->xfer_mode = PSX_GPU_XFER_NONE;
				}
			}

			return val;
		} break;

		default:
			assert(!"UNREACHABLE");
			break;
	}

	uint32_t ret = 0xFFFFFFFF;
	printf("GP0 R -> %08X\n", ret);
	return ret;
}

uint32_t GPUNAME(read_gp1)(struct GPU *gpu, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp)
{
	GPUNAME(run)(gpu, G, state, timestamp);

	//uint32_t ret = gpu->status;
	uint32_t ret = 0;

	// TODO: work out when these flags should be set
	ret |= (1<<25);
	ret |= (1<<26);
	ret |= (1<<27);
	ret |= (1<<28);

	ret |= ((gpu->xfer_dma & 0x3)<<29);

	//printf("GP1 R -> %08X\n", ret);
	return ret;
}

#define GPU_DROP_N_COMMANDS(N) { \
	memmove(gpu->cmd_fifo, gpu->cmd_fifo+N, (16-N)*sizeof(uint32_t)); \
	gpu->cmd_count -= N; \
}

void GPUNAME(write_gp0)(struct GPU *gpu, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp, uint32_t val)
{
	GPUNAME(run)(gpu, G, state, timestamp);

	// Check transfer mode
	switch(gpu->xfer_mode) {
		case PSX_GPU_XFER_NONE:
			break;

		case PSX_GPU_XFER_FROM_GPU:
			printf("!!! GPU Write in READ mode %08X\n", val);
			gpu->xfer_addr += 2;
			gpu->xfer_xrem -= 2;
			if(gpu->xfer_xrem == 0) {
				gpu->xfer_addr += gpu->xfer_stride;
				gpu->xfer_xrem = gpu->xfer_width;
				gpu->xfer_yrem -= 1;
				if(gpu->xfer_yrem == 0) {
					gpu->xfer_mode = PSX_GPU_XFER_NONE;
				}
			}
			return;

		case PSX_GPU_XFER_TO_GPU:
			//printf("... GPU Write %08X: %08X\n", gpu->xfer_addr, val);
			gpu->vram.w[(gpu->xfer_addr & (1024*512-1))>>1] = val;
			gpu->xfer_addr += 2;
			gpu->xfer_xrem -= 2;
			if(gpu->xfer_xrem == 0) {
				gpu->xfer_addr += gpu->xfer_stride;
				gpu->xfer_xrem = gpu->xfer_width;
				gpu->xfer_yrem -= 1;
				if(gpu->xfer_yrem == 0) {
					gpu->xfer_mode = PSX_GPU_XFER_NONE;
				}
			}
			return;

		default:
			assert(!"UNREACHABLE");
			break;
	}

	// Handle overflows disgracefully
	if(gpu->cmd_count >= 16) {
		printf("!!! GPU FIFO OVERFLOW !!! - %08X\n", val);
		return;
	}

	// Add to FIFO
	gpu->cmd_fifo[gpu->cmd_count++] = val;

	uint32_t *c = gpu->cmd_fifo;

	uint32_t cmd = (c[0]>>24);
	switch(cmd) {
		//
		// RECTS
		//
		case 0x60: // Rect variable
		case 0x68: // Rect 1x1
		case 0x70: // Rect 8x8
		case 0x78: // Rect 16x16
		{
			uint32_t rw, rh;
			switch(cmd) {
				case 0x68:
				case 0x6A:
				case 0x6C:
				case 0x6E:
					rw = 1; rh = 1; break;
				case 0x70:
				case 0x72:
				case 0x74:
				case 0x76:
					rw = 8; rh = 8; break;
				case 0x78:
				case 0x7A:
				case 0x7C:
				case 0x7E:
					rw = 16; rh = 16; break;
				default:
					if(gpu->cmd_count < 3) { return; }
					rw = (int32_t)(int16_t)(c[2]);
					rh = (int32_t)(int16_t)(c[2]>>16);
					break;
			}

			if(gpu->cmd_count < 2) { return; }
			int32_t rx = (int32_t)(int16_t)(c[1]);
			int32_t ry = (int32_t)(int16_t)(c[1]>>16);
			rx += gpu->draw_ox;
			ry += gpu->draw_oy;
			uint32_t col = c[0]&0x00FFFFFF;

			// TODO: factor in window stuff
			uint32_t r = (uint32_t)(uint8_t)col;
			uint32_t g = (uint32_t)(uint8_t)(col>>8);
			uint32_t b = (uint32_t)(uint8_t)(col>>16);
			// TODO: dither
			// TODO: stencil/transp bit
			r >>= 3;
			g >>= 3;
			b >>= 3;
			uint32_t pcol = (b<<10)|(g<<5)|(r);

			for(int y = ry; y < ry+rh; y++) {
			for(int x = rx; x < rx+rw; x++) {
				if(rx >= 0 && rx < 1024 && ry >= 0 && ry < 512) {
					uint32_t addr = (y<<10)+x;
					addr &= 1024*512-1;
					gpu->vram.p[addr] = pcol;
				}
			}
			}
			//printf("GP0 - Rect 1x1 %d,%d - %06X\n", rx, ry, col);
			GPU_DROP_N_COMMANDS(2);
		} break;

		//
		// ATTRIBS
		//
		case 0xE5: { // Drawing offset
			gpu->draw_ox = (((int32_t)c[0])<<21)>>21;
			gpu->draw_oy = (((int32_t)c[0])<<10)>>21;
			printf("GP0 - Drawing offset %5d %5d\n", gpu->draw_ox, gpu->draw_oy);
			GPU_DROP_N_COMMANDS(1);
		} break;

		default:
		switch(cmd&0xE0)
		{
			//
			// TRANSFERS
			//
			case 0xA0: // Upload rect
			case 0xC0: // Download rect
			{
				if(gpu->cmd_count < 3) { return; }
				switch(cmd&0xE0) {
					case 0xA0:
						printf("GP0 - Upload rect %08X %08X\n", c[1], c[2]);
						gpu->xfer_mode = PSX_GPU_XFER_TO_GPU;
						break;
					case 0xC0:
						printf("GP0 - Download rect %08X %08X\n", c[1], c[2]);
						gpu->xfer_mode = PSX_GPU_XFER_FROM_GPU;
						break;

					default:
						assert(!"UNREACHABLE");
						break;
				}
				uint32_t mx = (uint32_t)(uint16_t)(c[1]);
				uint32_t my = (uint32_t)(uint16_t)(c[1]>>16);
				uint32_t mw = (uint32_t)(uint16_t)(c[2]);
				uint32_t mh = (uint32_t)(uint16_t)(c[2]>>16);

				// TODO: properly handle odd X/W values!
				mx &= 1;
				mw = (mw+1)&~1;
				assert((mx&1) == 0);
				assert((mw&1) == 0);

				mx &= 1024-1;
				my &= 512-1;
				mw -= 1;
				mh -= 1;
				mw &= 1024-1;
				mh &= 512-1;
				mw += 1;
				mh += 1;

				gpu->xfer_addr = mx + (my<<10);
				gpu->xfer_width = mw;
				gpu->xfer_stride = 1024-mw;
				gpu->xfer_xrem = mw;
				gpu->xfer_yrem = mh;
				GPU_DROP_N_COMMANDS(3);
			} break;

			default:
				printf("GP0 W %08X\n", val);
				GPU_DROP_N_COMMANDS(1);
				break;
		} break;
	}
}

void GPUNAME(write_gp1)(struct GPU *gpu, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp, uint32_t val)
{
	GPUNAME(run)(gpu, G, state, timestamp);
	switch(val>>24) {
		// case 0x00: // Reset GPU - TODO!

		case 0x01: // Clear FIFO
			gpu->cmd_count = 0;
			break;

		case 0x04: // DMA mode
			printf("... DMA mode %08X\n", val);
			gpu->xfer_dma = (val & 0x3);
			// TODO: don't cancel transfers
#if 0
			if(gpu->xfer_dma == PSX_GPU_DMA_OFF || gpu->xfer_dma == PSX_GPU_DMA_FIFO) {
				gpu->xfer_mode = PSX_GPU_XFER_NONE;
			}
#endif
			break;

		case 0x05: // Display source address in halfwords
			gpu->disp_addr = val&(1024*512-1);
			break;
		case 0x06: // Screen X range in vclks
			gpu->screen_x0 = ((val)&0xFFF);
			gpu->screen_x1 = ((val>>12)&0xFFF);
			//printf("Xrange: %04X -> %04X\n", gpu->screen_x0, gpu->screen_x1);
			break;
		case 0x07: // Screen Y range in scanlines
			gpu->screen_y0 = ((val)&0x3FF);
			gpu->screen_y1 = ((val>>10)&0x3FF);
			//printf("Yrange: %04X -> %04X\n", gpu->screen_y0, gpu->screen_y1);
			break;

		case 0x08: // Display mode
			switch(val&(0x43)) {
				case 0x00: gpu->screen_div = 10; break;
				case 0x01: gpu->screen_div = 8; break;
				case 0x02: gpu->screen_div = 5; break;
				case 0x03: gpu->screen_div = 4; break;
				default: gpu->screen_div = 7; break;
			}

			printf("Display Mode: %08X\n", val);
			break;

		default:
			printf("GP1 W %08X\n", val);
			break;
	}
}

