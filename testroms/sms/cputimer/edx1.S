	;
	; ED prefix
	; X = 1
	;

	; Z=0

	; Y=0
	TEST_BEG "LD C; IN B, (C)"
	ld c, $DC
	in b, (c)
	TEST_END 19
	; Y=1
	TEST_BEG "LD C; IN C, (C)"
	ld c, $DC
	in c, (c)
	TEST_END 19
	; Y=2
	TEST_BEG "EXX; LD C; IN D, (C)"
	exx
	ld c, $DC
	in d, (c)
	exx
	TEST_END 27
	; Y=3
	TEST_BEG "EXX; LD C; IN E, (C)"
	exx
	ld c, $DC
	in e, (c)
	exx
	TEST_END 27
	; Y=4
	TEST_BEG "EXX; LD C; IN H, (C)"
	exx
	ld c, $DC
	in h, (c)
	exx
	TEST_END 27
	; Y=5
	TEST_BEG "EXX; LD C; IN L, (C)"
	exx
	ld c, $DC
	in l, (c)
	exx
	TEST_END 27
	; Y=6
	TEST_BEG "LD C; IN (C)"
	ld c, $DC
	in (c)
	TEST_END 19
	; Y=7
	TEST_BEG "LD C; IN A, (C)"
	ld c, $DC
	in a, (c)
	TEST_END 19

	call print_ln

	; Z=1

	; Y=0
	TEST_BEG "LD C; OUT (C), B"
	ld c, $DC
	out (c), b
	TEST_END 19
	; Y=1
	TEST_BEG "LD C; OUT (C), C"
	ld c, $DC
	out (c), c
	TEST_END 19
	; Y=2
	TEST_BEG "EXX; LD C; OUT (C), D"
	exx
	ld c, $DC
	out (c), d
	exx
	TEST_END 27
	; Y=3
	TEST_BEG "EXX; LD C; OUT (C), E"
	exx
	ld c, $DC
	out (c), e
	exx
	TEST_END 27
	; Y=4
	TEST_BEG "EXX; LD C; OUT (C), H"
	exx
	ld c, $DC
	out (c), h
	exx
	TEST_END 27
	; Y=5
	TEST_BEG "EXX; LD C; OUT (C), L"
	exx
	ld c, $DC
	out (c), l
	exx
	TEST_END 27
	; Y=6
	TEST_BEG "LD C; OUT (C), 0"
	ld c, $DC
	out (c), 0
	TEST_END 19
	; Y=7
	TEST_BEG "LD C; OUT (C), A"
	ld c, $DC
	out (c), a
	TEST_END 19

	call print_ln

	; Z=2

	; Y=0
	TEST_BEG "EXX; SBC HL, BC"
	exx
	sbc hl, bc
	exx
	TEST_END 23
	; Y=1
	TEST_BEG "EXX; ADC HL, BC"
	exx
	adc hl, bc
	exx
	TEST_END 23
	; Y=2
	TEST_BEG "EXX; SBC HL, DE"
	exx
	sbc hl, de
	exx
	TEST_END 23
	; Y=3
	TEST_BEG "EXX; ADC HL, DE"
	exx
	adc hl, de
	exx
	TEST_END 23
	; Y=4
	TEST_BEG "EXX; SBC HL, HL"
	exx
	sbc hl, hl
	exx
	TEST_END 23
	; Y=5
	TEST_BEG "EXX; ADC HL, HL"
	exx
	adc hl, hl
	exx
	TEST_END 23
	; Y=6
	TEST_BEG "EXX; SBC HL, SP"
	exx
	sbc hl, sp
	exx
	TEST_END 23
	; Y=7
	TEST_BEG "EXX; ADC HL, SP"
	exx
	adc hl, sp
	exx
	TEST_END 23

	call print_ln

	; Z=3

	; Y=0
	TEST_BEG "LD (NN), BC"
	ld (scratch0), bc
	TEST_END 20
	; Y=1
	TEST_BEG "LD BC, (NN)"
	ld bc, (scratch0)
	TEST_END 20
	; Y=2
	TEST_BEG "LD (NN), DE"
	ld (scratch0), de
	TEST_END 20
	; Y=3
	TEST_BEG "EXX; LD DE, (NN)"
	exx
	ld de, (scratch0)
	exx
	TEST_END 28
	; Y=4
	TEST_BEG "ED-LD (NN), HL"
	.db $ED, $63
	.dw scratch0
	TEST_END 20
	; Y=5
	TEST_BEG "EXX; ED-LD HL, (NN)"
	exx
	.db $ED, $6B
	.dw scratch0
	exx
	TEST_END 28
	; Y=6
	TEST_BEG "LD (NN), SP"
	ld (scratch0), sp
	TEST_END 20
	; Y=7
	TEST_BEG "LD (NN),SP;LD SP,(NN)"
	ld (scratch0), sp
	ld sp, (scratch0)
	TEST_END 40

	call print_ln

	; Z=4

	; Y=0
	TEST_BEG "NEG"
	neg
	TEST_END 8
	; Onwards: duplicates

	call print_ln

	; Z=5

	; Y=0
	TEST_BEG "LDPUSH;RETN"
	ld bc, +
	push bc
	retn
	+:
	TEST_END 35
	; Y=1
	TEST_BEG "LDPUSH;RETI"
	ld bc, +
	push bc
	reti
	+:
	TEST_END 35
	; Onwards: duplicates

	call print_ln

	; Z=6
	; We'll only test IM 1 in this block.
	; Y=2
	TEST_BEG "IM 1"
	im 1
	TEST_END 8

	call print_ln

	; Z=7

	; Y=0
	TEST_BEG "LD I, A"
	ld i, a
	TEST_END 9
	; Y=1
	TEST_BEG "LD R, A"
	ld r, a
	TEST_END 9
	; Y=2
	TEST_BEG "LD A, I"
	ld a, i
	TEST_END 9
	; Y=3
	TEST_BEG "LD A, R"
	ld a, r
	TEST_END 9
	; Y=4
	TEST_BEG "EXX; LD HL; RRD"
	exx
	ld hl, scratch0
	rrd
	exx
	TEST_END 36
	; Y=5
	TEST_BEG "EXX; LD HL; RLD"
	exx
	ld hl, scratch0
	rld
	exx
	TEST_END 36
	; Onwards: undocumented no-ops

	call print_ln

