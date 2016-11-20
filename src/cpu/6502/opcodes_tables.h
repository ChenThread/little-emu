static cpu_6502_opcode* cpu_6502_opcodes[] = {
	cpu_6502_brk, cpu_6502_ora_zpindx, cpu_6502_hlt, cpu_6502i_slo_zpindx, // 00
	cpu_6502_nop_zp, cpu_6502_ora_zp, cpu_6502_asl_zp, cpu_6502i_slo_zp, // 04
	cpu_6502_php, cpu_6502_ora_imm, cpu_6502_asl_ra, cpu_6502i_anc_imm, // 08
	cpu_6502_nop_abs, cpu_6502_ora_abs, cpu_6502_asl_abs, cpu_6502i_slo_abs, // 0C
	cpu_6502_bpl, cpu_6502_ora_zpindy, cpu_6502_hlt, cpu_6502i_slo_zpindy, // 10 
	cpu_6502_nop_zpx, cpu_6502_ora_zpx, cpu_6502_asl_zpx, cpu_6502i_slo_zpx, // 14
	cpu_6502_clc, cpu_6502_ora_absy, cpu_6502_nop, cpu_6502i_slo_absy, // 18
	cpu_6502_nop_absx, cpu_6502_ora_absx, cpu_6502_asl_absx, cpu_6502i_slo_absx, // 1C
	cpu_6502_jsr, cpu_6502_and_zpindx, cpu_6502_hlt, cpu_6502i_rla_zpindx, // 20
	cpu_6502_bit_zp, cpu_6502_and_zp, cpu_6502_rol_zp, cpu_6502i_rla_zp, // 24
	cpu_6502_plp, cpu_6502_and_imm, cpu_6502_rol_ra, cpu_6502i_anc_imm, // 28
	cpu_6502_bit_abs, cpu_6502_and_abs, cpu_6502_rol_abs, cpu_6502i_rla_abs, // 2C
	cpu_6502_bmi, cpu_6502_and_zpindy, cpu_6502_hlt, cpu_6502i_rla_zpindy, // 30
	cpu_6502_nop_zpx, cpu_6502_and_zpx, cpu_6502_rol_zpx, cpu_6502i_rla_zpx, // 34
	cpu_6502_sec, cpu_6502_and_absy, cpu_6502_nop, cpu_6502i_rla_absy, // 38
	cpu_6502_nop_absx, cpu_6502_and_absx, cpu_6502_rol_absx, cpu_6502i_rla_absx, // 3C
	cpu_6502_rti, cpu_6502_eor_zpindx, cpu_6502_hlt, cpu_6502i_sre_zpindx, // 40
	cpu_6502_nop_zp, cpu_6502_eor_zp, cpu_6502_lsr_zp, cpu_6502i_sre_zp, // 44
	cpu_6502_pha, cpu_6502_eor_imm, cpu_6502_lsr_ra, cpu_6502i_alr_imm, // 48
	cpu_6502_jmp_abs, cpu_6502_eor_abs, cpu_6502_lsr_abs, cpu_6502i_sre_abs, // 4C
	cpu_6502_bvc, cpu_6502_eor_zpindy, cpu_6502_hlt, cpu_6502i_sre_zpindy, // 50
	cpu_6502_nop_zpx, cpu_6502_eor_zpx, cpu_6502_lsr_zpx, cpu_6502i_sre_zpx, // 54
	cpu_6502_cli, cpu_6502_eor_absy, cpu_6502_nop, cpu_6502i_sre_absy, // 58
	cpu_6502_nop_absx, cpu_6502_eor_absx, cpu_6502_lsr_absx, cpu_6502i_sre_absx, // 5C
	cpu_6502_rts, cpu_6502_adc_zpindx, cpu_6502_hlt, cpu_6502i_rra_zpindx, // 60
	cpu_6502_nop_zp, cpu_6502_adc_zp, cpu_6502_ror_zp, cpu_6502i_rra_zp, // 64
	cpu_6502_pla, cpu_6502_adc_imm, cpu_6502_ror_ra, cpu_6502i_arr_imm, // 68
	cpu_6502_jmp_ind, cpu_6502_adc_abs, cpu_6502_ror_abs, cpu_6502i_rra_abs, // 6C
	cpu_6502_bvs, cpu_6502_adc_zpindy, cpu_6502_hlt, cpu_6502i_rra_zpindy, // 70
	cpu_6502_nop_zpx, cpu_6502_adc_zpx, cpu_6502_ror_zpx, cpu_6502i_rra_zpx, // 74
	cpu_6502_sei, cpu_6502_adc_absy, cpu_6502_nop, cpu_6502i_rra_absy, // 78
	cpu_6502_nop_absx, cpu_6502_adc_absx, cpu_6502_ror_absx, cpu_6502i_rra_absx, // 7C
	cpu_6502_nop_imm, cpu_6502_sta_zpindx, cpu_6502_nop_imm, cpu_6502i_sax_zpindx, // 80
	cpu_6502_sty_zp, cpu_6502_sta_zp, cpu_6502_stx_zp, cpu_6502i_sax_zp, // 84
	cpu_6502_dey, cpu_6502_nop_imm, cpu_6502_txa, cpu_6502i_xaa_imm, // 88
	cpu_6502_sty_abs, cpu_6502_sta_abs, cpu_6502_stx_abs, cpu_6502i_sax_abs, // 8C
	cpu_6502_bcc, cpu_6502_sta_zpindy, cpu_6502_hlt, cpu_6502i_ahx_zpindy, // 90
	cpu_6502_sty_zpx, cpu_6502_sta_zpx, cpu_6502_stx_zpy, cpu_6502i_sax_zpy, // 94
	cpu_6502_tya, cpu_6502_sta_absy, cpu_6502_txs, cpu_6502i_tas_absy, // 98
	cpu_6502i_shy_absx, cpu_6502_sta_absx, cpu_6502i_shx_absx, cpu_6502i_ahx_absy, // 9C
	cpu_6502_ldy_imm, cpu_6502_lda_zpindx, cpu_6502_ldx_imm, cpu_6502i_lax_zpindx, // A0
	cpu_6502_ldy_zp, cpu_6502_lda_zp, cpu_6502_ldx_zp, cpu_6502i_lax_zp, // A4
	cpu_6502_tay, cpu_6502_lda_imm, cpu_6502_tax, cpu_6502i_lax_imm, // A8
	cpu_6502_ldy_abs, cpu_6502_lda_abs, cpu_6502_ldx_abs, cpu_6502i_lax_abs, // AC
	cpu_6502_bcs, cpu_6502_lda_zpindy, cpu_6502_hlt, cpu_6502i_lax_zpindy, // B0
	cpu_6502_ldy_zpx, cpu_6502_lda_zpx, cpu_6502_ldx_zpy, cpu_6502i_lax_zpy, // B4
	cpu_6502_clv, cpu_6502_lda_absy, cpu_6502_tsx, cpu_6502i_las_absy, // B8
	cpu_6502_ldy_absx, cpu_6502_lda_absx, cpu_6502_ldx_absy, cpu_6502i_lax_absy, // BC
	cpu_6502_cpy_imm, cpu_6502_cmp_zpindx, cpu_6502_nop_imm, cpu_6502i_dcp_zpindx, // C0
	cpu_6502_cpy_zp, cpu_6502_cmp_zp, cpu_6502_dec_zp, cpu_6502i_dcp_zp, // C4
	cpu_6502_iny, cpu_6502_cmp_imm, cpu_6502_dex, cpu_6502i_axs_imm, // C8
	cpu_6502_cpy_abs, cpu_6502_cmp_abs, cpu_6502_dec_abs, cpu_6502i_dcp_abs, // CC
	cpu_6502_bne, cpu_6502_cmp_zpindy, cpu_6502_hlt, cpu_6502i_dcp_zpindy, // D0
	cpu_6502_nop_zpx, cpu_6502_cmp_zpx, cpu_6502_dec_zpx, cpu_6502i_dcp_zpx, // D4
	cpu_6502_cld, cpu_6502_cmp_absy, cpu_6502_nop, cpu_6502i_dcp_absy, // D8
	cpu_6502_nop_absx, cpu_6502_cmp_absx, cpu_6502_dec_absx, cpu_6502i_dcp_absx, // DC
	cpu_6502_cpx_imm, cpu_6502_sbc_zpindx, cpu_6502_nop_imm, cpu_6502i_isc_zpindx, // E0
	cpu_6502_cpx_zp, cpu_6502_sbc_zp, cpu_6502_inc_zp, cpu_6502i_isc_zp, // E4
	cpu_6502_inx, cpu_6502_sbc_imm, cpu_6502_nop, cpu_6502_sbc_imm, // E8
	cpu_6502_cpx_abs, cpu_6502_sbc_abs, cpu_6502_inc_abs, cpu_6502i_isc_abs, // EC
	cpu_6502_beq, cpu_6502_sbc_zpindy, cpu_6502_hlt, cpu_6502i_isc_zpindy, // F0
	cpu_6502_nop_zpx, cpu_6502_sbc_zpx, cpu_6502_inc_zpx, cpu_6502i_isc_zpx, // F4
	cpu_6502_sed, cpu_6502_sbc_absy, cpu_6502_nop, cpu_6502i_isc_absy, // F8
	cpu_6502_nop_absx, cpu_6502_sbc_absx, cpu_6502_inc_absx, cpu_6502i_isc_absx // FC
};

static uint8_t cpu_6502_opcode_cycles[] = {
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