//include "video/psx/all.h"

void GPUNAME(init)(struct EmuGlobal *G, struct GPU *gpu)
{
	*gpu = (struct GPU){ .H={.timestamp=0,}, };
	gpu->cmd_count = 0;
	gpu->screen_x0 = 0x200;
	gpu->screen_y0 = 0x010;
	gpu->screen_x1 = 0x200+2560;
	gpu->screen_y1 = 0x010+240;
	gpu->disp_x = 0;
	gpu->disp_y = 0;
	gpu->screen_div = 4;
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
			for(int hctr = hbeg; hctr < hend; hctr++) {
				int x = hctr*PIXELS_STEP;

				if(x >= FRAME_START_H && x < FRAME_END_H) {
					int sx = (x-FRAME_START_H)/PIXEL_DIV;
					frame_data[vctr][hctr] = (sx&1)*0xFF0000;
				} else {
					assert(hctr >= 0 && hctr < PIXELS_WIDE);
					frame_data[vctr][hctr] = (
						x >= FRAME_START_H && x < FRAME_END_H
						? 0x0055AAFF
						: BORDER_COLOR);
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
	uint32_t ret = 0;
	printf("GP0 R -> %08X\n", ret);
	return ret;
}

uint32_t GPUNAME(read_gp1)(struct GPU *gpu, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp)
{
	GPUNAME(run)(gpu, G, state, timestamp);
	uint32_t ret = gpu->status;
	printf("GP1 R -> %08X\n", ret);
	return ret;
}

void GPUNAME(write_gp0)(struct GPU *gpu, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp, uint32_t val)
{
	GPUNAME(run)(gpu, G, state, timestamp);
	printf("GP0 W %08X\n", val);
}

void GPUNAME(write_gp1)(struct GPU *gpu, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp, uint32_t val)
{
	GPUNAME(run)(gpu, G, state, timestamp);
	printf("GP1 W %08X\n", val);
}

