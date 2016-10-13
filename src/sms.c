#include "common.h"

uint8_t sms_input_fetch(struct SMS *sms, uint64_t timestamp, int port)
{
	printf("input %016llX %d\n", (unsigned long long)timestamp, port);

	SDL_Event ev;
	while(SDL_PollEvent(&ev)) {
		switch(ev.type) {
			case SDL_KEYDOWN:
				switch(ev.key.keysym.sym)
				{
					case SDLK_w: sms->joy[0] &= ~0x01; break;
					case SDLK_s: sms->joy[0] &= ~0x02; break;
					case SDLK_a: sms->joy[0] &= ~0x04; break;
					case SDLK_d: sms->joy[0] &= ~0x08; break;
					case SDLK_KP_2: sms->joy[0] &= ~0x10; break;
					case SDLK_KP_3: sms->joy[0] &= ~0x20; break;
					default:
						break;
				} break;

			case SDL_KEYUP:
				switch(ev.key.keysym.sym)
				{
					case SDLK_w: sms->joy[0] |= 0x01; break;
					case SDLK_s: sms->joy[0] |= 0x02; break;
					case SDLK_a: sms->joy[0] |= 0x04; break;
					case SDLK_d: sms->joy[0] |= 0x08; break;
					case SDLK_KP_2: sms->joy[0] |= 0x10; break;
					case SDLK_KP_3: sms->joy[0] |= 0x20; break;
					default:
						break;
				} break;

			case SDL_QUIT:
				exit(0);
				break;
			default:
				break;
		}
	}

	//printf("OUTPUT: %02X\n", sms->joy[port&1]);
	return sms->joy[port&1];
}

void sms_init(struct SMS *sms)
{
	*sms = (struct SMS){ .timestamp=0 };
	sms->paging[3] = 0; // 0xFFFC
	sms->paging[0] = 0; // 0xFFFD
	sms->paging[1] = 1; // 0xFFFE
	sms->paging[2] = 2; // 0xFFFF
	sms->joy[0] = 0xFF;
	sms->joy[1] = 0xFF;
	z80_init(&(sms->z80));
	vdp_init(&(sms->vdp));
}

void sms_copy(struct SMS *dest, struct SMS *src)
{
	memcpy(dest, src, sizeof(struct SMS));
}

void sms_run(struct SMS *sms, uint64_t timestamp)
{
	if(!TIME_IN_ORDER(sms->timestamp, timestamp)) {
		return;
	}

	//uint64_t dt = timestamp - sms->timestamp;
	z80_run(&(sms->z80), sms, timestamp);
	vdp_run(&(sms->vdp), sms, timestamp);

	sms->timestamp = timestamp;
}

