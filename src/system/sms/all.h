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

#ifndef USE_NTSC
#define USE_NTSC 0
#endif

#define PSG_OUT_BUF_LEN (1<<20)

#include "littleemu.h"
#include "video/tms9918/all.h"
#include "audio/sn76489/all.h"
#include "cpu/z80/all.h"

struct SMS
{
	struct EmuState H;
	uint8_t ram[8192];
	struct Z80 z80;
	struct VDP vdp;
	struct PSG psg;
	uint8_t paging[4];
	uint8_t joy[2];
	uint8_t memcfg;
	uint8_t iocfg;
	uint8_t hlatch;
} __attribute__((__packed__));

struct SMSGlobal
{
	struct EmuGlobal H;
	struct SMS current;

	struct EmuRamHead ram_heads[3];
	struct EmuRomHead rom_heads[2];

	// SMS
	uint8_t rom[4*1024*1024];
	size_t rom_len;
	bool rom_is_banked;

	// VDP
	uint8_t frame_data[SCANLINES][342];

	// PSG
	int32_t outhpf_charge;
};

// sms.c
void sms_init(struct SMSGlobal *G, struct SMS *sms);
void sms_copy(struct SMS *dest, struct SMS *src);
void sms_run(struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp);
//void sms_run_frame(struct SMSGlobal *G, struct SMS *sms);
extern void (*sms_hook_poll_input)(struct SMSGlobal *G, struct SMS *sms, int controller, uint64_t timestamp);

// psg.c
void sms_psg_pop_16bit_mono(int16_t *buf, size_t len);
void sms_psg_run(struct PSG *psg, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp);
void sms_psg_init(struct EmuGlobal *G, struct PSG *psg);
void sms_psg_write(struct PSG *psg, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp, uint8_t val);

// vdp.c
uint8_t sms_vdp_read_ctrl(struct VDP *vdp, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp);
uint8_t sms_vdp_read_data(struct VDP *vdp, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp);
void sms_vdp_write_ctrl(struct VDP *vdp, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp, uint8_t val);
void sms_vdp_write_data(struct VDP *vdp, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp, uint8_t val);
void sms_vdp_run(struct VDP *vdp, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp);
void sms_vdp_init(struct EmuGlobal *G, struct VDP *vdp);
void sms_vdp_estimate_line_irq(struct VDP *vdp, struct EmuGlobal *G, struct EmuState *state, uint64_t timestamp);

// z80.c
void sms_z80_reset(struct Z80 *z80);
void sms_z80_init(struct EmuGlobal *H, struct Z80 *z80);
void sms_z80_irq(struct Z80 *z80, struct EmuGlobal *H, struct EmuState *state, uint8_t dat);
void sms_z80_nmi(struct Z80 *z80, struct EmuGlobal *H, struct EmuState *state);
void sms_z80_run(struct Z80 *z80, struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp);

