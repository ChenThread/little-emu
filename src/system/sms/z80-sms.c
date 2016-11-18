#include "system/sms/all.h"

#define z80_mem_write sms_z80_mem_write
#define z80_mem_read sms_z80_mem_read
#define z80_io_write sms_z80_io_write
#define z80_io_read sms_z80_io_read

void sms_z80_mem_write(struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp, uint16_t addr, uint8_t val);
uint8_t sms_z80_mem_read(struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp, uint16_t addr);
void sms_z80_io_write(struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp, uint16_t addr, uint8_t val);
uint8_t sms_z80_io_read(struct SMSGlobal *G, struct SMS *sms, uint64_t timestamp, uint16_t addr);

#define Z80_STATE_PARAMS struct SMSGlobal *G, struct SMS *sms
#define Z80_STATE_ARGS G, sms

#define Z80NAME(n) sms_z80_##n

#if 0
// OVERCLOCK
#define Z80_ADD_CYCLES(z80, v) (z80)->H.timestamp += ((v)*1)
#else
// Normal
#define Z80_ADD_CYCLES(z80, v) (z80)->H.timestamp += ((v)*3)
#endif

#include "cpu/z80/core.c"

