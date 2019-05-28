#include "system/psx/all.h"

void psx_timer_predict_irq(struct EmuGlobal *H, struct EmuState *state, int idx)
{
	assert(idx >= 0 && idx < PSX_TIMER_COUNT);
	struct PSX *psx = (struct PSX *)state;
	struct PSXTimer *timer = &psx->timer[idx];

	if((timer->mode & 0x0030) == 0) {
		//printf("noirq %d\n", idx);
		return; // No IRQ set
	}

	const uint64_t timestep = 11;

	// Work out steps to each marker
	uint32_t steps_targ = (timer->target&0xFFFF) - (timer->counter&0xFFFF);
	uint32_t steps_FFFF = (0xFFFF) - (timer->counter&0xFFFF);

	if((timer->mode & 0x0010) == 0x0010) {
		uint64_t ts = timer->H.timestamp + timestep*steps_targ;
		if(TIME_IN_ORDER(ts, psx->mips.H.timestamp_end)) {
			//printf("steps add %04X target %04X\n", steps_targ, timer->target);
			psx->mips.H.timestamp_end = ts;
		}
	}

	if((timer->mode & 0x0020) == 0x0020) {
		//if(timer->target == 0xFFFF || (timer->mode & 0x0028) == 0x0020) {
		{
			uint64_t ts = timer->H.timestamp + timestep*steps_FFFF;
			if(TIME_IN_ORDER(ts, psx->mips.H.timestamp_end)) {
				//printf("steps add %04X 0xFFFF\n", steps_FFFF);
				psx->mips.H.timestamp_end = ts;
			}
			//timer->H.timestamp +
		}
	}

	//psx->mips.H.timestamp_end
}

void psx_timer_run(struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp, int idx)
{
	assert(idx >= 0 && idx < PSX_TIMER_COUNT);
	struct PSX *psx = (struct PSX *)state;
	struct PSXTimer *timer = &psx->timer[idx];

	if(!TIME_IN_ORDER(timer->H.timestamp, timestamp)) {
		return;
	}

	// TODO: a lot of things.
	// TODO: reset on target mode

	timer->H.timestamp_end = timestamp;
	uint64_t timedelta = (timer->H.timestamp_end - timer->H.timestamp);
	const uint64_t timestep = 11;
	timedelta /= timestep;

	// Work out steps to each marker
	uint32_t steps_targ = (timer->target&0xFFFF) - (timer->counter&0xFFFF);
	uint32_t steps_FFFF = (0xFFFF) - (timer->counter&0xFFFF);

	if(steps_targ >= timedelta) {
		timer->mode |= 0x0800;
	}
	if(steps_FFFF >= timedelta) {
		timer->mode |= 0x1000;
	}

	timer->mode |= 0x0400; // TODO get the right behaviour for this bit

	timer->counter += timedelta;
	timer->counter &= 0xFFFF;

	//printf("Timer %d gap %016llX\n", idx, (unsigned long long)timedelta);

	timer->H.timestamp += timedelta*timestep;
}

void psx_timers_write(struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp, uint32_t addr, uint32_t val)
{
	struct PSX *psx = (struct PSX *)state;

	int idx = ((addr>>4)&0xF);
	assert(idx >= 0 && idx < PSX_TIMER_COUNT);
	switch(addr&0xC)
	{
		case 0x0: // Counter
			psx_timer_run(H, state, timestamp, idx);
			psx->timer[idx].counter = val & 0xFFFF;
			psx_timer_predict_irq(H, state, idx);
			break;
		case 0x4: // Mode
			psx_timer_run(H, state, timestamp, idx);
			// TODO: handle this properly
			psx->timer[idx].mode = val & 0xFFFF;
			psx_timer_predict_irq(H, state, idx);
			break;
		case 0x8: // Target
			psx_timer_run(H, state, timestamp, idx);
			psx->timer[idx].target = val & 0xFFFF;
			psx_timer_predict_irq(H, state, idx);
			break;
		case 0xC:
		default:
			// Unknown!
			break;
	}
}

uint32_t psx_timers_read(struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp, uint32_t addr)
{
	struct PSX *psx = (struct PSX *)state;

	int idx = ((addr>>4)&0xF);
	assert(idx >= 0 && idx < PSX_TIMER_COUNT);
	switch(addr&0xC)
	{
		case 0x0: // Counter
			psx_timer_run(H, state, timestamp, idx);
			psx_timer_predict_irq(H, state, idx);
			return psx->timer[idx].counter;
		case 0x4: {
			// Mode
			psx_timer_run(H, state, timestamp, idx);
			uint32_t mode = psx->timer[idx].mode & 0xFFFF;
			psx->timer[idx].mode &= ~0x1800; // clear bits
			psx_timer_predict_irq(H, state, idx);
			//printf("MODE %d %04X\n", idx, mode);
			return mode;
		}
		case 0x8: // Target
			psx_timer_run(H, state, timestamp, idx);
			psx_timer_predict_irq(H, state, idx);
			return psx->timer[idx].target;
		case 0xC:
		default:
			// Unknown!
			return 0;
	}
}

