#define PSG_OUT_BUF_LEN (1<<24)

struct PSG
{
	// PSG state
	uint16_t period[4];
	uint32_t poffs[4];
	uint16_t vol[4];
	uint16_t onstate[4];
	uint16_t lfsr_offs;
	uint8_t lcmd;
	uint8_t lnoise;
	
	// Tracking state
	uint64_t timestamp;
	uint64_t timestamp_end;
} __attribute__((__packed__));

