#define RB 0
#define RC 1
#define RD 2
#define RE 3
#define RH 4
#define RL 5
#define RF 6
#define RA 7

struct Z80
{
	// CPU state
	uint8_t gpr[8];
	uint8_t shadow[8];
	uint8_t idx[2][2];
	uint8_t wz[4]; // internal register + shadow; required for some flag bit 3/5 stuff
	uint8_t i,r,iff1,iff2;
	uint16_t sp;
	uint16_t pc;
	uint8_t halted, im, noni;

	// Tracking state
	uint64_t timestamp;
	uint64_t timestamp_end;
} __attribute__((__packed__));

