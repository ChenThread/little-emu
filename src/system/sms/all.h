#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

#ifdef DEDI
#include <dlfcn.h>
#else
#include <SDL.h>
#endif

#ifndef USE_NTSC
#define USE_NTSC 0
#endif

#include "littleemu.h"
#include "video/tms9918/all.h"
#include "audio/sn76489/all.h"
#include "cpu/z80/all.h"

struct SMS
{
	uint8_t vram[16384];
	uint8_t ram[8192];
	uint8_t cram[32];
	struct Z80 z80;
	struct VDP vdp;
	struct PSG psg;
	uint8_t paging[4];
	uint8_t joy[2];
	uint8_t memcfg;
	uint8_t iocfg;
	uint8_t hlatch;

	bool no_draw;
	uint64_t timestamp;
	uint64_t timestamp_end;
} __attribute__((__packed__));

struct SMSGlobal
{
	struct SMS current;

	// SMS
	uint8_t rom[4*1024*1024];
	uint64_t twait;
	bool rom_is_banked;

	// VDP
	uint8_t frame_data[SCANLINES][342];

	// PSG
	int32_t outhpf_charge;
} __attribute__((__packed__));

// sms.c
extern struct SMSGlobal Gsms;
uint64_t time_now(void);
void sms_init(struct SMSGlobal *G, struct SMS *sms);
void sms_copy(struct SMS *dest, struct SMS *src);
void sms_run(struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp);
void sms_run_frame(struct SMSGlobal *G, struct SMS *sms);
extern void (*sms_hook_poll_input)(struct SMSGlobal *G, struct SMS *sms, int controller, uint64_t timestamp);

// psg.c
void psg_pop_16bit_mono(int16_t *buf, size_t len);
void psg_run(struct PSG *psg, struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp);
void psg_init(struct SMSGlobal *G, struct PSG *psg);
void psg_write(struct PSG *psg, struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp, uint8_t val);

// vdp.c
uint8_t vdp_read_ctrl(struct VDP *vdp, struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp);
uint8_t vdp_read_data(struct VDP *vdp, struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp);
void vdp_write_ctrl(struct VDP *vdp, struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp, uint8_t val);
void vdp_write_data(struct VDP *vdp, struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp, uint8_t val);
void vdp_run(struct VDP *vdp, struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp);
void vdp_init(struct SMSGlobal *G, struct VDP *vdp);
void vdp_estimate_line_irq(struct VDP *vdp, struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp);

// z80.c
void sms_z80_reset(struct Z80 *z80);
void sms_z80_init(struct SMSGlobal *G, struct Z80 *z80);
void sms_z80_irq(struct Z80 *z80, struct SMSGlobal *G, struct SMS *sms, uint8_t dat);
void sms_z80_nmi(struct Z80 *z80, struct SMSGlobal *G, struct SMS *sms);
void sms_z80_run(struct Z80 *z80, struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp);

