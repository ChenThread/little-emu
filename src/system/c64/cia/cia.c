#ifndef CIA_IRQ_ALARM
	#define CIA_IRQ_TIMER_A (1 << 0)
	#define CIA_IRQ_TIMER_B (1 << 1)
	#define CIA_IRQ_ALARM (1 << 2)
	#define CIA_IRQ_SERIAL (1 << 3)
	#define CIA_TIMER_A_RUNNING(cia) ((cia)->timer_a_ctrl & 1)
	#define CIA_TIMER_B_RUNNING(cia) ((cia)->timer_b_ctrl & 1)
#endif

static inline uint8_t CIA_NAME(inc_bcd)(uint8_t in) {
	in++;
	if ((in & 0xF) >= 0xA) {
		in += 0x6;
		if ((in & 0xF0) >= 0xA0) {
			in += 0x60;
		}
	}
	return in;
}

void CIA_NAME(init)(struct EmuGlobal *G, struct CIA *cia)
{
	*cia = (struct CIA){
		.H={.timestamp=0,},
		.port_a_rw = 0xFF,
		.port_b_rw = 0xFF,
		.port_a_r = 0xFF,
		.port_b_r = 0xFF,
		.direction_a = 0xFF,
		.direction_b = 0xFF,
	};
}

uint8_t CIA_NAME(read_mem)(struct CIA *cia, struct C64Global *H, struct C64 *state, uint16_t addr) {
	uint8_t tmp;
	switch (addr & 0x0F) {
		case 0x00:
			return CIA_NAME(read_port_a)(cia, H, state, cia->direction_a);
		case 0x01:
			return CIA_NAME(read_port_b)(cia, H, state, cia->direction_b);
		case 0x02:
			return cia->direction_a;
		case 0x03:
			return cia->direction_b;
		case 0x04:
			return cia->timer_a & 0xFF;
		case 0x05:
			return cia->timer_a >> 8;
		case 0x06:
			return cia->timer_b & 0xFF;
		case 0x07:
			return cia->timer_b >> 8;
		case 0x08:
			return cia->rtc_dsec;
		case 0x09:
			return cia->rtc_sec;
		case 0x0A:
			return cia->rtc_min;
		case 0x0B:
			return cia->rtc_hour;
		case 0x0D:
			tmp = cia->irq_status;
			if (tmp) {
				tmp |= 0x80;
				cia->irq_status = 0;
			}
			return tmp;
		case 0x0E:
			return cia->timer_a_ctrl;
		case 0x0F:
			return cia->timer_b_ctrl;
		default:
			return 0xFF;
	}
}

void CIA_NAME(write_mem)(struct CIA *cia, struct C64Global *H, struct C64 *state, uint16_t addr, uint8_t value) {
	switch (addr & 0x0F) {
		case 0x00:
			CIA_NAME(write_port_a)(cia, H, state, cia->direction_a, value);
			break;
		case 0x01:
			CIA_NAME(write_port_b)(cia, H, state, cia->direction_b, value);
			break;
		case 0x02:
			cia->direction_a = value;
			break;
		case 0x03:
			cia->direction_b = value;
			break;
		case 0x04:
			cia->timer_a_latch = (cia->timer_a_latch & 0xFF00) | value;
			break;
		case 0x05:
			cia->timer_a_latch = (cia->timer_a_latch & 0xFF) | (value << 8);
			if (!CIA_TIMER_A_RUNNING(cia))
				cia->timer_a = cia->timer_a_latch;
			break;
		case 0x06:
			cia->timer_b_latch = (cia->timer_b_latch & 0xFF00) | value;
			break;
		case 0x07:
			cia->timer_b_latch = (cia->timer_b_latch & 0xFF) | (value << 8);
			if (!CIA_TIMER_B_RUNNING(cia))
				cia->timer_b = cia->timer_b_latch;
			break;
		case 0x08:
			if (cia->timer_b_ctrl & 0x80)
				cia->rtc_alarm_dsec = value & 0x0F;
			else
				cia->rtc_dsec = value & 0x0F;
			break;
		case 0x09:
			if (cia->timer_b_ctrl & 0x80)
				cia->rtc_alarm_sec = value & 0x7F;
			else
				cia->rtc_sec = value & 0x7F;
			break;
		case 0x0A:
			if (cia->timer_b_ctrl & 0x80)
				cia->rtc_alarm_min = value & 0x7F;
			else
				cia->rtc_min = value & 0x7F;
			break;
		case 0x0B:
			if (cia->timer_b_ctrl & 0x80)
				cia->rtc_alarm_hour = value;
			else
				cia->rtc_hour = value;
			break;
		case 0x0C:
			// TODO
			break;
		case 0x0D:
			if (value & 0x80)
				cia->irq_enable |= (value & 0x1F);
			else
				cia->irq_enable &= ~(value & 0x1F);
			break;
		case 0x0E:
			cia->timer_a_ctrl = value;
			if (value & 0x10) // force load - TODO: verify
				cia->timer_a = cia->timer_a_latch;
			break;
		case 0x0F:
			cia->timer_b_ctrl = value;
			if (value & 0x10) // force load - TODO: verify
				cia->timer_b = cia->timer_b_latch;
			break;
	}
}

static inline void CIA_NAME(try_interrupt)(struct CIA *cia, struct C64Global *H, struct C64 *state, uint8_t val) {
	if ((cia->irq_enable & val) && !cia->irq_status) { // TODO: any status or just the one?
		cia->irq_status |= val;
		CIA_NAME(interrupt)(cia, H, state);
	}
}

static inline void CIA_NAME(tick_b)(struct CIA *cia, struct C64Global *H, struct C64 *state) {
	if (cia->timer_b_ctrl & 0x20) {
		// CNT signals
	} else {
		// system clock
		if (cia->timer_b == 0) {
			CIA_NAME(try_interrupt)(cia, H, state, CIA_IRQ_TIMER_B);

			if (!(cia->timer_b_ctrl & 0x08))
				cia->timer_b = cia->timer_b_latch;
			else
				cia->timer_b_ctrl &= 0xFE;
		} else cia->timer_b--;
	}
}

static inline void CIA_NAME(tick_a)(struct CIA *cia, struct C64Global *H, struct C64 *state) {
	if (cia->timer_a_ctrl & 0x20) {
		// CNT signals
	} else {
		// system clock
		if (cia->timer_a == 0) {
			CIA_NAME(try_interrupt)(cia, H, state, CIA_IRQ_TIMER_A);

			if (CIA_TIMER_B_RUNNING(cia) && (cia->timer_b_ctrl & 0x40)) {
				CIA_NAME(tick_b)(cia, H, state);
			}

			if (!(cia->timer_a_ctrl & 0x08))
				cia->timer_a = cia->timer_a_latch;
			else
				cia->timer_a_ctrl &= 0xFE;
		} else cia->timer_a--;
	}
}

void CIA_NAME(run)(struct CIA *cia, struct C64Global *H, struct C64 *state, uint64_t timestamp) {
	if(!TIME_IN_ORDER(cia->H.timestamp, timestamp)) {
		return;
	}

	cia->H.timestamp_end = timestamp;
	while(TIME_IN_ORDER(cia->H.timestamp, cia->H.timestamp_end)) {
		// Timer A - tick
		if (CIA_TIMER_A_RUNNING(cia)) {
			CIA_NAME(tick_a)(cia, H, state);
		}

		// Timer B - tick
		if (CIA_TIMER_B_RUNNING(cia) && !(cia->timer_b_ctrl & 0x40)) {
			CIA_NAME(tick_b)(cia, H, state);
		}

		// RTC - check alarm
		if (cia->rtc_dsec == cia->rtc_alarm_dsec
			&& cia->rtc_sec == cia->rtc_alarm_sec
			&& cia->rtc_min == cia->rtc_alarm_min
			&& cia->rtc_hour == cia->rtc_alarm_hour)
		{
			CIA_NAME(try_interrupt)(cia, H, state, CIA_IRQ_ALARM);
		}

		// RTC - tick
		if ((cia->H.timestamp % CIA_TIME_DSEC) == 0) {
			cia->rtc_dsec++;
			if (cia->rtc_dsec >= 10) {
				cia->rtc_dsec = 0;
				cia->rtc_sec = CIA_NAME(inc_bcd)(cia->rtc_sec);
				if (cia->rtc_sec >= 0x60) {
					cia->rtc_sec = 0;
					cia->rtc_min = CIA_NAME(inc_bcd)(cia->rtc_min);
					if (cia->rtc_min >= 0x60) {
						cia->rtc_min = 0;
						cia->rtc_hour = (cia->rtc_hour & 0x80) | CIA_NAME(inc_bcd)(cia->rtc_hour & 0x7F);
						if ((cia->rtc_hour & 0x7F) >= 0x12) {
							cia->rtc_hour = (cia->rtc_hour & 0x80) ^ 0x80;
						}
					}
				}
			}
		}

		cia->H.timestamp++;
	}
}