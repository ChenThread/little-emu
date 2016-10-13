#include "common.h"

uint8_t frame_data[SCANLINES][342];

static void vdp_do_reg_write(struct VDP *vdp, struct SMS *sms, uint64_t timestamp)
{
	uint8_t reg = (uint8_t)((vdp->ctrl_addr>>8)&0x0F);
	uint8_t val = (uint8_t)(vdp->ctrl_addr);
	printf("REG %X = %02X\n", reg, val);
	vdp->regs[reg] = val;
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
}

void vdp_run(struct VDP *vdp, struct SMS *sms, uint64_t timestamp)
{
	if(!TIME_IN_ORDER(vdp->timestamp, timestamp)) {
		return;
	}

	timestamp &= ~1;
	// Fetch vcounters/hcounters
	uint32_t vctr_beg = ((vdp->timestamp/(684ULL))%((unsigned long long)SCANLINES));
	uint32_t hctr_beg = (vdp->timestamp%(684ULL));
	uint32_t vctr_end = ((timestamp/(684ULL))%((unsigned long long)SCANLINES));
	uint32_t hctr_end = (timestamp%(684ULL));
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
		int y = vctr - 67;

		// Latch H-scroll
		if(hbeg >= 47-17 && 47-17 <= hend) {
			vdp->scx = vdp->regs[0x08];
			scx = (-vdp->scx)&0xFF;

			// Might as well latch V-scroll while we're at it
			// THIS IS A GUESS.
			if(vctr == 66) {
				vdp->scy = vdp->regs[0x09];
				scy = (vdp->scy)&0xFF;
			}
		}

		if(y < 0 || y >= 192 || (vdp->regs[0x01]&0x40)==0) {
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
					v = (v == 0 || (s != 0 && prio == 0) ? s : v|pal);
					//v |= pal;
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
	}

	//printf("%03d.%03d -> %03d.%03d\n" , vctr_beg, hctr_beg , vctr_end, hctr_end);

	// Update timestamp
	vdp->timestamp = timestamp;
}

uint8_t vdp_read_ctrl(struct VDP *vdp, struct SMS *sms, uint64_t timestamp)
{
	vdp_run(vdp, sms, timestamp);
	vdp->ctrl_latch = 0;
	sms->z80.in_irq = 0;
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

	vdp->ctrl_addr = ((vdp->ctrl_addr+1)&0x3FFF)
		| (vdp->ctrl_addr&0xC000);
}

