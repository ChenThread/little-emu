static CPU_6502_NAME(opcode)* CPU_6502_NAME(opcodes)[] = {
	CPU_6502_NAME(brk), CPU_6502_NAME(ora_zpindx), CPU_6502_NAME(hlt), CPU_6502_NAME(i_slo_zpindx), // 00
	CPU_6502_NAME(nop_zp), CPU_6502_NAME(ora_zp), CPU_6502_NAME(asl_zp), CPU_6502_NAME(i_slo_zp), // 04
	CPU_6502_NAME(php), CPU_6502_NAME(ora_imm), CPU_6502_NAME(asl_ra), CPU_6502_NAME(i_anc_imm), // 08
	CPU_6502_NAME(nop_abs), CPU_6502_NAME(ora_abs), CPU_6502_NAME(asl_abs), CPU_6502_NAME(i_slo_abs), // 0C
	CPU_6502_NAME(bpl), CPU_6502_NAME(ora_zpindy), CPU_6502_NAME(hlt), CPU_6502_NAME(i_slo_zpindy), // 10 
	CPU_6502_NAME(nop_zpx), CPU_6502_NAME(ora_zpx), CPU_6502_NAME(asl_zpx), CPU_6502_NAME(i_slo_zpx), // 14
	CPU_6502_NAME(clc), CPU_6502_NAME(ora_absy), CPU_6502_NAME(nop), CPU_6502_NAME(i_slo_absy), // 18
	CPU_6502_NAME(nop_absx), CPU_6502_NAME(ora_absx), CPU_6502_NAME(asl_absx), CPU_6502_NAME(i_slo_absx), // 1C
	CPU_6502_NAME(jsr), CPU_6502_NAME(and_zpindx), CPU_6502_NAME(hlt), CPU_6502_NAME(i_rla_zpindx), // 20
	CPU_6502_NAME(bit_zp), CPU_6502_NAME(and_zp), CPU_6502_NAME(rol_zp), CPU_6502_NAME(i_rla_zp), // 24
	CPU_6502_NAME(plp), CPU_6502_NAME(and_imm), CPU_6502_NAME(rol_ra), CPU_6502_NAME(i_anc_imm), // 28
	CPU_6502_NAME(bit_abs), CPU_6502_NAME(and_abs), CPU_6502_NAME(rol_abs), CPU_6502_NAME(i_rla_abs), // 2C
	CPU_6502_NAME(bmi), CPU_6502_NAME(and_zpindy), CPU_6502_NAME(hlt), CPU_6502_NAME(i_rla_zpindy), // 30
	CPU_6502_NAME(nop_zpx), CPU_6502_NAME(and_zpx), CPU_6502_NAME(rol_zpx), CPU_6502_NAME(i_rla_zpx), // 34
	CPU_6502_NAME(sec), CPU_6502_NAME(and_absy), CPU_6502_NAME(nop), CPU_6502_NAME(i_rla_absy), // 38
	CPU_6502_NAME(nop_absx), CPU_6502_NAME(and_absx), CPU_6502_NAME(rol_absx), CPU_6502_NAME(i_rla_absx), // 3C
	CPU_6502_NAME(rti), CPU_6502_NAME(eor_zpindx), CPU_6502_NAME(hlt), CPU_6502_NAME(i_sre_zpindx), // 40
	CPU_6502_NAME(nop_zp), CPU_6502_NAME(eor_zp), CPU_6502_NAME(lsr_zp), CPU_6502_NAME(i_sre_zp), // 44
	CPU_6502_NAME(pha), CPU_6502_NAME(eor_imm), CPU_6502_NAME(lsr_ra), CPU_6502_NAME(i_alr_imm), // 48
	CPU_6502_NAME(jmp_abs), CPU_6502_NAME(eor_abs), CPU_6502_NAME(lsr_abs), CPU_6502_NAME(i_sre_abs), // 4C
	CPU_6502_NAME(bvc), CPU_6502_NAME(eor_zpindy), CPU_6502_NAME(hlt), CPU_6502_NAME(i_sre_zpindy), // 50
	CPU_6502_NAME(nop_zpx), CPU_6502_NAME(eor_zpx), CPU_6502_NAME(lsr_zpx), CPU_6502_NAME(i_sre_zpx), // 54
	CPU_6502_NAME(cli), CPU_6502_NAME(eor_absy), CPU_6502_NAME(nop), CPU_6502_NAME(i_sre_absy), // 58
	CPU_6502_NAME(nop_absx), CPU_6502_NAME(eor_absx), CPU_6502_NAME(lsr_absx), CPU_6502_NAME(i_sre_absx), // 5C
	CPU_6502_NAME(rts), CPU_6502_NAME(adc_zpindx), CPU_6502_NAME(hlt), CPU_6502_NAME(i_rra_zpindx), // 60
	CPU_6502_NAME(nop_zp), CPU_6502_NAME(adc_zp), CPU_6502_NAME(ror_zp), CPU_6502_NAME(i_rra_zp), // 64
	CPU_6502_NAME(pla), CPU_6502_NAME(adc_imm), CPU_6502_NAME(ror_ra), CPU_6502_NAME(i_arr_imm), // 68
	CPU_6502_NAME(jmp_ind_page), CPU_6502_NAME(adc_abs), CPU_6502_NAME(ror_abs), CPU_6502_NAME(i_rra_abs), // 6C
	CPU_6502_NAME(bvs), CPU_6502_NAME(adc_zpindy), CPU_6502_NAME(hlt), CPU_6502_NAME(i_rra_zpindy), // 70
	CPU_6502_NAME(nop_zpx), CPU_6502_NAME(adc_zpx), CPU_6502_NAME(ror_zpx), CPU_6502_NAME(i_rra_zpx), // 74
	CPU_6502_NAME(sei), CPU_6502_NAME(adc_absy), CPU_6502_NAME(nop), CPU_6502_NAME(i_rra_absy), // 78
	CPU_6502_NAME(nop_absx), CPU_6502_NAME(adc_absx), CPU_6502_NAME(ror_absx), CPU_6502_NAME(i_rra_absx), // 7C
	CPU_6502_NAME(nop_imm), CPU_6502_NAME(sta_zpindx), CPU_6502_NAME(nop_imm), CPU_6502_NAME(i_sax_zpindx), // 80
	CPU_6502_NAME(sty_zp), CPU_6502_NAME(sta_zp), CPU_6502_NAME(stx_zp), CPU_6502_NAME(i_sax_zp), // 84
	CPU_6502_NAME(dey), CPU_6502_NAME(nop_imm), CPU_6502_NAME(txa), CPU_6502_NAME(i_xaa_imm), // 88
	CPU_6502_NAME(sty_abs), CPU_6502_NAME(sta_abs), CPU_6502_NAME(stx_abs), CPU_6502_NAME(i_sax_abs), // 8C
	CPU_6502_NAME(bcc), CPU_6502_NAME(sta_zpindy), CPU_6502_NAME(hlt), CPU_6502_NAME(i_ahx_zpindy), // 90
	CPU_6502_NAME(sty_zpx), CPU_6502_NAME(sta_zpx), CPU_6502_NAME(stx_zpy), CPU_6502_NAME(i_sax_zpy), // 94
	CPU_6502_NAME(tya), CPU_6502_NAME(sta_absy), CPU_6502_NAME(txs), CPU_6502_NAME(i_tas_absy), // 98
	CPU_6502_NAME(i_shy_absx), CPU_6502_NAME(sta_absx), CPU_6502_NAME(i_shx_absx), CPU_6502_NAME(i_ahx_absy), // 9C
	CPU_6502_NAME(ldy_imm), CPU_6502_NAME(lda_zpindx), CPU_6502_NAME(ldx_imm), CPU_6502_NAME(i_lax_zpindx), // A0
	CPU_6502_NAME(ldy_zp), CPU_6502_NAME(lda_zp), CPU_6502_NAME(ldx_zp), CPU_6502_NAME(i_lax_zp), // A4
	CPU_6502_NAME(tay), CPU_6502_NAME(lda_imm), CPU_6502_NAME(tax), CPU_6502_NAME(i_lax_imm), // A8
	CPU_6502_NAME(ldy_abs), CPU_6502_NAME(lda_abs), CPU_6502_NAME(ldx_abs), CPU_6502_NAME(i_lax_abs), // AC
	CPU_6502_NAME(bcs), CPU_6502_NAME(lda_zpindy), CPU_6502_NAME(hlt), CPU_6502_NAME(i_lax_zpindy), // B0
	CPU_6502_NAME(ldy_zpx), CPU_6502_NAME(lda_zpx), CPU_6502_NAME(ldx_zpy), CPU_6502_NAME(i_lax_zpy), // B4
	CPU_6502_NAME(clv), CPU_6502_NAME(lda_absy), CPU_6502_NAME(tsx), CPU_6502_NAME(i_las_absy), // B8
	CPU_6502_NAME(ldy_absx), CPU_6502_NAME(lda_absx), CPU_6502_NAME(ldx_absy), CPU_6502_NAME(i_lax_absy), // BC
	CPU_6502_NAME(cpy_imm), CPU_6502_NAME(cmp_zpindx), CPU_6502_NAME(nop_imm), CPU_6502_NAME(i_dcp_zpindx), // C0
	CPU_6502_NAME(cpy_zp), CPU_6502_NAME(cmp_zp), CPU_6502_NAME(dec_zp), CPU_6502_NAME(i_dcp_zp), // C4
	CPU_6502_NAME(iny), CPU_6502_NAME(cmp_imm), CPU_6502_NAME(dex), CPU_6502_NAME(i_axs_imm), // C8
	CPU_6502_NAME(cpy_abs), CPU_6502_NAME(cmp_abs), CPU_6502_NAME(dec_abs), CPU_6502_NAME(i_dcp_abs), // CC
	CPU_6502_NAME(bne), CPU_6502_NAME(cmp_zpindy), CPU_6502_NAME(hlt), CPU_6502_NAME(i_dcp_zpindy), // D0
	CPU_6502_NAME(nop_zpx), CPU_6502_NAME(cmp_zpx), CPU_6502_NAME(dec_zpx), CPU_6502_NAME(i_dcp_zpx), // D4
	CPU_6502_NAME(cld), CPU_6502_NAME(cmp_absy), CPU_6502_NAME(nop), CPU_6502_NAME(i_dcp_absy), // D8
	CPU_6502_NAME(nop_absx), CPU_6502_NAME(cmp_absx), CPU_6502_NAME(dec_absx), CPU_6502_NAME(i_dcp_absx), // DC
	CPU_6502_NAME(cpx_imm), CPU_6502_NAME(sbc_zpindx), CPU_6502_NAME(nop_imm), CPU_6502_NAME(i_isc_zpindx), // E0
	CPU_6502_NAME(cpx_zp), CPU_6502_NAME(sbc_zp), CPU_6502_NAME(inc_zp), CPU_6502_NAME(i_isc_zp), // E4
	CPU_6502_NAME(inx), CPU_6502_NAME(sbc_imm), CPU_6502_NAME(nop), CPU_6502_NAME(sbc_imm), // E8
	CPU_6502_NAME(cpx_abs), CPU_6502_NAME(sbc_abs), CPU_6502_NAME(inc_abs), CPU_6502_NAME(i_isc_abs), // EC
	CPU_6502_NAME(beq), CPU_6502_NAME(sbc_zpindy), CPU_6502_NAME(hlt), CPU_6502_NAME(i_isc_zpindy), // F0
	CPU_6502_NAME(nop_zpx), CPU_6502_NAME(sbc_zpx), CPU_6502_NAME(inc_zpx), CPU_6502_NAME(i_isc_zpx), // F4
	CPU_6502_NAME(sed), CPU_6502_NAME(sbc_absy), CPU_6502_NAME(nop), CPU_6502_NAME(i_isc_absy), // F8
	CPU_6502_NAME(nop_absx), CPU_6502_NAME(sbc_absx), CPU_6502_NAME(inc_absx), CPU_6502_NAME(i_isc_absx) // FC
};

static uint8_t CPU_6502_NAME(opcode_cycles)[] = {
	7, 6, 1, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6, // 00
	2, 5, 1, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, // 10
	6, 6, 1, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6, // 20
	2, 5, 1, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, // 30
	6, 6, 1, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6, // 40
	2, 5, 1, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, // 50
	6, 6, 1, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6, // 60
	2, 5, 1, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, // 70
	2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4, // 80
	2, 6, 1, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5, // 90
	2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4, // A0
	2, 5, 1, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4, // B0
	2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, // C0
	2, 5, 1, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, // D0
	2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, // E0
	2, 5, 1, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, // F0
};