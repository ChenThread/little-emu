#include "system/sms/all.h"

#define PSGNAME(n) sms_psg_##n

#define PSG_OUTHPF_CHARGE (((struct SMSGlobal *)G)->outhpf_charge)
//define PSG_SOUND_DATA (((struct SMSGlobal *)G)->psg_sound_data)
//define PSG_SOUND_DATA_OUT_OFFS (((struct SMSGlobal *)G)->psg_sound_data_out_offs)
//define PSG_SOUND_DATA_OFFS (((struct SMSGlobal *)G)->psg_sound_data_offs)
//define PSG_SOUND_DATA_LEN (((struct SMSGlobal *)G)->psg_sound_data_len)

int16_t psg_sound_data[PSG_OUT_BUF_LEN];
size_t psg_sound_data_out_offs;
size_t psg_sound_data_offs;
#ifndef DEDI
	SDL_atomic_t psg_sound_data_len;
#endif
#define PSG_SOUND_DATA psg_sound_data
#define PSG_SOUND_DATA_OUT_OFFS psg_sound_data_out_offs
#define PSG_SOUND_DATA_OFFS psg_sound_data_offs
#define PSG_SOUND_DATA_LEN psg_sound_data_len

#include "audio/sn76489/core.c"
