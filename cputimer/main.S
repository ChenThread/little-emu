; SCANLINES is for the timing *defaults*
; PAL
.define SCANLINES 313
; NTSC
;.define SCANLINES 262

.define IRQ_WAIT 102

.rombankmap
bankstotal 1
banksize $8000
banks 1
.endro

.memorymap
slotsize $8000
slot 0 $0000
slotsize $2000
slot 1 $C000
defaultslot 0
.endme

.ramsection "rams" slot 1
	scratch0 dw
	irq_hl dw
	text_xoffs db
	text_yoffs db
	int_fired db
.ends

.macro TEST_BEG
	jr TEST_BEG_textend_\@
	TEST_BEG_text_\@: .db \1, 0
	TEST_BEG_textend_\@:
	ld hl, TEST_BEG_text_\@
	call print_str
	halt
	ld hl, int_fired
	ld de, $0000
	ld (hl), $FF
	halt
	-----:
.endm

.macro TEST_END
	inc de ; 6
	bit 0, (hl) ; 12
	jp z, ----- ; 10
	; 24 total

	ld ix, $10001+3-(1+((228*(313)-IRQ_WAIT+(\1)+6+8)/(28+(\1))))
	ld iy, $10001+3-(1+((228*(262)-IRQ_WAIT+(\1)+6+8)/(28+(\1))))
	call test_finish
.endm

.org $0000
	di
	im 1
	ld sp, $DFF0
	jp start

.org $0038
	; 13 intack
	ld (irq_hl), hl ; 16
	ex af, af' ; 4
	ld hl, int_fired ; 10
	inc (hl) ; 11
	in a, ($BF) ; 11
	ex af, af' ; 4
	ld hl, (irq_hl) ; 16
	ei ; 4
	reti ; 13
	; total: 102

.org $0066
	; DON'T PRESS THIS BUTTON DAMMIT
	retn

test_finish:
	ex de, hl
	push hl
	call print_hex16
	ld a, $20
	call print_char
	pop hl

	; Check PAL
	push hl
	push ix
	pop de
	add hl, de
	ld a, h
	and a
	jr nz, TEST_END_not_pal_FINISH
	ld a, l
	cp $06
	jr nc, TEST_END_not_pal_FINISH
		ld hl, str_pal
		call print_str
		;push ix
		;pop hl
		;call print_hex16
		pop hl
		jr TEST_END_detected_FINISH
	TEST_END_not_pal_FINISH:
	pop hl

	; Check NTSC
	push hl
	push iy
	pop de
	add hl, de
	ld a, h
	and a
	jr nz, TEST_END_not_ntsc_FINISH
	ld a, l
	cp $06
	jr nc, TEST_END_not_ntsc_FINISH
		ld hl, str_ntsc
		call print_str
		pop hl
		jr TEST_END_detected_FINISH
	TEST_END_not_ntsc_FINISH:
	pop hl

	; It's neither
	push ix
	pop de
	ld hl, $0000
	or a
	sbc hl, de
	call print_hex16

	; Wait
	ld b, 50
	-: halt
	djnz -

	TEST_END_detected_FINISH:
	call print_ln
	ret

start:
	; Wait for VBLANK
	-:
		in a, ($7E)
		cp $C1
		jr nz, -

	; Set up VDP regs
	in a, ($BF)
	ld hl, vdp_init_regs
	ld bc, $00BF + ((($0A+1)*2)<<8)
	otir

	; Clear VRAM
	ld a, $00
	out ($BF), a
	ld a, $40
	out ($BF), a
	ld a, $00
	ld b, $00
	-:
		.repeat $20
		out ($BE), a
		.endr
		djnz -
	-:
		.repeat $20
		out ($BE), a
		.endr
		djnz -

	; Load palette
	ld a, $00
	out ($BF), a
	ld a, $C0
	out ($BF), a
	ld hl, vdp_init_palette
	ld bc, $20BE
	otir

	; Load font
	ld a, $00
	out ($BF), a
	ld a, $40
	out ($BF), a
	ld b, $00
	ld hl, vdp_font
	-:
		.repeat 8
		ld a, (hl)
		inc hl
		out ($BE), a
		xor a
		out ($BE), a
		out ($BE), a
		out ($BE), a
		.endr
		djnz -

	; Enable screen + VINT
	ld a, $60
	out ($BF), a
	ld a, $81
	out ($BF), a

	; Set up console
	xor a
	ld (text_yoffs), a
	ld (text_xoffs), a

	; Draw string
	ld hl, str_hello
	call print_str
	call print_ln
	call print_ln
	ld hl, $0123
	call print_hex16
	ld hl, $4567
	call print_hex16
	ld hl, $89AB
	call print_hex16
	ld hl, $CDEF
	call print_hex16
	call print_ln

	; Wait for VINT
	in a, ($BF)
	ei

	; Base test
	TEST_BEG "Base timing:           "
	TEST_END 0
	call print_ln

	; Ensure base test sanity
	; BIT 0, (HL)
	TEST_BEG "BIT 0, (HL):           "
	bit 0, (hl)
	TEST_END 12
	; INC DE
	TEST_BEG "INC BC:                "
	inc bc
	TEST_END 6
	TEST_BEG "DEC BC:                "
	dec bc
	TEST_END 6
	TEST_BEG "INC/DEC BC:            "
	inc bc
	dec bc
	TEST_END 12
	TEST_BEG "INC/DEC DE:            "
	inc de
	dec de
	TEST_END 12
	; JP z, $nnnn
	TEST_BEG "JP Z:                  "
	jp z, +
	+:
	TEST_END 10
	TEST_BEG "XOR A; JP Z:           "
	xor a
	jp z, +
	+:
	TEST_END 14
	TEST_BEG "XOR A; INC A; JP Z:    "
	xor a
	inc a
	jp z, +
	+:
	TEST_END 18

	call print_ln

	; Some supporting stuff
	TEST_BEG "PUSH DE; POP DE:       "
	push de
	pop de
	TEST_END 21
	TEST_BEG "PUSH DE; INC SP x2:    "
	push de
	inc sp
	inc sp
	TEST_END 23
	TEST_BEG "DEC SP x2; POP BC:     "
	dec sp
	dec sp
	pop bc
	TEST_END 22
	TEST_BEG "INC SP; DEC SP:        "
	inc sp
	dec sp
	TEST_END 12

	call print_ln

	; And now we take it from the top

	;
	; X = 0
	;

	; Z=0 (X=0)
	; Y=0
	TEST_BEG "NOP:                   "
	nop
	TEST_END 4
	; Y=1 cannot be tested
	; Y=2
	TEST_BEG "LD B, 2; DJNZ:         "
	ld b, $02
	-: djnz -
	TEST_END 7+13+8
	TEST_BEG "LD B, 3; DJNZ:         "
	ld b, $03
	-: djnz -
	TEST_END 7+13+13+8
	; Y=3
	TEST_BEG "JR:                    "
	jr +
	+:
	TEST_END 12
	; Y=4
	TEST_BEG "XOR A; JR NZ:          "
	xor a
	jr nz,+
	+:
	TEST_END 11
	TEST_BEG "XOR A; INC A; JR NZ:   "
	xor a
	inc a
	jr nz,+
	+:
	TEST_END 20
	; Y=5
	TEST_BEG "XOR A; JR Z:           "
	xor a
	jr z,+
	+:
	TEST_END 16
	TEST_BEG "XOR A; INC A; JR Z:    "
	xor a
	inc a
	jr z,+
	+:
	TEST_END 15
	; Y=6
	TEST_BEG "OR A; JR NC:           "
	or a
	jr nc,+
	+:
	TEST_END 16
	TEST_BEG "SCF; JR NC:            "
	scf
	jr nc,+
	+:
	TEST_END 11
	; Y=7
	TEST_BEG "OR A; JR C:            "
	or a
	jr c,+
	+:
	TEST_END 11
	TEST_BEG "SCF; JR C:             "
	scf
	jr c,+
	+:
	TEST_END 16

	call print_ln

	; Z=1 (X=0)
	; Y=0
	TEST_BEG "LD BC, NN:             "
	ld bc, $1234
	TEST_END 10

	; Y=1
	TEST_BEG "ADD HL, BC; LD HL, NN: "
	add hl, bc
	ld hl, int_fired
	TEST_END 21
	TEST_BEG "EXX; ADD HL, BC; EXX:  "
	exx
	add hl, bc
	exx
	TEST_END 19
	TEST_BEG "EXX; ADD IX, BC; EXX:  "
	exx
	add ix, bc
	exx
	TEST_END 23
	TEST_BEG "EXX; ADD IY, BC; EXX:  "
	exx
	add iy, bc
	exx
	TEST_END 23

	; Y=2
	TEST_BEG "PUSH; LD DE, NN; POP:  "
	push de
	ld de, $1234
	pop de
	TEST_END 31
	TEST_BEG "EXX; LD DE, NN; EXX:   "
	exx
	ld de, $1234
	exx
	TEST_END 18

	; Y=3
	TEST_BEG "ADD HL, DE; LD HL, NN: "
	add hl, de
	ld hl, int_fired
	TEST_END 21
	TEST_BEG "EXX; ADD HL, DE; EXX:  "
	exx
	add hl, de
	exx
	TEST_END 19
	TEST_BEG "EXX; ADD IX, DE; EXX:  "
	exx
	add ix, de
	exx
	TEST_END 23
	TEST_BEG "EXX; ADD IY, DE; EXX:  "
	exx
	add iy, de
	exx
	TEST_END 23

	; Y=4
	TEST_BEG "LD HL, NN:             "
	ld hl, int_fired
	TEST_END 10
	TEST_BEG "LD IX, NN:             "
	ld ix, int_fired
	TEST_END 14
	TEST_BEG "LD IY, NN:             "
	ld iy, int_fired
	TEST_END 14

	; Y=5
	TEST_BEG "ADD HL, HL; LD HL, NN: "
	add hl, de
	ld hl, int_fired
	TEST_END 21
	TEST_BEG "EXX; ADD HL, HL; EXX:  "
	exx
	add hl, de
	exx
	TEST_END 19
	TEST_BEG "ADD IX, IX:            "
	add ix, ix
	TEST_END 15
	TEST_BEG "ADD IY, IY:            "
	add iy, iy
	TEST_END 15

	; Y=6
	TEST_BEG "LD SP, NN:             "
	ld sp, $dff0
	TEST_END 10

	; Y=7
	TEST_BEG "ADD HL, SP; LD HL, NN: "
	add hl, sp
	ld hl, int_fired
	TEST_END 21
	TEST_BEG "EXX; ADD HL, SP; EXX:  "
	exx
	add hl, sp
	exx
	TEST_END 19
	TEST_BEG "ADD IX, SP:            "
	add ix, sp
	TEST_END 15
	TEST_BEG "ADD IY, SP:            "
	add iy, sp
	TEST_END 15

	call print_ln

	; Z=2 (X=0)
	; Y=0
	TEST_BEG "LD BC, NN; LD (BC), A: "
	ld bc, scratch0
	ld (bc), a
	TEST_END 17
	; Y=1
	TEST_BEG "LD A, (BC):            "
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
	TEST_BEG "LD A, (DE):            "
	ld a, (de)
	TEST_END 7
	; Y=4
	TEST_BEG "LD (NN), HL:           "
	ld (scratch0), hl
	TEST_END 16
	TEST_BEG "LD (NN), IX:           "
	ld (scratch0), ix
	TEST_END 20
	TEST_BEG "LD (NN), IY:           "
	ld (scratch0), iy
	TEST_END 20
	; Y=5
	TEST_BEG "LD HL, (NN); LD HL, NN:"
	ld hl, (scratch0)
	ld hl, int_fired
	TEST_END 26
	TEST_BEG "LD IX, (NN):           "
	ld ix, (scratch0)
	TEST_END 20
	TEST_BEG "LD IY, (NN):           "
	ld iy, (scratch0)
	TEST_END 20
	; Y=6
	TEST_BEG "LD (NN), A:            "
	ld (scratch0), a
	TEST_END 13
	; Y=7
	TEST_BEG "LD A, (NN):            "
	ld a, (scratch0)
	TEST_END 13

	call print_ln

	; Z=3 (X=0)
	; Y=0
	TEST_BEG "INC BC:                "
	inc bc
	TEST_END 6
	; Y=1
	TEST_BEG "DEC BC:                "
	dec bc
	TEST_END 6
	; Y=2
	TEST_BEG "EXX; INC DE; EXX:      "
	exx
	inc de
	exx
	TEST_END 14
	; Y=3
	TEST_BEG "EXX; DEC DE; EXX:      "
	exx
	dec de
	exx
	TEST_END 14
	; Y=4
	TEST_BEG "EXX; INC HL; EXX:      "
	exx
	inc hl
	exx
	TEST_END 14
	TEST_BEG "INC IX:                "
	inc ix
	TEST_END 10
	TEST_BEG "INC IY:                "
	inc iy
	TEST_END 10
	; Y=5
	TEST_BEG "EXX; DEC HL; EXX:      "
	exx
	dec hl
	exx
	TEST_END 14
	TEST_BEG "DEC IX:                "
	dec ix
	TEST_END 10
	TEST_BEG "DEC IY:                "
	dec iy
	TEST_END 10
	; Y=6
	TEST_BEG "INC SP; LD SP, NN:     "
	inc sp
	ld sp, $dff0
	TEST_END 16
	; Y=7
	TEST_BEG "DEC SP; LD SP, NN:     "
	dec sp
	ld sp, $dff0
	TEST_END 16

	call print_ln

	; Z=4 (X=0)
	; Y=0
	TEST_BEG "INC B:                 "
	inc b
	TEST_END 4
	; Y=1
	TEST_BEG "INC C:                 "
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
	TEST_BEG "INC IXH:               "
	inc ixh
	TEST_END 8
	TEST_BEG "INC IYH:               "
	inc iyh
	TEST_END 8
	; Y=5
	TEST_BEG "EXX; INC L; EXX        "
	exx
	inc l
	exx
	TEST_END 12
	TEST_BEG "INC IXL:               "
	inc ixl
	TEST_END 8
	TEST_BEG "INC IYL:               "
	inc iyl
	TEST_END 8
	; Y=6
	TEST_BEG "EXX; LD HL; INC (HL):  "
	exx
	ld hl, scratch0
	inc (hl)
	exx
	TEST_END 29
	TEST_BEG "LD IX; INC (IX+0):     "
	ld ix, scratch0
	inc (ix+0)
	TEST_END 37
	TEST_BEG "LD IY; INC (IY+0):     "
	ld iy, scratch0
	inc (iy+0)
	TEST_END 37
	; Y=7
	TEST_BEG "INC A:                 "
	inc a
	TEST_END 4

	; Z=5 (X=0)
	; Y=0
	TEST_BEG "DEC B:                 "
	dec b
	TEST_END 4
	; Y=1
	TEST_BEG "DEC C:                 "
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
	TEST_BEG "DEC IXH:               "
	dec ixh
	TEST_END 8
	TEST_BEG "DEC IYH:               "
	dec iyh
	TEST_END 8
	; Y=5
	TEST_BEG "EXX; DEC L; EXX        "
	exx
	dec l
	exx
	TEST_END 12
	TEST_BEG "DEC IXL:               "
	dec ixl
	TEST_END 8
	TEST_BEG "DEC IYL:               "
	dec iyl
	TEST_END 8
	; Y=6
	TEST_BEG "EXX; LD HL; DEC (HL):  "
	exx
	ld hl, scratch0
	dec (hl)
	exx
	TEST_END 29
	TEST_BEG "LD IX; DEC (IX+0):     "
	ld ix, scratch0
	dec (ix+0)
	TEST_END 37
	TEST_BEG "LD IY; DEC (IY+0):     "
	ld iy, scratch0
	dec (iy+0)
	TEST_END 37
	; Y=7
	TEST_BEG "DEC A:                 "
	dec a
	TEST_END 4

	call print_ln

	; Z=6 (X=0)
	; Y=0
	TEST_BEG "LD B, N:               "
	ld b,$39
	TEST_END 7
	; Y=1
	TEST_BEG "LD C, N:               "
	ld c,$39
	TEST_END 7
	; Y=2
	TEST_BEG "EXX; LD D, N; EXX:     "
	exx
	ld d,$39
	exx
	TEST_END 15
	; Y=3
	TEST_BEG "EXX; LD E, N; EXX:     "
	exx
	ld e,$39
	exx
	TEST_END 15
	; Y=4
	TEST_BEG "LD H, N:               "
	ld h,int_fired>>8
	TEST_END 7
	TEST_BEG "LD IXH, N:             "
	ld ixh,$39
	TEST_END 11
	TEST_BEG "LD IYH, N:             "
	ld iyh,$39
	TEST_END 11
	; Y=5
	TEST_BEG "LD L, N:               "
	ld l,int_fired&$FF
	TEST_END 7
	TEST_BEG "LD IXL, N:             "
	ld ixl,$39
	TEST_END 11
	TEST_BEG "LD IYL, N:             "
	ld iyl,$39
	TEST_END 11
	; Y=6
	TEST_BEG "EXX; LD HL; LD (HL), N:"
	exx
	ld hl, scratch0
	ld (hl),$39
	exx
	TEST_END 28
	TEST_BEG "LD IX; LD (IX+0), N:   "
	ld ix, scratch0
	ld (ix+0),$39
	TEST_END 33
	TEST_BEG "LD IY; LD (IY+0), N:   "
	ld iy, scratch0
	ld (iy+0),$39
	TEST_END 33
	; Y=7
	TEST_BEG "LD A, N:               "
	ld a,$39
	TEST_END 7

	call print_ln

	; Z=7 (X=0)
	; Y=0
	TEST_BEG "RLCA:                  "
	rlca
	TEST_END 4
	; Y=1
	TEST_BEG "RRCA:                  "
	rrca
	TEST_END 4
	; Y=2
	TEST_BEG "RLA:                   "
	rla
	TEST_END 4
	; Y=3
	TEST_BEG "RRA:                   "
	rra
	TEST_END 4
	; Y=4
	TEST_BEG "DAA:                   "
	daa
	TEST_END 4
	; Y=5
	TEST_BEG "CPL:                   "
	cpl
	TEST_END 4
	; Y=6
	TEST_BEG "SCF:                   "
	scf
	TEST_END 4
	; Y=7
	TEST_BEG "CCF:                   "
	ccf
	TEST_END 4

	call print_ln

	;
	; X = 1
	;

	; Y=0
	TEST_BEG "LD B, B:               "
	ld b, b
	TEST_END 4
	TEST_BEG "LD B, C:               "
	ld b, c
	TEST_END 4
	TEST_BEG "LD B, D:               "
	ld b, d
	TEST_END 4
	TEST_BEG "LD B, E:               "
	ld b, e
	TEST_END 4
	TEST_BEG "LD B, H:               "
	ld b, h
	TEST_END 4
	TEST_BEG "LD B, IXH:             "
	ld b, ixh
	TEST_END 8
	TEST_BEG "LD B, IYH:             "
	ld b, iyh
	TEST_END 8
	TEST_BEG "LD B, L:               "
	ld b, l
	TEST_END 4
	TEST_BEG "LD B, IXL:             "
	ld b, ixl
	TEST_END 8
	TEST_BEG "LD B, IYL:             "
	ld b, iyl
	TEST_END 8
	TEST_BEG "LD B, (HL):            "
	ld b, (hl)
	TEST_END 7
	TEST_BEG "LD B, (IX+0):          "
	ld b, (ix+0)
	TEST_END 19
	TEST_BEG "LD B, (IY+0):          "
	ld b, (iy+0)
	TEST_END 19
	TEST_BEG "LD B, A:               "
	ld b, a
	TEST_END 4

	; Y=1
	TEST_BEG "LD C, B:               "
	ld c, b
	TEST_END 4
	TEST_BEG "LD C, C:               "
	ld c, c
	TEST_END 4
	TEST_BEG "LD C, D:               "
	ld c, d
	TEST_END 4
	TEST_BEG "LD C, E:               "
	ld c, e
	TEST_END 4
	TEST_BEG "LD C, H:               "
	ld c, h
	TEST_END 4
	TEST_BEG "LD C, IXH:             "
	ld c, ixh
	TEST_END 8
	TEST_BEG "LD C, IYH:             "
	ld c, iyh
	TEST_END 8
	TEST_BEG "LD C, L:               "
	ld c, l
	TEST_END 4
	TEST_BEG "LD C, IXL:             "
	ld c, ixl
	TEST_END 8
	TEST_BEG "LD C, IYL:             "
	ld c, iyl
	TEST_END 8
	TEST_BEG "LD C, (HL):            "
	ld c, (hl)
	TEST_END 7
	TEST_BEG "LD C, (IX+0):          "
	ld c, (ix+0)
	TEST_END 19
	TEST_BEG "LD C, (IY+0):          "
	ld c, (iy+0)
	TEST_END 19
	TEST_BEG "LD C, A:               "
	ld c, a
	TEST_END 4

	; Y=2
	TEST_BEG "EXX; LD D, B; EXX:     "
	exx
	ld d, b
	exx
	TEST_END 12
	TEST_BEG "EXX; LD D, C; EXX:     "
	exx
	ld d, c
	exx
	TEST_END 12
	TEST_BEG "EXX; LD D, D; EXX:     "
	exx
	ld d, d
	exx
	TEST_END 12
	TEST_BEG "EXX; LD D, E; EXX:     "
	exx
	ld d, e
	exx
	TEST_END 12
	TEST_BEG "EXX; LD D, H; EXX:     "
	exx
	ld d, h
	exx
	TEST_END 12
	TEST_BEG "EXX; LD D, IXH; EXX:   "
	exx
	ld d, ixh
	exx
	TEST_END 16
	TEST_BEG "EXX; LD D, IYH; EXX:   "
	exx
	ld d, iyh
	exx
	TEST_END 16
	TEST_BEG "EXX; LD D, L; EXX:     "
	exx
	ld d, l
	exx
	TEST_END 12
	TEST_BEG "EXX; LD D, IXL; EXX:   "
	exx
	ld d, ixl
	exx
	TEST_END 16
	TEST_BEG "EXX; LD D, IYL; EXX:   "
	exx
	ld d, iyl
	exx
	TEST_END 16
	TEST_BEG "EXX; LD D, (HL); EXX:  "
	exx
	ld d, (hl)
	exx
	TEST_END 15
	TEST_BEG "EXX; LD D, (IX+0); EXX:"
	exx
	ld d, (ix+0)
	exx
	TEST_END 27
	TEST_BEG "EXX; LD D, (IY+0); EXX:"
	exx
	ld d, (iy+0)
	exx
	TEST_END 27
	TEST_BEG "EXX; LD D, A; EXX:     "
	exx
	ld d, a
	exx
	TEST_END 12

	; Y=3
	TEST_BEG "EXX; LD E, B; EXX:     "
	exx
	ld e, b
	exx
	TEST_END 12
	TEST_BEG "EXX; LD E, C; EXX:     "
	exx
	ld e, c
	exx
	TEST_END 12
	TEST_BEG "EXX; LD E, D; EXX:     "
	exx
	ld e, d
	exx
	TEST_END 12
	TEST_BEG "EXX; LD E, E; EXX:     "
	exx
	ld e, e
	exx
	TEST_END 12
	TEST_BEG "EXX; LD E, H; EXX:     "
	exx
	ld e, h
	exx
	TEST_END 12
	TEST_BEG "EXX; LD E, IXH; EXX:   "
	exx
	ld e, ixh
	exx
	TEST_END 16
	TEST_BEG "EXX; LD E, IYH; EXX:   "
	exx
	ld e, iyh
	exx
	TEST_END 16
	TEST_BEG "EXX; LD E, L; EXX:     "
	exx
	ld e, l
	exx
	TEST_END 12
	TEST_BEG "EXX; LD E, IXL; EXX:   "
	exx
	ld e, ixl
	exx
	TEST_END 16
	TEST_BEG "EXX; LD E, IYL; EXX:   "
	exx
	ld e, iyl
	exx
	TEST_END 16
	TEST_BEG "EXX; LD E, (HL); EXX:  "
	exx
	ld e, (hl)
	exx
	TEST_END 15
	TEST_BEG "EXX; LD E, (IX+0); EXX:"
	exx
	ld e, (ix+0)
	exx
	TEST_END 27
	TEST_BEG "EXX; LD E, (IY+0); EXX:"
	exx
	ld e, (iy+0)
	exx
	TEST_END 27
	TEST_BEG "EXX; LD E, A; EXX:     "
	exx
	ld e, a
	exx
	TEST_END 12


	call print_ln

	;
	; X = 2
	;


	; TODO: the rest

	; DONE!
	call print_ln
	ld hl, str_done1
	call print_str
	call print_ln
	call print_ln
	-: jp -

str_done1: .db "*** ALL DONE! ***", 0

print_ln:
	push af
	push bc

	; Wait until X offset clear
	ld a, (text_xoffs)
	neg
	add a,$20
	jr z, +
	ld b, a
	-:
		ld a, $20
		call print_char
		djnz -
	+:

	; Clear X offset
	xor a
	ld (text_xoffs), a

	; Get Y offset
	ld a, (text_yoffs)
	inc a

	; Check for wrap
	cp 24
	jr c, +
		xor a
	+:

	; Write Y offset
	ld (text_yoffs), a

	pop bc
	pop af
	ret

	; HL = string
print_str:
-:
	; Read char
	ld a, (hl)
	and a
	ret z
	; Print and loop
	inc hl
	call print_char
	jp -

	; HL = hex number
print_hex16:
	push af
	push hl
	ld a, h
	call print_hex8
	ld a, l
	call print_hex8
	pop hl
	pop af
	ret

	; A = hex number
print_hex8:
	push af
	rrca
	rrca
	rrca
	rrca
	and $0F
	call print_hex4
	pop af
	push af
	and $0F
	call print_hex4
	pop af
	ret

	; A = hex number from $00 to $0F
print_hex4:
	push af

	; Make it a decimal digit
	add a, $30

	; If we overflow, make it a letter instead
	cp $3A
	jr c, +
		add a, $07
	+:

	; Print
	call print_char

	pop af
	ret

	; A = char
print_char:
	push hl

	; Save AF
	push af

	; Prep HL with Y offset
	; This will be our VDP command
	ld l, $00
	ld a, (text_yoffs)
	ld h, a
	srl h
	rr l
	srl h
	rr l

	; Add X offset
	ld a, (text_xoffs)
	add a, a
	add a, l
	ld l, a

	; Increment X offset
	ld a, (text_xoffs)
	inc a
	ld (text_xoffs), a

	; Apply upper bits so we know we're writing to the tilemap
	ld a, h
	or $78
	ld h, a

	; Write to control port
	ld a, l ; 4
	out ($BF), a ; 11
	ld a, ($0000) ; 13
	ld a, h ; 4
	out ($BF), a ; 11
	inc hl ; 6 BURNER

	; Restore AF
	pop af

	; Print char
	push af ; 11
	out ($BE), a ; 11
	ld a, ($0000) ; 13 BURNER
	xor a ; 4
	out ($BE), a ; 11
	pop af ; 10
	pop hl ; 10
	ret ; 10

vdp_init_regs:
	.dw $8004
	.dw $8100
	.dw $82FF
	.dw $83FF
	.dw $84FF
	.dw $85FF
	.dw $86FF
	.dw $8700
	.dw $8800
	.dw $8900
	.dw $8AFF

vdp_init_palette:
	.db $10, $3F, $00, $00
	.db $00, $00, $00, $00
	.db $00, $00, $00, $00
	.db $00, $00, $00, $00

	.db $30, $00, $00, $00
	.db $00, $00, $00, $00
	.db $00, $00, $00, $00
	.db $00, $00, $00, $00

vdp_font:
	.incbin "font.bin"

	;db "--------------------------------"
str_hello: .db "Z80 Cycle Timing Tester",0
str_bad: .db "*BAD",0
str_pal: .db "pal",0
str_ntsc: .db "ntsc",0

