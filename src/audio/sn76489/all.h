struct PSG
{
	struct EmuState H;

	uint16_t period[4];
	uint32_t poffs[4];
	uint16_t vol[4];
	uint16_t onstate[4];
	uint16_t lfsr_offs;
	uint8_t lcmd;
	uint8_t lnoise;
} __attribute__((__packed__));

