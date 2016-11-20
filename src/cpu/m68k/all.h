struct M68K
{
	struct EmuState H;

	uint32_t ra[8];
	uint32_t rd[8];
	uint32_t pc;
	uint32_t last_ea, last_non_ea;
	uint32_t usp_store;
	uint16_t sr;

	uint8_t halted;
	uint8_t needs_reset;
} __attribute__((__packed__));

#define F_X 0x0010
#define F_N 0x0008
#define F_Z 0x0004
#define F_V 0x0002
#define F_C 0x0001

