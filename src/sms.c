#include "common.h"

struct SMS sms_current;

uint8_t sms_rom[4*1024*1024];
bool sms_rom_is_banked = false;
uint64_t twait;

uint64_t time_now(void)
{
	struct timeval ts;
	gettimeofday(&ts, NULL);

	uint64_t sec = ts.tv_sec;
	uint64_t usec = ts.tv_usec;
	sec *= 1000000ULL;
	usec += sec;
	return usec;
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
	sms->memcfg = 0xAB;
	sms->iocfg = 0xFF;
	sms->hlatch = 0x80; // TODO: find out what this is on reset
	z80_init(&(sms->z80));
	vdp_init(&(sms->vdp));
	psg_init(&(sms->psg));
	//sms->z80.timestamp = 1;
	//sms->vdp.timestamp = 0;

	sms->no_draw = false;
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
	while(TIME_IN_ORDER(sms->z80.timestamp_end, timestamp)) {
		sms->z80.timestamp_end = timestamp;
		vdp_estimate_line_irq(&(sms->vdp), sms, sms->vdp.timestamp);
		//printf("%016lX %016lX %016lX %016lX %016lX\n", timestamp, sms->z80.timestamp, sms->z80.timestamp_end, sms->vdp.timestamp, sms->vdp.timestamp_end);
		z80_run(&(sms->z80), sms, sms->z80.timestamp_end);
		vdp_run(&(sms->vdp), sms, sms->z80.timestamp_end);
		psg_run(&(sms->psg), sms, sms->z80.timestamp_end);
	}

	sms->timestamp = timestamp;
}

void sms_run_frame(struct SMS *sms)
{
	const int pt_VINT1 = 684*(FRAME_START_Y+0xC1) + (94-18*2);
#if !USE_NTSC
	const int pt_VINT2 = 684*(FRAME_START_Y+0xE1) + (94-18*2);
	//const int pt_VINT3 = 684*(FRAME_START_Y+0xF1) + (94-18*2);
#endif

	// Run a frame
	if(sms->timestamp == 0) {
		sms->z80.timestamp = pt_VINT1;// - (pt_VINT1%684);
		sms_run(sms, sms->timestamp + pt_VINT1);
	}
#if USE_NTSC
	sms_run(sms, sms->timestamp + 684*SCANLINES-pt_VINT1);
#else
	sms_run(sms, sms->timestamp + pt_VINT2-pt_VINT1);
	sms_run(sms, sms->timestamp + 684*SCANLINES-pt_VINT2);
	// FIXME: V-centre the frame properly so this doesn't break
	//sms_run(sms, sms->timestamp + pt_VINT3-pt_VINT2);
	//sms_run(sms, sms->timestamp + 684*SCANLINES-pt_VINT3);
#endif
	sms_run(sms, sms->timestamp + pt_VINT1);

	//sms_copy(&sms_prev, &sms_current);
}

