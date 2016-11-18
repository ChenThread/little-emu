//include "video/tms9918/all.h"

static const uint64_t HSC_OFFS = (47-17);
static const uint64_t LINT_OFFS = (47-17);
static const uint64_t SLATCH_OFFS = (47-15);
static const uint64_t VINT_OFFS = (47-18);

void vdp_estimate_line_irq(struct VDP *vdp, struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp)
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
	int vint_line = 0xC1;
	if((vdp->regs[0x00]&0x02) != 0) {
		if((vdp->regs[0x01]&0x18) == 0x10) {
			vint_line = 0xE1;
		} else if((vdp->regs[0x01]&0x18) == 0x08) {
			vint_line = 0xF1;
		}
	}
	uint64_t ts_beg_frame = vdp->timestamp - beg_toffs;
	uint64_t ts_beg_int = ts_beg_frame + (FRAME_START_Y)*684 + 2*LINT_OFFS;
	uint64_t ts_end_int = ts_beg_frame + (FRAME_START_Y+vint_line)*684 + 2*LINT_OFFS;

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
		ts += 684*(FRAME_START_Y+vdp->regs[0x0A]);
		ts += 2*LINT_OFFS+2;
		if(TIME_IN_ORDER(ts_beg, ts)) {
		if(TIME_IN_ORDER(ts, sms->z80.timestamp_end)) {
			sms->z80.timestamp_end = ts;
		}
		}

	} else {
		// Register reload does not happen
		// Advance to nearest plausible point
		uint64_t ts = vdp->timestamp - beg_toffs;
		ts += 684*FRAME_START_Y;
		ts += 2*LINT_OFFS+2;
		while(!TIME_IN_ORDER(ts_beg, ts)) {
			ts += 684;
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

static void vdp_do_reg_write(struct VDP *vdp, struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp)
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
	vdp_estimate_line_irq(vdp, G, sms, timestamp);
}

void vdp_init(struct SMSGlobal *G, struct VDP *vdp)
{
	*vdp = (struct VDP){ .timestamp=0 };
	vdp->ctrl_addr = 0xBF00;
	vdp->ctrl_latch = 0;
	vdp->read_buf = 0x00;
	vdp->status = 0x00;
	vdp->status_latches = 0x00;
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

void vdp_run(struct VDP *vdp, struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp)
{
	timestamp &= ~1;
	if(!TIME_IN_ORDER(vdp->timestamp, timestamp)) {
		return;
	}

	uint64_t timediff = timestamp - vdp->timestamp;

	// FIXME: get correct timing for non-192 modes
	int vint_line = 0xC1;
	if((vdp->regs[0x00]&0x02) != 0) {
		if((vdp->regs[0x01]&0x18) == 0x10) {
			vint_line = 0xE1;
		} else if((vdp->regs[0x01]&0x18) == 0x08) {
			vint_line = 0xF1;
		}
	}

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

	int scywrap = 28*8;
	if(vint_line != 0xC1) {
		bg_names = &sms->vram[((vdp->regs[0x02]<<10)&0x3000)+0x0700];
		scywrap = 32*8;
	}

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

	// Loop
	for(;;) {
		int hbeg = (vctr == vctr_beg ? hctr_beg : 0);
		int hend = (vctr == vctr_end ? hctr_end : 342);
		int y = vctr - FRAME_START_Y;

		// Latch H-scroll
		if(hbeg <= HSC_OFFS && HSC_OFFS < hend) {
			vdp->scx = vdp->regs[0x08];
			scx = (-vdp->scx)&0xFF;

			// Might as well latch V-scroll while we're at it
			// THIS IS A GUESS.
			if(vctr == 66) {
				vdp->scy = vdp->regs[0x09];
				scy = (vdp->scy)&0xFF;
			}
		}

		// Lock top H scroll if lock bit set
		if(((uint32_t)y) < 16 && (vdp->regs[0x00]&0x40)!=0) {
			scx = 0;
		}

		// Set vblank IRQ
		if(y == vint_line && (sms->vdp.regs[0x01]&0x20) != 0) {
			if(hbeg < VINT_OFFS && VINT_OFFS <= hend) {
				sms->vdp.irq_out |= 1;
			}
		}

		// Latch status flags
		if(hbeg <= SLATCH_OFFS && SLATCH_OFFS < hend) {
			if(y == vint_line && (sms->vdp.regs[0x01]&0x20) != 0) {
				vdp->status_latches |= 0x80;
			}
			vdp->status |= vdp->status_latches;
			vdp->status_latches = 0;
		}

		// Latch line counter
		if(hbeg < LINT_OFFS && LINT_OFFS <= hend) {
			if(y < 0 || y > vint_line-1) {
				vdp->line_counter = vdp->regs[0x0A];
				// HACK
				//if(vdp->line_counter == 0) { vdp->line_counter = 0xFF; }
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

		// Read sprite table
		// TODO: latch this into VDP state
		uint8_t stab[8];
		int stab_len = 0;
		for(int i = 0; i < 64; i++) {
			// End of list marker
			// XXX: only for 192-line mode!
			if(sp_tab_y[i] == 0xD0 && vint_line == 0xC1) {
				break;
			}

			uint8_t sy = (uint8_t)(y-(sp_tab_y[i]+1));
			if(sy < sdethigh) {
				if(stab_len >= 8) {
					if(y >= 0 && y < vint_line-1) {
						if(hbeg <= SLATCH_OFFS && SLATCH_OFFS < hend) {
							vdp->status |= 0x40; // OVR
						} else {
							vdp->status_latches |= 0x40; // OVR
						}
					}
					break;
				}
				stab[stab_len++] = i;
			}
		}

		// Check for collisions
		/*
		v: earliest possible collision point
		w: latest possible collision point

		Unless sx0 == sx1 these are optimistic,
		but anything outside of that range will not collide anyway.

		If (sx0-sx1) == +1:
		[If sx0 == sx1+1:]
		-------- -------- -v------ w-------
		00000000 00000000 0aaaaaaa a0000000
		00000000 00000000 bbbbbbbb 00000000
		If (sx0-sx1) == -1:
		[If sx0 == sx1-1:]
		-------- -------- -v------ w-------
		00000000 00000000 0aaaaaaa a0000000
		00000000 00000000 00bbbbbb bb000000

		v,w fixed according to sdetwide
		TODO: make this work in 2x mag mode (i.e. sdetwide != 8)

		*/
		uint32_t sw = sdetwide-1;
		int cbeg = lborder+47;
		if(cbeg > hbeg) { cbeg = hbeg; }
		if((vdp->regs[0x01]&0x40)!=0){
		for(int i = 0; i < stab_len && i < 8; i++) {
			uint16_t sx0 = (uint16_t)(sp_tab_p[stab[i]*2+0]);
			sx0 += sxoffs;
			uint16_t sn0 = smask&(uint16_t)sp_tab_p[stab[i]*2+1];
			uint16_t sy0 = 0xFF&(uint16_t)(y-sp_tab_y[stab[i]]);
			uint8_t *sp0 = &sp_tiles[(sn0*8+sy0)*4];
			uint32_t sd0 = sp0[0]|sp0[1]|sp0[2]|sp0[3];
			sd0 <<= sw;
			for(int j = i+1; j < stab_len && j < 8; j++) {
				uint16_t sx1 = (uint16_t)(sp_tab_p[stab[j]*2+0]);
				sx1 += sxoffs;
				uint16_t dsx = sx0-sx1+sw;
				if(dsx < sdetwide*2-1) {
					// Potential collision, need to read tile data
					uint16_t sn1 = smask&(uint16_t)sp_tab_p[stab[j]*2+1];
					uint16_t sy1 = 0xFF&(uint16_t)(y-sp_tab_y[stab[j]]);
					uint8_t *sp1 = &sp_tiles[(sn1*8+sy1)*4];
					uint32_t sd1 = sp1[0]|sp1[1]|sp1[2]|sp1[3];
					sd1 <<= dsx;
					uint32_t match = sd0&sd1;
					if((match) != 0) {
						// Calculate exact pixel
						int cx = sx0+47;

						if(false) {}
						else if((match&(0x80<<sw))!=0 && cbeg<cx+0){ cx += 0;}
						else if((match&(0x40<<sw))!=0 && cbeg<cx+1){ cx += 1;}
						else if((match&(0x20<<sw))!=0 && cbeg<cx+2){ cx += 2;}
						else if((match&(0x10<<sw))!=0 && cbeg<cx+3){ cx += 3;}
						else if((match&(0x08<<sw))!=0 && cbeg<cx+4){ cx += 4;}
						else if((match&(0x04<<sw))!=0 && cbeg<cx+5){ cx += 5;}
						else if((match&(0x02<<sw))!=0 && cbeg<cx+6){ cx += 6;}
						else if((match&(0x01<<sw))!=0 && cbeg<cx+7){ cx += 7;}
						else { continue; }

						// Ensure collision point is within bounds
						int cx0 = cx-47;
						if(cx0 >= 256) { continue; }

						// TODO: handle inter-timestep cases
						if(hbeg < cx && cx <= hend) {
							//printf("COL %d %d\n", colx0, colx1);
							vdp->status |= 0x20; // COL
							break;
						}
					}
				}
			}
		}
		}

		if(y < 0 || y >= vint_line-1 || (vdp->regs[0x01]&0x40)==0) {
			if(y < -FRAME_BORDER_TOP || y >= vint_line-1+FRAME_BORDER_BOTTOM) {
				assert(vctr >= 0 && vctr < SCANLINES);
				for(int hctr = hbeg; hctr < hend; hctr++) {
					assert(hctr >= 0 && hctr < 342);
					G->frame_data[vctr][hctr] = 0x00;
				}
			} else {
				assert(vctr >= 0 && vctr < SCANLINES);
				for(int hctr = hbeg; hctr < hend; hctr++) {
					assert(hctr >= 0 && hctr < 342);
					G->frame_data[vctr][hctr] = (
						hctr >= 47-13 && hctr < 47+256+15
						? bcol
						: 0x00);
				}
			}
		} else if(sms->no_draw) {
			// TODO!
		} else {
			int py = (y+scy)%scywrap;

			int scr_ykill = (((vdp->regs[0x00]&0x80)==0)
				? 0x100
				: 24*8-((-scx)&7));

			assert(vctr >= 0 && vctr < SCANLINES);
			for(int hctr = hbeg; hctr < hend; hctr++) {
				int x = hctr - 47;

				if(x >= lborder && x < 256) {
					// TODO refactor this all into something more believable
					int px = (x+scx)&0xFF;
					int eff_py = (x < scr_ykill
						? py
						: y&0xFF);

					// Read name table for tile
					uint8_t *np = &bg_names[2*((eff_py>>3)*32+(px>>3))];
					uint16_t nl = (uint16_t)(np[0]);
					uint16_t nh = (uint16_t)(np[1]);
					uint16_t n = nl|(nh<<8);
					uint16_t tile = n&0x1FF;
					//tile = ((px>>3)+(eff_py>>3)*32)&0x1FF;

					// Prepare for tile read
					uint8_t *tp = &bg_tiles[tile*4*8];
					uint8_t pal = (nh<<1)&0x10;
					uint8_t prio = (nh>>4)&0x01;
					uint8_t xflip = ((nh>>1)&1)*7;
					uint8_t yflip = ((nh>>2)&1)*7;
					int spx = (px^xflip^7)&7;
					int spy = ((eff_py^yflip)&7)<<2;

					uint8_t v = 0;

					// Ensure we aren't in the pre-read stage
					if(x >= ((-scx)&7)) { 
						// Read tile
						uint8_t t0 = tp[spy+0];
						uint8_t t1 = tp[spy+1];
						uint8_t t2 = tp[spy+2];
						uint8_t t3 = tp[spy+3];

						// Planar to chunky
						v = 0
							| (((t0>>spx)&1)<<0)
							| (((t1>>spx)&1)<<1)
							| (((t2>>spx)&1)<<2)
							| (((t3>>spx)&1)<<3)
							| 0;
						//v = spx^spy;
					}

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
							uint16_t sy = (uint16_t)(y-(uint16_t)(uint8_t)(sp_tab_y[j]+1));
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
							uint8_t ns = 0
								| (((s0>>sx)&1)<<0)
								| (((s1>>sx)&1)<<1)
								| (((s2>>sx)&1)<<2)
								| (((s3>>sx)&1)<<3)
								| 0;

							if(ns != 0) {
								s = ns|0x10;
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
					G->frame_data[vctr][hctr] = sms->cram[v&0x1F];
					//G->frame_data[vctr][hctr] = v&0x1F;
				} else {
					assert(hctr >= 0 && hctr < 342);
					G->frame_data[vctr][hctr] = (
						hctr >= 47-13 && hctr < 47+256+15
						? bcol
						: 0x00);
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

uint8_t vdp_read_ctrl(struct VDP *vdp, struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp)
{
	vdp_run(vdp, G, sms, timestamp);
	vdp->ctrl_latch = 0;
	vdp->irq_out &= ~3;
	uint8_t ret = vdp->status;
	vdp->status = 0x00;
	return ret;
}

uint8_t vdp_read_data(struct VDP *vdp, struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp)
{
	vdp_run(vdp, G, sms, timestamp);
	vdp->ctrl_latch = 0;
	uint8_t ret = vdp->read_buf;
	vdp->read_buf = sms->vram[vdp->ctrl_addr&0x3FFF];
	//printf("VDP READ %04X %02X -> %02X\n", vdp->ctrl_addr, ret, vdp->read_buf);
	vdp->ctrl_addr = ((vdp->ctrl_addr+1)&0x3FFF)
		| (vdp->ctrl_addr&0xC000);
	return ret;
}

void vdp_write_ctrl(struct VDP *vdp, struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp, uint8_t val)
{
	vdp_run(vdp, G, sms, timestamp);
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
				vdp_do_reg_write(vdp, G, sms, timestamp);
				break;
			case 3: // CRAM
				break;
		}

		vdp->ctrl_latch = 0;
	}
	//printf("VDP CTRL %02X\n", val);
}

void vdp_write_data(struct VDP *vdp, struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp, uint8_t val)
{
	vdp_run(vdp, G, sms, timestamp);
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

