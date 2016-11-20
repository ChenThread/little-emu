#define FLAG_N (1 << 7)
#define FLAG_V (1 << 6)
#define FLAG_B (1 << 4)
#define FLAG_D (1 << 3)
#define FLAG_I (1 << 2)
#define FLAG_Z (1 << 1)
#define FLAG_C (1 << 0)

struct CPU_6502
{
	struct EmuState H;
	
	uint8_t ra, rx, ry, sp, flag;
	uint16_t pc;

	bool irq_request, nmi_request;
} __attribute__((__packed__));

void cpu_6502_irq(struct CPU_6502 *state);
void cpu_6502_nmi(struct CPU_6502 *state);