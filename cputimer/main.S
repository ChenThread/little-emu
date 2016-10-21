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
	rst $10
	.db \1, 0
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

	ld hl, $10001+3-(1+((228*(313)-IRQ_WAIT+(\1)+6+8)/(28+(\1))))
	ld bc, $10001+3-(1+((228*(262)-IRQ_WAIT+(\1)+6+8)/(28+(\1))))
	ld sp, $DFF0
	rst $18
.endm

.org $0000
	di
	im 1
	ld sp, $DFF0
	jp start

.org $0010
	jp test_print_str
.org $0018
	jp test_finish

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

test_print_str:
	pop hl
	call print_str
	inc hl
	ld a, ':'
	call print_char
	push hl
	-:
		ld a, (text_xoffs)
		cp 23
		ret z
		ld a, $20
		call print_char
		jr -

test_finish:
	push hl
	pop ix
	push bc
	pop iy
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
	TEST_BEG "Base timing"
	TEST_END 0
	call print_ln

	; Ensure base test sanity
	; BIT 0, (HL)
	TEST_BEG "BIT 0, (HL)"
	bit 0, (hl)
	TEST_END 12
	; INC DE
	TEST_BEG "INC BC"
	inc bc
	TEST_END 6
	TEST_BEG "DEC BC"
	dec bc
	TEST_END 6
	TEST_BEG "INC/DEC BC"
	inc bc
	dec bc
	TEST_END 12
	TEST_BEG "INC/DEC DE"
	inc de
	dec de
	TEST_END 12
	; JP z, $nnnn
	TEST_BEG "JP Z"
	jp z, +
	+:
	TEST_END 10
	TEST_BEG "XOR A; JP Z"
	xor a
	jp z, +
	+:
	TEST_END 14
	TEST_BEG "XOR A; INC A; JP Z"
	xor a
	inc a
	jp z, +
	+:
	TEST_END 18

	call print_ln

	; Some supporting stuff
	TEST_BEG "PUSH DE; POP DE"
	push de
	pop de
	TEST_END 21
	TEST_BEG "PUSH DE; INC SP x2"
	push de
	inc sp
	inc sp
	TEST_END 23
	TEST_BEG "DEC SP x2; POP BC"
	dec sp
	dec sp
	pop bc
	TEST_END 22
	TEST_BEG "INC SP; DEC SP"
	inc sp
	dec sp
	TEST_END 12

	call print_ln

	; And now we take it from the top

	.include "x0.S"
	.include "x1.S"
	.include "x2.S"
	.include "x3.S"

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

