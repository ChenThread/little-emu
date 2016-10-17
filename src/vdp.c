#include "common.h"

uint8_t frame_data[SCANLINES][342];

void vdp_estimate_line_irq(struct VDP *vdp, struct SMS *sms, uint64_t timestamp)
{
	if(!TIME_IN_ORDER(vdp->timestamp, sms->z80.timestamp_end)) {
		return;
	}

	// Get beginning + end
	uint64_t ts_beg = vdp->timestamp;
	uint64_t ts_end = sms->z80.timestamp_end;
	//uint64_t beg_frame = ts_beg/((unsigned long long)(684*SCANLINES));
	uint32_t beg_toffs = ts_beg%((unsigned long long)(684*SCANLINES));
	//uint64_t end_frame = ts_end/((unsigned long long)(684*SCANLINES));
	//uint32_t end_toffs = ts_end%((unsigned long long)(684*SCANLINES));

	// Get some timestamps
	uint64_t ts_beg_frame = vdp->timestamp - beg_toffs;
	uint64_t ts_beg_int = ts_beg_frame + (70)*684 + (47-17)*2;
	uint64_t ts_end_int = ts_beg_frame + (70+192+1)*684 + (47-17)*2;

	// If we are after the frame interrupt, advance
	if(!TIME_IN_ORDER(ts_beg, ts_end_int)) {
		ts_beg_frame += 684*SCANLINES;
		ts_beg_int += 684*SCANLINES;
		ts_end_int += 684*SCANLINES;
	}

	// Ensure that we can fire an interrupt at all
	if(!TIME_IN_ORDER(ts_beg_int, ts_end)) {
		return;
	}
	if(TIME_IN_ORDER(ts_end_int, ts_beg)) {
		return;
	}

	// Get counters
	//uint32_t beg_vctr = beg_toffs/684;
	//uint32_t beg_hctr = beg_toffs%684;
	//uint32_t end_vctr = end_toffs/684;
	//uint32_t end_hctr = end_toffs%684;

	// Check if it will reload
	if(TIME_IN_ORDER(ts_beg, ts_beg_int)) {
		// Register reload happens
		uint64_t ts = vdp->timestamp - beg_toffs;
		ts += 684*(70+vdp->regs[0x0A]);
		ts += 2*(47-17+1);
		if(TIME_IN_ORDER(ts_beg, ts)) {
		if(TIME_IN_ORDER(ts, sms->z80.timestamp_end)) {
			sms->z80.timestamp_end = ts;
		}
		}

	} else {
		// Register reload does not happen
		// Advance to nearest plausible point
		uint64_t ts = vdp->timestamp - beg_toffs;
		ts += 684*70;
		ts += 2*(47-17+1);
		while(!TIME_IN_ORDER(ts_beg, ts)) {
			ts += 684*70;
		}

		// Advance by line counter
		ts += 684*(uint32_t)(vdp->line_counter);

		if(TIME_IN_ORDER(ts_beg, ts)) {
		if(TIME_IN_ORDER(ts, sms->z80.timestamp_end)) {
			sms->z80.timestamp_end = ts;
		}
		}
	}
}

static void vdp_do_reg_write(struct VDP *vdp, struct SMS *sms, uint64_t timestamp)
{
	uint8_t reg = (uint8_t)((vdp->ctrl_addr>>8)&0x0F);
	uint8_t val = (uint8_t)(vdp->ctrl_addr);
	//printf("REG %X = %02X\n", reg, val);

	vdp->regs[reg] = val;

	if((vdp->regs[0x01]&0x20) == 0) {
		vdp->irq_mask &= ~1;
	} else {
		vdp->irq_mask |=  1;
	}

	if((vdp->regs[0x00]&0x10) == 0) {
		vdp->irq_mask &= ~2;
	} else {
		vdp->irq_mask |=  2;
	}
	vdp_estimate_line_irq(vdp, sms, timestamp);
}

void vdp_init(struct VDP *vdp)
{
	*vdp = (struct VDP){ .timestamp=0 };
	vdp->ctrl_addr = 0xBF00;
	vdp->ctrl_latch = 0;
	vdp->read_buf = 0x00;
	vdp->status = 0x00;
	vdp->regs[0x00] = 0x26;
	vdp->regs[0x01] = 0xE0;
	vdp->regs[0x02] = 0xFF;
	vdp->regs[0x03] = 0xFF;
	vdp->regs[0x04] = 0xFF;
	vdp->regs[0x05] = 0xFF;
	vdp->regs[0x06] = 0xFF;
	vdp->regs[0x07] = 0x00;
	vdp->regs[0x08] = 0x00;
	vdp->regs[0x09] = 0x00;
	vdp->irq_mask = 3;
	vdp->irq_out = 0;
}

void vdp_run(struct VDP *vdp, struct SMS *sms, uint64_t timestamp)
{
	timestamp &= ~1;
	if(!TIME_IN_ORDER(vdp->timestamp, timestamp)) {
		return;
	}

	uint64_t timediff = timestamp - vdp->timestamp;

	// Fetch vcounters/hcounters
	uint32_t vctr_beg = ((vdp->timestamp/(684ULL))%((unsigned long long)SCANLINES));
	uint32_t hctr_beg = (vdp->timestamp%(684ULL));
	uint32_t vctr_end = ((timestamp/(684ULL))%((unsigned long long)SCANLINES));
	uint32_t hctr_end = (timestamp%(684ULL));
	vdp->timestamp_end = timestamp;
	assert((hctr_beg&1) == 0);
	assert((hctr_end&1) == 0);
	hctr_beg >>= 1;
	hctr_end >>= 1;

	// Fetch pointers
	uint8_t *sp_tab_y = &sms->vram[((vdp->regs[0x05]<<7)&0x3F00)+0x00];
	uint8_t *sp_tab_p = &sms->vram[((vdp->regs[0x05]<<7)&0x3F00)+0x80];
	uint8_t *sp_tiles = &sms->vram[((vdp->regs[0x06]<<11)&0x2000)];
	uint8_t *bg_names = &sms->vram[((vdp->regs[0x02]<<10)&0x3800)];
	uint8_t *bg_tiles = &sms->vram[0];

	// Draw screen section
	int vctr = vctr_beg;
	int smask = ((vdp->regs[0x01]&0x02) != 0 ? 0xFE : 0xFF);
	int sdethigh = ((vdp->regs[0x01]&0x02) != 0 ? 16 : 8);
	int sdetwide = 8;
	int sshift = ((vdp->regs[0x01]&0x01) != 0 ? 1 : 0);
	int sxoffs = ((vdp->regs[0x00]&0x08) != 0 ? -8 : 0);
	sdethigh <<= sshift;
	sdetwide <<= sshift;
	int lborder = ((vdp->regs[0x00]&0x20) != 0 ? 8 : 0);
	int bcol = sms->cram[(vdp->regs[0x07]&0x0F)|0x10];
	int scx = (-vdp->scx)&0xFF;
	int scy = (vdp->scy)&0xFF;

	//vdp->regs[0x02] = 0xFF;
	//vdp->regs[0x05] = 0xFF;
	//vdp->regs[0x06] = 0xFF;

	for(;;) {
		int hbeg = (vctr == vctr_beg ? hctr_beg : 0);
		int hend = (vctr == vctr_end ? hctr_end : 342);
		int y = vctr - 70;

		// Latch H-scroll
		if(hbeg <= 47-17 && 47-17 < hend) {
			vdp->scx = vdp->regs[0x08];
			scx = (-vdp->scx)&0xFF;

			// Might as well latch V-scroll while we're at it
			// THIS IS A GUESS.
			if(vctr == 66) {
				vdp->scy = vdp->regs[0x09];
				scy = (vdp->scy)&0xFF;
			}
		}

		// Latch line counter
		if(hbeg < 47-17 && 47-17 <= hend) {
			if(y < 0 || y >= 193) {
				vdp->line_counter = vdp->regs[0x0A];
				if(vdp->line_counter == 0)
					vdp->line_counter = 0xFF;
				//printf("SLI Reload %02X\n", vdp->line_counter);
			} else {
				//printf("SLI dec %02X [%0d %0d]\n", vdp->line_counter, vctr_beg, vctr_end);
				if((vdp->line_counter--) == 0) {
					vdp->line_counter = vdp->regs[0x0A];
					if(vdp->line_counter == 0)
						vdp->line_counter = 0xFF;

					//printf("SLI Reload %02X\n", vdp->line_counter);
					{
						// Kill it here
						//z80_irq(&sms->z80, sms, 0xFF);
						vdp->irq_out |= 2;
						//vdp->status |= 0x80;
						hend = (47-17);
						timediff -= (hend-hbeg)*2;
						//printf("%016lX\n", timediff);
						vdp->timestamp_end -= timediff;
						if(TIME_IN_ORDER(sms->z80.timestamp_end,
								vdp->timestamp_end)) {
							sms->z80.timestamp_end = vdp->timestamp_end;
						}
						vctr_end = vctr;
						uint32_t vctr_chk = ((vdp->timestamp_end/(684ULL))%((unsigned long long)SCANLINES));
						uint32_t hctr_chk = (vdp->timestamp_end%(684ULL));
						//printf("hend %d %d\n", hend, hctr_chk>>1);
						//printf("vend %d %d\n", vctr_end, vctr_chk);
						fflush(stdout);
						assert(vctr_end == vctr_chk);
						assert(hend == (hctr_chk>>1));
						//printf("SLI %3d %3d %3d %02X\n"
						//	, vctr, hbeg, hend, vdp->line_counter);
					}
				}
			}
		}

		if(sms->no_draw) {
			// Do nothing

		} else if(y < 0 || y >= 192 || (vdp->regs[0x01]&0x40)==0) {
			if(y < -54 || y >= 192+48) {
				assert(vctr >= 0 && vctr < SCANLINES);
				for(int hctr = hbeg; hctr < hend; hctr++) {
					assert(hctr >= 0 && hctr < 342);
					frame_data[vctr][hctr] = 0x00;
				}
			} else {
				assert(vctr >= 0 && vctr < SCANLINES);
				for(int hctr = hbeg; hctr < hend; hctr++) {
					assert(hctr >= 0 && hctr < 342);
					frame_data[vctr][hctr] = bcol;
				}
			}
		} else {
			int py = (y+scy)%(28*8);

			// Calculate sprites
			// TODO: find actual location for this
			// TODO: delegate this to later state
			uint8_t stab[8];
			int stab_len = 0;
			for(int i = 0; i < 64; i++) {
				// End of list marker
				// XXX: only for 192-line mode!
				if(sp_tab_y[i] == 0xD0) {
					break;
				}

				uint8_t sy = (uint8_t)(y-sp_tab_y[i]);
				if(sy < sdethigh) {
					stab[stab_len++] = i;
					if(stab_len >= 8) {
						break;
					}
				}
			}

			assert(vctr >= 0 && vctr < SCANLINES);
			for(int hctr = hbeg; hctr < hend; hctr++) {
				int x = hctr - 47;
				if(x >= lborder && x < 256) {
					// TODO get colour PROPERLY
					// TODO sprites
					int px = (x+scx)&0xFF;

					// Read name table for tile
					uint8_t *np = &bg_names[2*((py>>3)*32+(px>>3))];
					uint16_t nl = (uint16_t)(np[0]);
					uint16_t nh = (uint16_t)(np[1]);
					uint16_t n = nl|(nh<<8);
					uint16_t tile = n&0x1FF;
					//tile = ((px>>3)+(py>>3)*32)&0x1FF;

					// Prepare for tile read
					uint8_t *tp = &bg_tiles[tile*4*8];
					uint8_t pal = (nh<<1)&0x10;
					uint8_t prio = (nh>>4)&0x01;
					uint8_t xflip = ((nh>>1)&1)*7;
					uint8_t yflip = ((nh>>2)&1)*7;
					int spx = (px^xflip^7)&7;
					int spy = ((py^yflip)&7)<<2;

					// Read tile
					uint8_t t0 = tp[spy+0];
					uint8_t t1 = tp[spy+1];
					uint8_t t2 = tp[spy+2];
					uint8_t t3 = tp[spy+3];

					// Planar to chunky
					uint8_t v = 0
						| (((t0>>spx)&1)<<0)
						| (((t1>>spx)&1)<<1)
						| (((t2>>spx)&1)<<2)
						| (((t3>>spx)&1)<<3)
						| 0;
					//v = spx^spy;

					// Get sprite pixels
					uint8_t s = 0;
					for(int i = 0; i < stab_len; i++) {
						int j = stab[i];
						uint16_t sx = (uint16_t)(x-sp_tab_p[j*2+0]);
						sx += sxoffs;
						if(sx < sdetwide) {
							// Read tile
							uint16_t sn = (uint16_t)sp_tab_p[j*2+1];
							sn &= smask;
							uint16_t sy = (uint16_t)(y-sp_tab_y[j]);
							sy &= 0xFF;
							uint8_t *sp = &sp_tiles[sn*4*8];
							sx >>= sshift;
							sy >>= sshift;
							sx ^= 7;
							sy <<= 2;
							uint8_t s0 = sp[sy+0];
							uint8_t s1 = sp[sy+1];
							uint8_t s2 = sp[sy+2];
							uint8_t s3 = sp[sy+3];
							s = 0
								| (((s0>>sx)&1)<<0)
								| (((s1>>sx)&1)<<1)
								| (((s2>>sx)&1)<<2)
								| (((s3>>sx)&1)<<3)
								| 0;

							if(s != 0) {
								s |= 0x10;
								break;
							}
						}
					}

					// Write
					if(s != 0 && (v == 0 || prio == 0)) {
						v = s;
					} else {
						v |= pal;
					}
					assert(hctr >= 0 && hctr < 342);
					frame_data[vctr][hctr] = sms->cram[v&0x1F];
					//frame_data[vctr][hctr] = v&0x1F;
				} else {
					assert(hctr >= 0 && hctr < 342);
					frame_data[vctr][hctr] = bcol;
				}
			}
		}

		// Next line (if it exists)
		if(vctr == vctr_end) { break; }
		vctr++;
		if(vctr == SCANLINES) { vctr = 0; }
		assert(vctr < SCANLINES);
		timediff -= (hend-hbeg)*2;
	}

	//printf("%03d.%03d -> %03d.%03d\n" , vctr_beg, hctr_beg , vctr_end, hctr_end;

	// Update timestamp
	vdp->timestamp = vdp->timestamp_end;
}

uint8_t vdp_read_ctrl(struct VDP *vdp, struct SMS *sms, uint64_t timestamp)
{
	vdp_run(vdp, sms, timestamp);
	vdp->ctrl_latch = 0;
	vdp->irq_out &= ~3;
	uint8_t ret = vdp->status;
	vdp->status = 0x00;
	return ret;
}

uint8_t vdp_read_data(struct VDP *vdp, struct SMS *sms, uint64_t timestamp)
{
	vdp_run(vdp, sms, timestamp);
	vdp->ctrl_latch = 0;
	uint8_t ret = vdp->read_buf;
	vdp->read_buf = sms->vram[vdp->ctrl_addr&0x3FFF];
	//printf("VDP READ %04X %02X -> %02X\n", vdp->ctrl_addr, ret, vdp->read_buf);
	vdp->ctrl_addr = ((vdp->ctrl_addr+1)&0x3FFF)
		| (vdp->ctrl_addr&0xC000);
	return ret;
}

void vdp_write_ctrl(struct VDP *vdp, struct SMS *sms, uint64_t timestamp, uint8_t val)
{
	vdp_run(vdp, sms, timestamp);
	if(vdp->ctrl_latch == 0) {
		vdp->ctrl_addr &= ~0x00FF;
		vdp->ctrl_addr |= ((uint16_t)val)<<0;
		//printf("VDP LOWA %04X\n", vdp->ctrl_addr);
		vdp->ctrl_latch = 1;
	} else {
		vdp->ctrl_addr &= ~0xFF00;
		vdp->ctrl_addr |= ((uint16_t)val)<<8;
		//printf("VDP CTRL %04X\n", vdp->ctrl_addr);

		switch(vdp->ctrl_addr>>14) {
			case 0: // Read
				vdp->read_buf = sms->vram[vdp->ctrl_addr&0x3FFF];
				vdp->ctrl_addr = ((vdp->ctrl_addr+1)&0x3FFF)
					| (vdp->ctrl_addr&0xC000);
				break;
			case 1: // Write
				break;
			case 2: // Register
				vdp_do_reg_write(vdp, sms, timestamp);
				break;
			case 3: // CRAM
				break;
		}

		vdp->ctrl_latch = 0;
	}
	//printf("VDP CTRL %02X\n", val);
}

void vdp_write_data(struct VDP *vdp, struct SMS *sms, uint64_t timestamp, uint8_t val)
{
	vdp_run(vdp, sms, timestamp);
	//if(vdp->ctrl_latch != 0) { printf("VDP DLWR %04X %02X\n", vdp->ctrl_addr, val); }
	vdp->ctrl_latch = 0;
	if(vdp->ctrl_addr >= 0xC000) {
		// CRAM
		sms->cram[vdp->ctrl_addr&0x001F] = val;
		//printf("CRAM %02X %02X\n", vdp->ctrl_addr&0x1F, val);
	} else {
		// VRAM
		sms->vram[vdp->ctrl_addr&0x3FFF] = val;
		//printf("VRAM %04X %02X\n", vdp->ctrl_addr&0x3FFF, val);
	}
	vdp->read_buf = val;

	vdp->ctrl_addr = ((vdp->ctrl_addr+1)&0x3FFF)
		| (vdp->ctrl_addr&0xC000);
}

