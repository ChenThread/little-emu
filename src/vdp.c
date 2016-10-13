#include "common.h"

void vdp_init(struct VDP *vdp)
{
	*vdp = (struct VDP){ .timestamp=0 };
}

void vdp_run(struct VDP *vdp, struct SMS *sms, uint64_t timestamp)
{
	if(!TIME_IN_ORDER(vdp->timestamp, timestamp)) {
		return;
	}

	// Fetch vcounter/hcounter
	uint32_t vctr = ((timestamp/(684ULL))%((unsigned long long)SCANLINES));
	uint32_t hctr = (timestamp%(684ULL));
	//printf("%d %d\n", vctr, hctr);

	vdp->timestamp = timestamp;
}

