#define GPR_ZERO 0
#define GPR_FP 28
#define GPR_GP 29
#define GPR_SP 30
#define GPR_RA 31

struct MIPS
{
	struct EmuState H;

	uint32_t gpr[32];
	uint32_t cop0reg[32];
	uint32_t gtereg[32];
	uint32_t gtectl[32];
	uint32_t rhi, rlo;
	uint32_t pc;
	uint32_t pc_diff1;
	uint32_t lsaddr, lsop;
	int32_t lsreg;
	uint8_t halted;
} __attribute__((__packed__));

