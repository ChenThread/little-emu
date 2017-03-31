//include "cpu/psx/all.h"

void MIPSNAME(reset)(struct MIPS *mips)
{
	// NOTE: in reality these don't get cleared
	for(int i = 0; i < 32; i++) {
		mips->gpr[i] = 0;
		mips->cop0reg[i] = 0;
		mips->gtereg[i] = 0;
		mips->gtectl[i] = 0;
	}

	// SR
	mips->cop0reg[0x0C] |= (1<<22);

	mips->pc = 0xBFC00000;
	mips->pc_diff1 = 4;
	mips->halted = 0;
	mips->lsreg = -1;
	mips->lsaddr = 0;
	mips->lsop = 0;

	//MIPS_ADD_CYCLES(mips, 1);
}

void MIPSNAME(fault_set)(struct MIPS *mips, MIPS_STATE_PARAMS, int cause)
{
	printf("FAULT %02X %08X\n", cause, mips->op_pc);
	if(cause == CAUSE_RI) {
		uint32_t otyp = (mips->op>>26);
		printf("RI %08X %02X\n", mips->op, otyp);
	}

	mips->lsreg = -1;
	mips->lsop = 0;

	// TODO fill cause in properly
	mips->cop0reg[0x0D] &= ~0x8000007C;
	mips->cop0reg[0x0D] |= ((cause&0x1F)<<2);

	// FIXME this is actually cheating
	if(mips->pc_diff1 != 4) {
		mips->cop0reg[0x0D] |= 0x80000000;
		mips->cop0reg[0x0E] = mips->pc - 4;
	} else {
		mips->cop0reg[0x0E] = mips->pc;
	}

	//if(cause == CAUSE_Ov) { return; }
	mips->fault_fired = true;
	//if(cause == CAUSE_AdEL) { return; }

	mips->cop0reg[0x0C] &= ~0x30;
	mips->cop0reg[0x0C] |= (mips->cop0reg[0x0C] & 0x0F)<<2;
	mips->cop0reg[0x0C] &= ~0x03;

	mips->pc = ((mips->cop0reg[0x0C]&(1<<22)) == 0
		? 0x80000080
		: 0xBFC00180);
	mips->pc_diff1 = 4;
	//printf("NEW PC: %08X\n", mips->pc);
}

static uint32_t MIPSNAME(mem_read32_direct)(struct MIPS *mips, MIPS_STATE_PARAMS, uint32_t addr)
{
	// TODO cache stuff
	// TODO add a latch to this
	if(addr == 0xFFFE0130) {
		// TODO!
		int ret = 0;
		printf("INTERN1 R -> %08X\n", ret);
		return ret;
	}

	uint32_t v = MIPSNAME(mem_read)(MIPS_STATE_ARGS,
		mips->H.timestamp,
		addr & 0x1FFFFFFF,
		0xFFFFFFFF);

	return v;
}

static uint32_t MIPSNAME(mem_read32_l)(struct MIPS *mips, MIPS_STATE_PARAMS, uint32_t addr, uint32_t oldval)
{
	uint32_t mask = (0xFFFFFFFF<<(((~addr)&3)<<3));
	uint32_t v = MIPSNAME(mem_read32_direct)(mips, MIPS_STATE_ARGS, addr&~3);
	v <<= ((~addr)&3)<<3;
	return (v&mask)|(oldval&~mask);
}

static uint32_t MIPSNAME(mem_read32_r)(struct MIPS *mips, MIPS_STATE_PARAMS, uint32_t addr, uint32_t oldval)
{
	uint32_t mask = (0xFFFFFFFF>>((addr&3)<<3));
	uint32_t v = MIPSNAME(mem_read32_direct)(mips, MIPS_STATE_ARGS, addr&~3);
	v >>= (addr&3)<<3;
	return (v&mask)|(oldval&~mask);
}

static uint32_t MIPSNAME(mem_read32)(struct MIPS *mips, MIPS_STATE_PARAMS, uint32_t addr)
{
	if((addr & 0x3) != 0) {
		psx_mips_fault_set(mips, MIPS_STATE_ARGS, CAUSE_AdEL);
		return 0xFFFFFFFF;
	}

	uint32_t v = MIPSNAME(mem_read32_direct)(mips, MIPS_STATE_ARGS, addr);
	return v;
}

static uint32_t MIPSNAME(mem_read16)(struct MIPS *mips, MIPS_STATE_PARAMS, uint32_t addr)
{
	if((addr & 0x1) != 0) {
		psx_mips_fault_set(mips, MIPS_STATE_ARGS, CAUSE_AdEL);
		return 0xFFFFFFFF;
	}

	uint32_t v = MIPSNAME(mem_read32_direct)(mips, MIPS_STATE_ARGS, addr);
	return (uint32_t)(int32_t)(int16_t)(v >> ((addr&2)<<3));
}

static uint32_t MIPSNAME(mem_read8)(struct MIPS *mips, MIPS_STATE_PARAMS, uint32_t addr)
{
	uint32_t v = MIPSNAME(mem_read32_direct)(mips, MIPS_STATE_ARGS, addr);
	return (uint32_t)(int32_t)(int8_t)(v >> ((addr&3)<<3));
}

static void MIPSNAME(mem_write32_direct)(struct MIPS *mips, MIPS_STATE_PARAMS, uint32_t addr, uint32_t latch, uint32_t val)
{
	// TODO cache stuff
	if(addr == 0xFFFE0130) {
		// TODO!
		printf("INTERN1 W %08X\n", val);
		return;
	}

	MIPSNAME(mem_write)(MIPS_STATE_ARGS,
		mips->H.timestamp,
		addr & 0x1FFFFFFF,
		latch,
		val);
}

static uint32_t MIPSNAME(fetch_op_x)(struct MIPS *mips, MIPS_STATE_PARAMS)
{
	// TODO cache stuff
	uint32_t op = MIPSNAME(mem_read)(MIPS_STATE_ARGS,
		mips->H.timestamp,
		mips->pc & 0x1FFFFFFF,
		0xFFFFFFFF);

	//printf("\n****\n%08X %08X %08X\n", mips->pc, mips->pc_diff1, mips->pc + mips->pc_diff1);
	mips->pc += mips->pc_diff1;
	mips->pc_diff1 = 4;
	MIPS_ADD_CYCLES(mips, 1);
	return op;
}

void MIPSNAME(run)(struct MIPS *mips, MIPS_STATE_PARAMS, uint64_t timestamp)
{
	// Don't jump back into the past
	if(!TIME_IN_ORDER(mips->H.timestamp, timestamp)) {
		return;
	}

	// If halted, don't waste time fetching ops
	if(mips->halted) {
		mips->H.timestamp = timestamp;
		return;
	}

	// Run ops
	uint64_t lstamp = mips->H.timestamp;
	mips->H.timestamp_end = timestamp;
	while(TIME_IN_ORDER(mips->H.timestamp, mips->H.timestamp_end)) {
		if(false) {
			printf("%020lld: %04X: SP=%04X\n"
				, (unsigned long long)((mips->H.timestamp-lstamp)/(uint64_t)1)
				, mips->pc, mips->gpr[GPR_SP]);
		}

		// Check for IRQ
		// TODO!

		lstamp = mips->H.timestamp;

		//
		// HLE. THIS IS CANCER. FIXME THIS IS CANCER HLE IS CANCER FIX THIS CANCER DO IT RIGHT INSTEAD OF HLE-ING IT
		//
#if 1
		if((mips->pc&0x1FFFFFFF) == 0xA0) {
			switch(mips->gpr[9]&0xFFFF) {
				case 0x3E: // puts(fmt, ...)
					MIPSNAME(puts)(mips, MIPS_STATE_ARGS);
					break;
				case 0x3F: // printf(fmt, ...)
					MIPSNAME(printf)(mips, MIPS_STATE_ARGS);
					break;
				case 0x44: // FlushCache()
					break;
				default:
					printf("HLE: A0 %08X\n", mips->gpr[9]);
					abort();
					break;
			}
			mips->pc = mips->gpr[31];
			mips->pc_diff1 = 4;
			continue;
		} else if((mips->pc&0x1FFFFFFF) == 0xB0) {
			printf("HLE: B0 %08X\n", mips->gpr[9]);
			abort();
		} else if((mips->pc&0x1FFFFFFF) == 0xC0) {
			printf("HLE: C0 %08X\n", mips->gpr[9]);
			abort();
		}
#endif

		// Fetch
		mips->fault_fired = false;
		mips->op_pc = mips->pc;
		mips->op = MIPSNAME(fetch_op_x)(mips, MIPS_STATE_ARGS);
		if(mips->fault_fired) {
			printf("FIX YOUR SHIT %08X\n", mips->pc);
			continue;
		}

		// Clear r0
		mips->gpr[GPR_ZERO] = 0;

		//printf("%08X %08X %08X\n", mips->op_pc, mips->op, mips->gpr[GPR_SP]);
	
		int rs = (mips->op>>21)&0x1F;
		int rt = (mips->op>>16)&0x1F;
		int rd = (mips->op>>11)&0x1F;
		int sa = (mips->op>>6)&0x1F;
		int ofunc = mips->op&0x3F;
		int otyp = (mips->op>>26)&0x3F;

		// Decode
		if(otyp == 0) switch(ofunc) {
			case 0x00: // SLL
				mips->gpr[rd] = mips->gpr[rt] << sa;
				break;
			case 0x02: // SRL
				mips->gpr[rd] = ((uint32_t)mips->gpr[rt]) >> sa;
				break;
			case 0x03: // SRA
				mips->gpr[rd] = (uint32_t)(((int32_t)mips->gpr[rt]) >> sa);
				break;
			case 0x04: // SLLV
				mips->gpr[rd] = mips->gpr[rt] << mips->gpr[rs];
				break;
			case 0x06: // SRLV
				mips->gpr[rd] = ((uint32_t)mips->gpr[rt]) >> (uint32_t)mips->gpr[rs];
				break;
			case 0x07: // SRAV
				mips->gpr[rd] = (uint32_t)(((int32_t)mips->gpr[rt]) >> mips->gpr[rs]);
				break;

			case 0x09: // JALR
				mips->gpr[rd] = mips->op_pc + 8;
				// FALL THROUGH
			case 0x08: // JR
				//printf("JUMP %08X %d\n", mips->gpr[rs], rs);
				mips->pc_diff1 = mips->gpr[rs];
				mips->pc_diff1 -= mips->pc;
				//xcomms |= 0x10;
				break;

			case 0x0C: // SYSCALL
				MIPSNAME(fault_set)(mips, MIPS_STATE_ARGS, CAUSE_Sys);
				break;
			case 0x0D: // BREAK
				MIPSNAME(fault_set)(mips, MIPS_STATE_ARGS, CAUSE_Bp);
				break;

			case 0x10: // MFHI
				//printf("MFHI %08X\n", mips->op_pc);
				mips->gpr[rd] = mips->rhi;
				break;
			case 0x11: // MTHI
				//printf("MTHI %08X\n", mips->op_pc);
				mips->rhi = mips->gpr[rs];
				break;
			case 0x12: // MFLO
				//printf("MFLO %08X\n", mips->op_pc);
				mips->gpr[rd] = mips->rlo;
				break;
			case 0x13: // MTLO
				//printf("MTLO %08X\n", mips->op_pc);
				mips->rlo = mips->gpr[rs];
				break;

			// Because we actually have 64-bit ints in C,
			// as opposed to only having 32-bit ints in GLSL 4.30,
			// this part is a bit nicer.
			case 0x18: // MUL
				{
					int64_t va = (int64_t)(int32_t)mips->gpr[rs];
					int64_t vb = (int64_t)(int32_t)mips->gpr[rt];
					int64_t cmbres = (va*vb);

					mips->rlo = (uint32_t)(uint64_t)(cmbres);
					mips->rhi = (uint32_t)(uint64_t)(cmbres>>32ULL);
#if 0
				printf("%08X %08X %08X %08X %08X\n"
					, mips->gpr[rs]
					, mips->gpr[rt]
					, mips->rlo
					, mips->rhi
					, mips->op_pc
				);
#endif
				}
			case 0x19: // MULU
				{
					uint64_t va = (uint64_t)(uint32_t)mips->gpr[rs];
					uint64_t vb = (uint64_t)(uint32_t)mips->gpr[rt];
					uint64_t cmbres = (va*vb);

					mips->rlo = (uint32_t)(uint64_t)(cmbres);
					mips->rhi = (uint32_t)(uint64_t)(cmbres>>32ULL);
				}
				break;

			case 0x1A: // DIV
				if(mips->gpr[rt] == 0) {
					// DIV BY ZERO
					mips->rhi = mips->gpr[rs];
					mips->rlo = ((int32_t)mips->gpr[rs] < 0 ? 1 : -1);
				} else {
					mips->rhi = ((int64_t)(int32_t)mips->gpr[rs]) % (int64_t)(int32_t)mips->gpr[rt];
					mips->rlo = ((int64_t)(int32_t)mips->gpr[rs]) / (int64_t)(int32_t)mips->gpr[rt];
				}
#if 0
				static int div_counter = 0;
				printf("%5d %08X %08X %08X %08X\n"
					, div_counter++
					, mips->gpr[rs]
					, mips->gpr[rt]
					, mips->rlo
					, mips->rhi
				);
#endif
				break;
			case 0x1B: // DIVU
				if(mips->gpr[rt] == 0) {
					// DIV BY ZERO
					mips->rhi = mips->gpr[rs];
					mips->rlo = -1;
				} else {
					mips->rhi = ((uint32_t)(mips->gpr[rs]) % (uint32_t)(mips->gpr[rt]));
					mips->rlo = ((uint32_t)(mips->gpr[rs]) / (uint32_t)(mips->gpr[rt]));
				}
				break;

			// TODO: overflow traps
			case 0x20: // ADD
				{
					uint32_t va = mips->gpr[rs];
					uint32_t vb = mips->gpr[rt];
					uint32_t vr = va + vb;
					if(((va^vb)>>31) == 0 && ((vr^va)>>31) != 0) {
						MIPSNAME(fault_set)(mips, MIPS_STATE_ARGS, CAUSE_Ov);
					} else {
						mips->gpr[rd] = vr;
					}
				} break;
			case 0x21: // ADDU
				mips->gpr[rd] = mips->gpr[rs] + mips->gpr[rt];
				break;
			case 0x22: // SUB
				{
					uint32_t va = mips->gpr[rs];
					uint32_t vb = mips->gpr[rt];
					uint32_t vr = va - vb;
					if(((va^vb)>>31) != 0 && ((vr^va)>>31) != 0) {
						MIPSNAME(fault_set)(mips, MIPS_STATE_ARGS, CAUSE_Ov);
					} else {
						mips->gpr[rd] = vr;
					}
				} break;
			case 0x23: // SUBU
				mips->gpr[rd] = mips->gpr[rs] - mips->gpr[rt];
				break;

			case 0x24: // AND
				mips->gpr[rd] = mips->gpr[rs] & mips->gpr[rt];
				break;
			case 0x25: // OR
				mips->gpr[rd] = mips->gpr[rs] | mips->gpr[rt];
				break;
			case 0x26: // XOR
				mips->gpr[rd] = mips->gpr[rs] ^ mips->gpr[rt];
				break;
			case 0x27: // NOR
				mips->gpr[rd] = ~(mips->gpr[rs] | mips->gpr[rt]);
				break;

			case 0x2A: // SLT
				mips->gpr[rd] = ((int32_t)mips->gpr[rs] < (int32_t)mips->gpr[rt] ? 1 : 0);
				break;
			case 0x2B: // SLTU
				mips->gpr[rd] = ((uint32_t)(mips->gpr[rs]) < (uint32_t)(mips->gpr[rt]) ? 1 : 0);
				break;

			default:
				// RI
				MIPSNAME(fault_set)(mips, MIPS_STATE_ARGS, CAUSE_RI);
				break;
		} else if(otyp == 0x01) switch(rt) {
			case 0x00: // BLTZ
				if((int32_t)mips->gpr[rs] < 0) {
					mips->pc_diff1 = ((int32_t)(int16_t)mips->op)<<2;
					//xcomms |= 0x10;
				}
				break;
			case 0x01: // BGEZ
				if((int32_t)mips->gpr[rs] >= 0) {
					mips->pc_diff1 = ((int32_t)(int16_t)mips->op)<<2;
					//xcomms |= 0x10;
				}
				break;
			case 0x10: // BLTZAL
				if((int32_t)mips->gpr[rs] < 0) {
					mips->gpr[31] = mips->op_pc + 8;
					mips->pc_diff1 = ((int32_t)(int16_t)mips->op)<<2;
					//xcomms |= 0x10;
				}
				break;
			case 0x11: // BGEZAL
				if((int32_t)mips->gpr[rs] >= 0) {
					mips->gpr[31] = mips->op_pc + 8;
					mips->pc_diff1 = ((int32_t)(int16_t)mips->op)<<2;
					//xcomms |= 0x10;
				}
				break;

			default:
				// RI
				MIPSNAME(fault_set)(mips, MIPS_STATE_ARGS, CAUSE_RI);
				break;

		} else if(otyp == 0x10) switch(rs) {
			case 0x00: // MFC
				switch(rd) {
					case 0x03: // BPC
						mips->gpr[rt] = mips->cop0reg[rd];
						break;

					case 0x06: // JumpDest
						mips->gpr[rt] = mips->cop0reg[rd];
						break;

					case 0x07: // DCIC
						mips->gpr[rt] = mips->cop0reg[rd];
						break;

					case 0x08: // BadVAddr
						mips->gpr[rt] = mips->cop0reg[rd];
						break;

					case 0x0C: // SR
						mips->gpr[rt] = mips->cop0reg[rd];
						break;

					case 0x0D: // CAUSE
						mips->gpr[rt] = mips->cop0reg[rd];
						break;

					case 0x0E: // EPC
						mips->gpr[rt] = mips->cop0reg[rd];
						break;

					case 0x0F: // PRId
						mips->gpr[rt] = 0x00000002;
						break;

					default:
						// RI
						printf("rd mfc %02X\n", rd);
						MIPSNAME(fault_set)(mips, MIPS_STATE_ARGS, CAUSE_RI);
						break;

				} break;

			case 0x04: // MTC
				switch(rd) {

					case 0x03: // BPC
						// TODO!
						mips->cop0reg[rd] = mips->gpr[rt];
						break;

					case 0x05: // BDA
						// TODO!
						mips->cop0reg[rd] = mips->gpr[rt];
						break;

					case 0x06: // JUMPDEST
						// TODO!
						mips->cop0reg[rd] = mips->gpr[rt];
						break;

					case 0x07: // DCIC
						// TODO!
						mips->cop0reg[rd] = mips->gpr[rt];
						break;

					case 0x09: // BDAM
						// TODO!
						mips->cop0reg[rd] = mips->gpr[rt];
						break;

					case 0x0B: // BPCM
						// TODO!
						mips->cop0reg[rd] = mips->gpr[rt];
						break;

					case 0x0C: // SR
						// TODO!
						mips->cop0reg[rd] = mips->gpr[rt];
						break;

					case 0x0D: // CAUSE
						mips->cop0reg[rd] = (mips->cop0reg[rd] & ~0x0300)
							| (mips->gpr[rt] & 0x0300);
						break;

					default:
						// RI
						printf("rd mtc %02X\n", rd);
						MIPSNAME(fault_set)(mips, MIPS_STATE_ARGS, CAUSE_RI);
						break;

				} break;

			case 0x10: // COP0
				switch(mips->op&0x3F) {
					case 0x10: // RFE
						mips->cop0reg[0x0C] = (mips->cop0reg[0x0C]&~0x0F)
							| ((mips->cop0reg[0x0C]>>2)&0x0F);
						break;

					default:
						// RI
						printf("cop0 %02X\n", (mips->op&0x3F));
						MIPSNAME(fault_set)(mips, MIPS_STATE_ARGS, CAUSE_RI);
						break;

				} break;

			default:
				// RI
				printf("rs %02X\n", rs);
				MIPSNAME(fault_set)(mips, MIPS_STATE_ARGS, CAUSE_RI);
				break;
		} else switch(otyp) {
			case 0x03: // JAL
				mips->gpr[31] = mips->op_pc + 8;
				// FALL THROUGH
			case 0x02: // J
				mips->pc_diff1 = (mips->op_pc&0xF0000000)|((mips->op&0x3FFFFFF)<<2);
				mips->pc_diff1 -= mips->pc;
				//xcomms |= 0x10;
				break;

			case 0x04: // BEQ
				if(mips->gpr[rs] == mips->gpr[rt]) {
					mips->pc_diff1 = ((int32_t)(int16_t)mips->op)<<2;
					//xcomms |= 0x10;
				}
				break;
			case 0x05: // BNE
				if(mips->gpr[rs] != mips->gpr[rt]) {
					mips->pc_diff1 = ((int32_t)(int16_t)mips->op)<<2;
					//xcomms |= 0x10;
				}
				break;
			case 0x06: // BLEZ
				if(mips->gpr[rs] <= 0) {
					mips->pc_diff1 = ((int32_t)(int16_t)mips->op)<<2;
					//xcomms |= 0x10;
				}
				break;
			case 0x07: // BGTZ
				if(mips->gpr[rs] > 0) {
					mips->pc_diff1 = ((int32_t)(int16_t)mips->op)<<2;
					//xcomms |= 0x10;
				}
				break;

			case 0x08: // ADDI
				{
					uint32_t va = mips->gpr[rs];
					uint32_t vb = (uint32_t)(int32_t)(int16_t)mips->op;
					uint32_t vr = va + vb;
					if(((va^vb)>>31) == 0 && ((vr^va)>>31) != 0) {
						MIPSNAME(fault_set)(mips, MIPS_STATE_ARGS, CAUSE_Ov);
					} else {
						mips->gpr[rt] = vr;
					}
				} break;

			case 0x09: // ADDIU
				mips->gpr[rt] = mips->gpr[rs] + (uint32_t)(int32_t)(int16_t)mips->op;
				break;
			case 0x0A: // SLTI
				mips->gpr[rt] = ((int32_t)mips->gpr[rs] < (int32_t)(int16_t)(int32_t)mips->op ? 1 : 0);
				break;
			case 0x0B: // SLTIU
				mips->gpr[rt] = (((uint32_t)(mips->gpr[rs]) < (uint32_t)((int32_t)(int16_t)mips->op)) ? 1 : 0);
				break;

			case 0x0C: // ANDI
				mips->gpr[rt] = mips->gpr[rs] & (mips->op&0xFFFF);
				break;
			case 0x0D: // ORI
				mips->gpr[rt] = mips->gpr[rs] | (mips->op&0xFFFF);
				break;
			case 0x0E: // XORI
				mips->gpr[rt] = mips->gpr[rs] ^ (mips->op&0xFFFF);
				break;
			case 0x0F: // LUI
				mips->gpr[rt] = (mips->op&0xFFFF)<<16;
				break;

			case 0x20: // LB
				mips->lsaddr = mips->gpr[rs] + (uint32_t)(int32_t)(int16_t)mips->op;
				mips->lsreg = rt;
				break;
			case 0x21: // LH
				mips->lsaddr = mips->gpr[rs] + (uint32_t)(int32_t)(int16_t)mips->op;
				mips->lsreg = rt;
				break;
			case 0x22: // LWL
				mips->lsaddr = mips->gpr[rs] + (uint32_t)(int32_t)(int16_t)mips->op;
				mips->lsreg = rt;
				break;
			case 0x23: // LW
				mips->lsaddr = mips->gpr[rs] + (uint32_t)(int32_t)(int16_t)mips->op;
				mips->lsreg = rt;
				break;
			case 0x24: // LBU
				mips->lsaddr = mips->gpr[rs] + (uint32_t)(int32_t)(int16_t)mips->op;
				mips->lsreg = rt;
				break;
			case 0x25: // LHU
				mips->lsaddr = mips->gpr[rs] + (uint32_t)(int32_t)(int16_t)mips->op;
				mips->lsreg = rt;
				break;
			case 0x26: // LWR
				mips->lsaddr = mips->gpr[rs] + (uint32_t)(int32_t)(int16_t)mips->op;
				mips->lsreg = rt;
				break;

			case 0x28: // SB
				mips->lsaddr = mips->gpr[rs] + (uint32_t)(int32_t)(int16_t)mips->op;
				mips->lsreg = -2-rt;
				break;
			case 0x29: // SH
				mips->lsaddr = mips->gpr[rs] + (uint32_t)(int32_t)(int16_t)mips->op;
				mips->lsreg = -2-rt;
				break;
			case 0x2B: // SW
				mips->lsaddr = mips->gpr[rs] + (uint32_t)(int32_t)(int16_t)mips->op;
				mips->lsreg = -2-rt;
				break;

			default:
				// RI
				MIPSNAME(fault_set)(mips, MIPS_STATE_ARGS, CAUSE_RI);
				break;
		}

		if(mips->lsreg != -1) {
			mips->lsop = otyp;
		}

		// Check if storing
		if(mips->lsreg <= -2) {
			// Store
			mips->lsreg = -2-mips->lsreg;
			if((mips->cop0reg[0x0C] & 0x10000) == 0) {
				// TODO: handle the isolated cache variant properly
				int mask = 0;
				int v = mips->gpr[mips->lsreg];
				int shift = 0;
				switch(mips->lsop) {
					case 0x28: // SB
						shift = ((mips->lsaddr&3)<<3);
						mask = 0xFF;
						break;
					case 0x29: // SH
						shift = ((mips->lsaddr&2)<<3);
						mask = 0xFFFF;
						break;
					case 0x2B: // SW
						shift = 0;
						mask = 0xFFFFFFFF;
						break;
				}
				MIPSNAME(mem_write32_direct)(mips, MIPS_STATE_ARGS, mips->lsaddr, mask<<shift, (v&mask)<<shift);
			}
			mips->lsreg = -1;
			continue;
		}

		// Fetch op or thing to load
		if(mips->lsreg >= 0) {
			// Load
			mips->fault_fired = false;
			int v = 0;
			if((mips->cop0reg[0x0C] & 0x10000) == 0) {
				// TODO: handle the isolated cache variant properly
				switch(mips->lsop) {
					case 0x20: // LB
						v = MIPSNAME(mem_read8)(mips, MIPS_STATE_ARGS, mips->lsaddr);
						break;
					case 0x21: // LH
						//printf("LH %08X %08X\n", mips->op_pc, mips->lsaddr);
						v = MIPSNAME(mem_read16)(mips, MIPS_STATE_ARGS, mips->lsaddr);
						break;
					case 0x22: // LWL
						v = MIPSNAME(mem_read32_l)(mips, MIPS_STATE_ARGS, mips->lsaddr, mips->gpr[mips->lsreg]);
						break;
					case 0x23: // LW
						v = MIPSNAME(mem_read32)(mips, MIPS_STATE_ARGS, mips->lsaddr);
						break;
					case 0x24: // LBU
						v = MIPSNAME(mem_read8)(mips, MIPS_STATE_ARGS, mips->lsaddr);
						v = (uint32_t)(uint8_t)v;
						break;
					case 0x25: // LHU
						v = MIPSNAME(mem_read16)(mips, MIPS_STATE_ARGS, mips->lsaddr);
						v = (uint32_t)(uint16_t)v;
						break;
					case 0x26: // LWR
						v = MIPSNAME(mem_read32_r)(mips, MIPS_STATE_ARGS, mips->lsaddr, mips->gpr[mips->lsreg]);
						break;
				}
			}
			if(!mips->fault_fired) {
				mips->gpr[mips->lsreg] = v;
			}
			mips->lsreg = -1;
			continue;
		}

	}
}

void MIPSNAME(init)(struct EmuGlobal *H, struct MIPS *mips)
{
	*mips = (struct MIPS){ .H={.timestamp=0,}, };
	MIPSNAME(reset)(mips);
}

