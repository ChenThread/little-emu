//include "audio/sn76489/all.h"

static uint16_t PSGNAME(volumes)[16] = {
	16384, 13014, 10338, 8211 ,
	6523 , 5181 , 4115 , 3269 ,
	2597 , 2063 , 1638 , 1301 ,
	1034 , 821  , 652  , 0    ,
};
static uint8_t lfsr_noise_sequence[65536];
static int lfsr_noise_len = 0;

static bool is_initialised = false;
static void PSGNAME(global_init)(void)
{
	if(is_initialised) {
		return;
	}
#ifndef DEDI
	SDL_AtomicSet(&PSG_SOUND_DATA_LEN, 0);
#endif
	is_initialised = true;

	uint16_t l = 0x8000;
	lfsr_noise_len = 0;
	do {
		lfsr_noise_sequence[lfsr_noise_len] = l&1;
		lfsr_noise_len++;
		uint16_t p = l&0x0009;
		p^=(p<<8);
		p^=(p<<4);
		p^=(p<<2);
		p^=(p<<1);
		p &= 0x8000;
		l >>= 1;
		l ^= p;
	} while(l != 0x8000);
	assert(lfsr_noise_len == 57337);
	//printf("%d\n", lfsr_noise_len);
}

void PSGNAME(pop_16bit_mono)(int16_t *buf, size_t len)
{
	// TODO: proper interpolation
	if(!is_initialised) {
		memset(buf, 0, len*2);
		return;
	}

#ifndef DEDI
	// Get number of samples to read/write
	ssize_t src_len = SDL_AtomicGet(&PSG_SOUND_DATA_LEN);
	SDL_AtomicAdd(&PSG_SOUND_DATA_LEN, -(src_len & ~(PSG_OUT_BUF_LEN-1)));
	src_len &= (PSG_OUT_BUF_LEN-1);
#if USE_NTSC
	ssize_t ideal_samples_to_read = (228*3*262*60*len)/(48000*48);
#else
	ssize_t ideal_samples_to_read = (228*3*313*50*len)/(48000*48);
#endif
	ssize_t samples_to_read = ideal_samples_to_read;
	ssize_t samples_to_write = len;
	if(src_len > ideal_samples_to_read*2) {
		samples_to_read = src_len-ideal_samples_to_read*2+ideal_samples_to_read;
		//samples_to_read = ideal_samples_to_read;
	}

	if(samples_to_read > src_len) {
		samples_to_read = src_len;
	}

//	printf("%d %d %d %d\n", (int)samples_to_read, (int)src_len, (int)ideal_samples_to_read, (int)samples_to_write);

	// If there's not enough to read, just fill
	if(samples_to_read < 8) {
		memset(buf, 0, len*2);
		return;
	}

	//printf("read %d %d\n", (int)samples_to_write, (int)samples_to_read);

	// Get slope info
	size_t sample_step = samples_to_read/samples_to_write;
	size_t sample_substep = samples_to_read%samples_to_write;
	size_t sample_div = samples_to_write;

	// Fill buffer
	size_t suboffs = 0;
	for(size_t i = 0; i < len; i++) {
		// Read sample
		buf[i] = PSG_SOUND_DATA[PSG_SOUND_DATA_OUT_OFFS];

		//if(buf[i] != 0) { printf("%d\n", buf[i]); }

		// Calculate step advancement
		size_t xstep = sample_step;
		suboffs += sample_substep;
		if(suboffs >= sample_div) {
			suboffs -= sample_div;
			xstep += 1;
		}

		// Advance
		PSG_SOUND_DATA_OUT_OFFS += xstep;
		PSG_SOUND_DATA_OUT_OFFS &= PSG_OUT_BUF_LEN-1;
	}
	SDL_AtomicAdd(&PSG_SOUND_DATA_LEN, -samples_to_read);
#endif
}

static uint64_t left_timediff;

void PSGNAME(run)(struct PSG *psg, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp)
{
	if(!TIME_IN_ORDER(psg->H.timestamp, timestamp)) {
		return;
	}

	int64_t timediff = (timestamp - psg->H.timestamp) + left_timediff;
	if (timediff < 0) {
		return;
	}

#ifndef DEDI
	if(G->no_draw) {
#endif
		for(int ch = 0; ch < 4; ch++) {
			if(psg->period[ch] <= 1) {
				continue;
			}
			for(uint64_t i = 0; i < timediff; i++) {
				if(psg->poffs[ch] == 0) {
					psg->poffs[ch] = psg->period[ch];
					psg->poffs[ch] *= 16*3;
					if(ch == 3) {
						psg->poffs[ch] <<= 1;
						psg->lfsr_offs += 1;
						if((psg->lnoise&4)==0) {
							// Periodic
							psg->lfsr_offs %= 15+1;
							if(psg->lfsr_offs == 15) {
								psg->onstate[ch] = 1;
							} else {
								psg->onstate[ch] = 0;
							}

						} else {
							// White noise
							psg->lfsr_offs %= lfsr_noise_len;
							psg->onstate[ch] = lfsr_noise_sequence[
								psg->lfsr_offs]&1;
						}
						psg->onstate[ch] *= 0xFFFF;
					} else {
						psg->onstate[ch] ^= 0xFFFF;
					}
				}
				if(psg->poffs[ch] != 0) {
					psg->poffs[ch] -= 1;
				}
			}
		}

		psg->H.timestamp = timestamp;
		return;
#ifndef DEDI
	}

	uint64_t i;
	uint32_t j = 0;
	for(i = 0; i < timediff; i+=(16*3)) {
		int32_t outval = 0;
		for(int ch = 0; ch < 4; ch++) {
			if(psg->period[ch] <= 1) {
				outval += (int32_t)(uint32_t)psg->vol[ch];
				continue;
			}
			if(psg->poffs[ch] == 0) {
				psg->poffs[ch] = psg->period[ch];
				if(ch == 3) {
					psg->poffs[ch] <<= 1;
					psg->lfsr_offs += 1;
					if((psg->lnoise&4)==0) {
						// Periodic
						psg->lfsr_offs %= 15+1;
						if(psg->lfsr_offs == 15) {
							psg->onstate[ch] = 1;
						} else {
							psg->onstate[ch] = 0;
						}

					} else {
						// White noise
						psg->lfsr_offs %= lfsr_noise_len;
						psg->onstate[ch] = lfsr_noise_sequence[
							psg->lfsr_offs]&1;
					}
					psg->onstate[ch] *= 0xFFFF;
				} else {
					psg->onstate[ch] ^= 0xFFFF;
				}
			}
			if(psg->poffs[ch] != 0) {
				psg->poffs[ch] -= 1;
			}
			outval += (int32_t)(uint32_t)(psg->vol[ch] & psg->onstate[ch]);
		}

		// Apply HPF
		//
		// We don't know what the HPF strength is yet,
		// but it doesn't really matter right now,
		// as long as we have one that works.
		outval <<= 8;
		PSG_OUTHPF_CHARGE += (outval - PSG_OUTHPF_CHARGE)>>14;
		outval -= PSG_OUTHPF_CHARGE;
		outval += (1<<(9-1));
		//outval >>= 9;
		outval >>= 11;

		// Saturate
		if(outval > 0x7FFF) { 
			outval = 0x7FFF;
		}
		if(outval < -0x8000) { 
			outval = -0x8000;
		}

		// Output and advance
		PSG_SOUND_DATA[PSG_SOUND_DATA_OFFS++] = outval;
		PSG_SOUND_DATA_OFFS &= (PSG_OUT_BUF_LEN-1);
		j++;
	}
	SDL_LockAudio();
	SDL_AtomicAdd(&PSG_SOUND_DATA_LEN, j);
	SDL_UnlockAudio();
	//assert(SDL_AtomicGet(&PSG_SOUND_DATA_LEN) < PSG_OUT_BUF_LEN);
	left_timediff = timediff - i;

	psg->H.timestamp = timestamp;
#endif
}

void PSGNAME(init)(struct EmuGlobal *G, struct PSG *psg)
{
	PSGNAME(global_init)();
	*psg = (struct PSG){ .H = {.timestamp=0,}, };
	psg->vol[0] = 0xF;
	psg->vol[1] = 0xF;
	psg->vol[2] = 0xF;
	psg->vol[3] = 0xF;

	// Early "real hardware" tests suggest this is the initial state.
	psg->lnoise = 0x0;
	psg->period[3] = 0x10;
}

void PSGNAME(write)(struct PSG *psg, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp, uint8_t val)
{
	PSGNAME(run)(psg, G, state, timestamp);

	//printf("PSG %02X\n", val);
	if((val&0x80) != 0) {
		// Command
		psg->lcmd = val;
		switch((val>>4)&7) {
			// Periods
			case 0x0:
				psg->period[0] &= 0x3F0;
				psg->period[0] |= ((uint16_t)(val&0x0F));
				//psg->poffs[0] = psg->period[0]*16*3;
				break;
			case 0x2:
				psg->period[1] &= 0x3F0;
				psg->period[1] |= ((uint16_t)(val&0x0F));
				//psg->poffs[1] = psg->period[1]*16*3;
				break;
			case 0x4:
				psg->period[2] &= 0x3F0;
				psg->period[2] |= ((uint16_t)(val&0x0F));
				//psg->poffs[2] = psg->period[2]*16*3;
				if((psg->lnoise&3) == 3) {
					psg->period[3] = psg->period[2];
					//psg->poffs[3] = psg->period[3]*16*3;
				}
				break;

			// Volumes
			case 0x1:
				psg->vol[0] = PSGNAME(volumes)[val&0xF];
				break;
			case 0x3:
				psg->vol[1] = PSGNAME(volumes)[val&0xF];
				break;
			case 0x5:
				psg->vol[2] = PSGNAME(volumes)[val&0xF];
				break;
			case 0x7:
				psg->vol[3] = PSGNAME(volumes)[val&0xF];
				break;

			// LFSR noise pattern stuff
			case 0x6:
				psg->lnoise = val&7;
				psg->lfsr_offs = 0;
				switch(val&3) {
					case 0x0:
						psg->period[3] = 0x10;
						break;
					case 0x1:
						psg->period[3] = 0x20;
						break;
					case 0x2:
						psg->period[3] = 0x40;
						break;
					case 0x3:
						psg->period[3] = psg->period[2];
						break;
				}
				psg->poffs[3] = psg->period[3]*16*3;
				break;
		}

	} else {
		// Data
		switch((psg->lcmd>>4)&7) {
			// Periods
			case 0x0:
				psg->period[0] &= 0x00F;
				psg->period[0] |= ((uint16_t)(val&0x3F))<<4;
				//psg->poffs[0] = psg->period[0]*16*3;
				break;
			case 0x2:
				psg->period[1] &= 0x00F;
				psg->period[1] |= ((uint16_t)(val&0x3F))<<4;
				//psg->poffs[1] = psg->period[1]*16*3;
				break;
			case 0x4:
				psg->period[2] &= 0x00F;
				psg->period[2] |= ((uint16_t)(val&0x3F))<<4;
				//psg->poffs[2] = psg->period[2]*16*3;
				if((psg->lnoise&3) == 3) {
					psg->period[3] = psg->period[2];
					//psg->poffs[3] = psg->period[3]*16*3;
				}
				break;

			default:
				PSGNAME(write)(psg, G, state, timestamp, (val&0xF)|(psg->lcmd&0xF0));
				break;
		}

	}

}

