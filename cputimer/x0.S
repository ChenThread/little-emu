	;
	; X = 0
	;

	; Z=0 (X=0)
	; Y=0
	TEST_BEG "NOP:"
	nop
	TEST_END 4
	; Y=1 cannot be tested
	; Y=2
	TEST_BEG "LD B, 2; DJNZ:"
	ld b, $02
	-: djnz -
	TEST_END 7+13+8
	TEST_BEG "LD B, 3; DJNZ:"
	ld b, $03
	-: djnz -
	TEST_END 7+13+13+8
	; Y=3
	TEST_BEG "JR:"
	jr +
	+:
	TEST_END 12
	; Y=4
	TEST_BEG "XOR A; JR NZ:"
	xor a
	jr nz,+
	+:
	TEST_END 11
	TEST_BEG "XOR A; INC A; JR NZ:"
	xor a
	inc a
	jr nz,+
	+:
	TEST_END 20
	; Y=5
	TEST_BEG "XOR A; JR Z:"
	xor a
	jr z,+
	+:
	TEST_END 16
	TEST_BEG "XOR A; INC A; JR Z:"
	xor a
	inc a
	jr z,+
	+:
	TEST_END 15
	; Y=6
	TEST_BEG "OR A; JR NC:"
	or a
	jr nc,+
	+:
	TEST_END 16
	TEST_BEG "SCF; JR NC:"
	scf
	jr nc,+
	+:
	TEST_END 11
	; Y=7
	TEST_BEG "OR A; JR C:"
	or a
	jr c,+
	+:
	TEST_END 11
	TEST_BEG "SCF; JR C:"
	scf
	jr c,+
	+:
	TEST_END 16

	call print_ln

	; Z=1 (X=0)
	; Y=0
	TEST_BEG "LD BC, NN:"
	ld bc, $1234
	TEST_END 10

	; Y=1
	TEST_BEG "ADD HL, BC; LD HL, NN:"
	add hl, bc
	ld hl, int_fired
	TEST_END 21
	TEST_BEG "EXX; ADD HL, BC; EXX:"
	exx
	add hl, bc
	exx
	TEST_END 19
	TEST_BEG "EXX; ADD IX, BC; EXX:"
	exx
	add ix, bc
	exx
	TEST_END 23
	TEST_BEG "EXX; ADD IY, BC; EXX:"
	exx
	add iy, bc
	exx
	TEST_END 23

	; Y=2
	TEST_BEG "PUSH; LD DE, NN; POP:"
	push de
	ld de, $1234
	pop de
	TEST_END 31
	TEST_BEG "EXX; LD DE, NN; EXX:"
	exx
	ld de, $1234
	exx
	TEST_END 18

	; Y=3
	TEST_BEG "ADD HL, DE; LD HL, NN:"
	add hl, de
	ld hl, int_fired
	TEST_END 21
	TEST_BEG "EXX; ADD HL, DE; EXX:"
	exx
	add hl, de
	exx
	TEST_END 19
	TEST_BEG "EXX; ADD IX, DE; EXX:"
	exx
	add ix, de
	exx
	TEST_END 23
	TEST_BEG "EXX; ADD IY, DE; EXX:"
	exx
	add iy, de
	exx
	TEST_END 23

	; Y=4
	TEST_BEG "LD HL, NN:"
	ld hl, int_fired
	TEST_END 10
	TEST_BEG "LD IX, NN:"
	ld ix, int_fired
	TEST_END 14
	TEST_BEG "LD IY, NN:"
	ld iy, int_fired
	TEST_END 14

	; Y=5
	TEST_BEG "ADD HL, HL; LD HL, NN:"
	add hl, de
	ld hl, int_fired
	TEST_END 21
	TEST_BEG "EXX; ADD HL, HL; EXX:"
	exx
	add hl, de
	exx
	TEST_END 19
	TEST_BEG "ADD IX, IX:"
	add ix, ix
	TEST_END 15
	TEST_BEG "ADD IY, IY:"
	add iy, iy
	TEST_END 15

	; Y=6
	TEST_BEG "LD SP, NN:"
	ld sp, $dff0
	TEST_END 10

	; Y=7
	TEST_BEG "ADD HL, SP; LD HL, NN:"
	add hl, sp
	ld hl, int_fired
	TEST_END 21
	TEST_BEG "EXX; ADD HL, SP; EXX:"
	exx
	add hl, sp
	exx
	TEST_END 19
	TEST_BEG "ADD IX, SP:"
	add ix, sp
	TEST_END 15
	TEST_BEG "ADD IY, SP:"
	add iy, sp
	TEST_END 15

	call print_ln

	; Z=2 (X=0)
	; Y=0
	TEST_BEG "LD BC, NN; LD (BC), A:"
	ld bc, scratch0
	ld (bc), a
	TEST_END 17
	; Y=1
	TEST_BEG "LD A, (BC):"
	ld a, (bc)
	TEST_END 7
	; Y=2
	TEST_BEG "EXX; LD DE; LD (DE), A:"
	exx
	ld de, scratch0
	ld (de), a
	exx
	TEST_END 25
	; Y=3
	TEST_BEG "LD A, (DE):"
	ld a, (de)
	TEST_END 7
	; Y=4
	TEST_BEG "LD (NN), HL:"
	ld (scratch0), hl
	TEST_END 16
	TEST_BEG "LD (NN), IX:"
	ld (scratch0), ix
	TEST_END 20
	TEST_BEG "LD (NN), IY:"
	ld (scratch0), iy
	TEST_END 20
	; Y=5
	TEST_BEG "LD HL, (NN); LD HL, NN:"
	ld hl, (scratch0)
	ld hl, int_fired
	TEST_END 26
	TEST_BEG "LD IX, (NN):"
	ld ix, (scratch0)
	TEST_END 20
	TEST_BEG "LD IY, (NN):"
	ld iy, (scratch0)
	TEST_END 20
	; Y=6
	TEST_BEG "LD (NN), A:"
	ld (scratch0), a
	TEST_END 13
	; Y=7
	TEST_BEG "LD A, (NN):"
	ld a, (scratch0)
	TEST_END 13

	call print_ln

	; Z=3 (X=0)
	; Y=0
	TEST_BEG "INC BC:"
	inc bc
	TEST_END 6
	; Y=1
	TEST_BEG "DEC BC:"
	dec bc
	TEST_END 6
	; Y=2
	TEST_BEG "EXX; INC DE; EXX:"
	exx
	inc de
	exx
	TEST_END 14
	; Y=3
	TEST_BEG "EXX; DEC DE; EXX:"
	exx
	dec de
	exx
	TEST_END 14
	; Y=4
	TEST_BEG "EXX; INC HL; EXX:"
	exx
	inc hl
	exx
	TEST_END 14
	TEST_BEG "INC IX:"
	inc ix
	TEST_END 10
	TEST_BEG "INC IY:"
	inc iy
	TEST_END 10
	; Y=5
	TEST_BEG "EXX; DEC HL; EXX:"
	exx
	dec hl
	exx
	TEST_END 14
	TEST_BEG "DEC IX:"
	dec ix
	TEST_END 10
	TEST_BEG "DEC IY:"
	dec iy
	TEST_END 10
	; Y=6
	TEST_BEG "INC SP; LD SP, NN:"
	inc sp
	ld sp, $dff0
	TEST_END 16
	; Y=7
	TEST_BEG "DEC SP; LD SP, NN:"
	dec sp
	ld sp, $dff0
	TEST_END 16

	call print_ln

	; Z=4 (X=0)
	; Y=0
	TEST_BEG "INC B:"
	inc b
	TEST_END 4
	; Y=1
	TEST_BEG "INC C:"
	inc c
	TEST_END 4
	; Y=2
	TEST_BEG "EXX; INC D; EXX        "
	exx
	inc d
	exx
	TEST_END 12
	; Y=3
	TEST_BEG "EXX; INC E; EXX        "
	exx
	inc e
	exx
	TEST_END 12
	; Y=4
	TEST_BEG "EXX; INC H; EXX        "
	exx
	inc h
	exx
	TEST_END 12
	TEST_BEG "INC IXH:"
	inc ixh
	TEST_END 8
	TEST_BEG "INC IYH:"
	inc iyh
	TEST_END 8
	; Y=5
	TEST_BEG "EXX; INC L; EXX        "
	exx
	inc l
	exx
	TEST_END 12
	TEST_BEG "INC IXL:"
	inc ixl
	TEST_END 8
	TEST_BEG "INC IYL:"
	inc iyl
	TEST_END 8
	; Y=6
	TEST_BEG "EXX; LD HL; INC (HL):"
	exx
	ld hl, scratch0
	inc (hl)
	exx
	TEST_END 29
	TEST_BEG "LD IX; INC (IX+0):"
	ld ix, scratch0
	inc (ix+0)
	TEST_END 37
	TEST_BEG "LD IY; INC (IY+0):"
	ld iy, scratch0
	inc (iy+0)
	TEST_END 37
	; Y=7
	TEST_BEG "INC A:"
	inc a
	TEST_END 4

	; Z=5 (X=0)
	; Y=0
	TEST_BEG "DEC B:"
	dec b
	TEST_END 4
	; Y=1
	TEST_BEG "DEC C:"
	dec c
	TEST_END 4
	; Y=2
	TEST_BEG "EXX; DEC D; EXX        "
	exx
	dec d
	exx
	TEST_END 12
	; Y=3
	TEST_BEG "EXX; DEC E; EXX        "
	exx
	dec e
	exx
	TEST_END 12
	; Y=4
	TEST_BEG "EXX; DEC H; EXX        "
	exx
	dec h
	exx
	TEST_END 12
	TEST_BEG "DEC IXH:"
	dec ixh
	TEST_END 8
	TEST_BEG "DEC IYH:"
	dec iyh
	TEST_END 8
	; Y=5
	TEST_BEG "EXX; DEC L; EXX        "
	exx
	dec l
	exx
	TEST_END 12
	TEST_BEG "DEC IXL:"
	dec ixl
	TEST_END 8
	TEST_BEG "DEC IYL:"
	dec iyl
	TEST_END 8
	; Y=6
	TEST_BEG "EXX; LD HL; DEC (HL):"
	exx
	ld hl, scratch0
	dec (hl)
	exx
	TEST_END 29
	TEST_BEG "LD IX; DEC (IX+0):"
	ld ix, scratch0
	dec (ix+0)
	TEST_END 37
	TEST_BEG "LD IY; DEC (IY+0):"
	ld iy, scratch0
	dec (iy+0)
	TEST_END 37
	; Y=7
	TEST_BEG "DEC A:"
	dec a
	TEST_END 4

	call print_ln

	; Z=6 (X=0)
	; Y=0
	TEST_BEG "LD B, N:"
	ld b,$39
	TEST_END 7
	; Y=1
	TEST_BEG "LD C, N:"
	ld c,$39
	TEST_END 7
	; Y=2
	TEST_BEG "EXX; LD D, N; EXX:"
	exx
	ld d,$39
	exx
	TEST_END 15
	; Y=3
	TEST_BEG "EXX; LD E, N; EXX:"
	exx
	ld e,$39
	exx
	TEST_END 15
	; Y=4
	TEST_BEG "LD H, N:"
	ld h,int_fired>>8
	TEST_END 7
	TEST_BEG "LD IXH, N:"
	ld ixh,$39
	TEST_END 11
	TEST_BEG "LD IYH, N:"
	ld iyh,$39
	TEST_END 11
	; Y=5
	TEST_BEG "LD L, N:"
	ld l,int_fired&$FF
	TEST_END 7
	TEST_BEG "LD IXL, N:"
	ld ixl,$39
	TEST_END 11
	TEST_BEG "LD IYL, N:"
	ld iyl,$39
	TEST_END 11
	; Y=6
	TEST_BEG "EXX; LD HL; LD (HL), N:"
	exx
	ld hl, scratch0
	ld (hl),$39
	exx
	TEST_END 28
	TEST_BEG "LD IX; LD (IX+0), N:"
	ld ix, scratch0
	ld (ix+0),$39
	TEST_END 33
	TEST_BEG "LD IY; LD (IY+0), N:"
	ld iy, scratch0
	ld (iy+0),$39
	TEST_END 33
	; Y=7
	TEST_BEG "LD A, N:"
	ld a,$39
	TEST_END 7

	call print_ln

	; Z=7 (X=0)
	; Y=0
	TEST_BEG "RLCA:"
	rlca
	TEST_END 4
	; Y=1
	TEST_BEG "RRCA:"
	rrca
	TEST_END 4
	; Y=2
	TEST_BEG "RLA:"
	rla
	TEST_END 4
	; Y=3
	TEST_BEG "RRA:"
	rra
	TEST_END 4
	; Y=4
	TEST_BEG "DAA:"
	daa
	TEST_END 4
	; Y=5
	TEST_BEG "CPL:"
	cpl
	TEST_END 4
	; Y=6
	TEST_BEG "SCF:"
	scf
	TEST_END 4
	; Y=7
	TEST_BEG "CCF:"
	ccf
	TEST_END 4

	call print_ln

