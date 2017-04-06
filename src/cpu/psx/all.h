#define GPR_ZERO 0
#define GPR_AT 1
#define GPR_V0 2
#define GPR_V1 3
#define GPR_A0 4
#define GPR_A1 5
#define GPR_A2 6
#define GPR_A3 7
#define GPR_T0 8
#define GPR_T1 9
#define GPR_T2 10
#define GPR_T3 11
#define GPR_T4 12
#define GPR_T5 13
#define GPR_T6 14
#define GPR_T7 15
#define GPR_S0 16
#define GPR_S1 17
#define GPR_S2 18
#define GPR_S3 19
#define GPR_S4 20
#define GPR_S5 21
#define GPR_S6 22
#define GPR_S7 23
#define GPR_T8 24
#define GPR_T9 25
#define GPR_K0 26
#define GPR_K1 27
#define GPR_GP 28
#define GPR_SP 29
#define GPR_FP 30
#define GPR_RA 31

static const int CAUSE_Int  = 0x00;
static const int CAUSE_AdEL = 0x04;
static const int CAUSE_AdES = 0x05;
static const int CAUSE_IBE  = 0x06;
static const int CAUSE_DBE  = 0x07;
static const int CAUSE_Sys  = 0x08;
static const int CAUSE_Bp   = 0x09;
static const int CAUSE_RI   = 0x0A;
static const int CAUSE_CpU  = 0x0B;
static const int CAUSE_Ov   = 0x0C;

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

	uint32_t new_lsval, lsval;
	int32_t new_lsreg, lsreg;
	uint32_t new_lslatch, lslatch;
	int32_t last_reg_write;

	uint32_t op_pc;
	uint32_t op;

	uint8_t was_branch;
	uint8_t halted;
	uint8_t fault_fired;

	bool is_in_bios;
	uint32_t exe_init_pc;
	uint32_t exe_init_gp;
	uint32_t exe_init_sp;
} __attribute__((__packed__));

