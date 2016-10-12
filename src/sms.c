#include "common.h"

uint8_t sms_input_fetch(struct SMS *sms, uint64_t timestamp, int port)
{
	// TODO!
	printf("input %016llX %d\n", (unsigned long long)timestamp, port);
	return 0xFF;
}

void sms_init(struct SMS *sms)
{
	*sms = (struct SMS){ .timestamp=0 };
	sms->paging[0] = 0;
	sms->paging[1] = 0;
	sms->paging[2] = 1;
	sms->paging[3] = 2;
	z80_init(&(sms->z80));
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

	sms->timestamp = timestamp;
}

