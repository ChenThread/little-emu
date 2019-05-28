#include "system/psx/all.h"

void psx_joy_update(struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp, uint32_t val)
{
	struct PSX *psx = (struct PSX *)state;
	val &= 0xFF;

	psx->joy[0].val <<= 8;
	psx->joy[0].val &= ~0xFF;
	uint32_t rval = 0xFF;
	switch(psx->joy[0].mode) {
		case PSX_JOY_MODE_UNENGAGED:
			break;
		case PSX_JOY_MODE_WAITING:
			if(val == 0x01) {
				psx->joy[0].mode = PSX_JOY_MODE_PAD_CMD;
			} else {
				psx->joy[0].mode = PSX_JOY_MODE_UNENGAGED;
			}
			break;
		case PSX_JOY_MODE_PAD_CMD:
			rval = 0x41;
			psx->joy[0].mode = PSX_JOY_MODE_BUTTON_0;
			break;
		case PSX_JOY_MODE_BUTTON_0:
			rval = 0x5A;
			psx->joy[0].mode = PSX_JOY_MODE_BUTTON_1;
			break;
		case PSX_JOY_MODE_BUTTON_1:
			rval = (uint8_t)(psx->joy[0].buttons);
			psx->joy[0].mode = PSX_JOY_MODE_BUTTON_2;
			break;
		case PSX_JOY_MODE_BUTTON_2:
			rval = (uint8_t)(psx->joy[0].buttons>>8);
			psx->joy[0].mode = PSX_JOY_MODE_UNENGAGED;
			break;
	}

	psx->joy[0].val |= (rval & 0xFF);
}
