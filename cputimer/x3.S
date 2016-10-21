	;
	; X = 3
	;

	; Z=0
	; Y=0
	TEST_BEG "LDPUSH;XOR A;RET NZ"
	ld bc, +
	push bc
	xor a
	ret nz
	+:
	TEST_END 30
	TEST_BEG "LDPUSH;XOR;INC;RET NZ"
	ld bc, +
	push bc
	xor a
	inc a
	ret nz
	+:
	TEST_END 40

	; Y=1
	TEST_BEG "LDPUSH;XOR A;RET Z"
	ld bc, +
	push bc
	xor a
	ret z
	+:
	TEST_END 36
	TEST_BEG "LDPUSH;XOR;INC;RET Z"
	ld bc, +
	push bc
	xor a
	inc a
	ret z
	+:
	TEST_END 34

	; Y=2
	TEST_BEG "LDPUSH;OR A;RET NC"
	ld bc, +
	push bc
	or a
	ret nc
	+:
	TEST_END 36
	TEST_BEG "LDPUSH;SCF;RET NC"
	ld bc, +
	push bc
	scf
	ret nc
	+:
	TEST_END 30

	; Y=3
	TEST_BEG "LDPUSH;OR A;RET C"
	ld bc, +
	push bc
	or a
	ret c
	+:
	TEST_END 30
	TEST_BEG "LDPUSH;SCF;RET C"
	ld bc, +
	push bc
	scf
	ret c
	+:
	TEST_END 36

	; Y=4
	TEST_BEG "LDPUSH;XOR A;RET PO"
	ld bc, +
	push bc
	xor a
	ret po
	+:
	TEST_END 30
	TEST_BEG "LDPUSH;XOR;XOR1;RET PO"
	ld bc, +
	push bc
	xor a
	xor $01
	ret po
	+:
	TEST_END 43

	; Y=5
	TEST_BEG "LDPUSH;XOR A;RET PE"
	ld bc, +
	push bc
	xor a
	ret pe
	+:
	TEST_END 36
	TEST_BEG "LDPUSH;XOR;XOR1;RET PE"
	ld bc, +
	push bc
	xor a
	xor $01
	ret pe
	+:
	TEST_END 37

	; Y=6
	TEST_BEG "LDPUSH;XOR A;RET P"
	ld bc, +
	push bc
	xor a
	ret p
	+:
	TEST_END 36
	TEST_BEG "LDPUSH;XOR;DEC;RET P"
	ld bc, +
	push bc
	xor a
	dec a
	ret p
	+:
	TEST_END 34

	; Y=7
	TEST_BEG "LDPUSH;XOR A;RET M"
	ld bc, +
	push bc
	xor a
	ret m
	+:
	TEST_END 30
	TEST_BEG "LDPUSH;XOR;DEC;RET M"
	ld bc, +
	push bc
	xor a
	dec a
	ret m
	+:
	TEST_END 40

	call print_ln

	; Z=1

	; Y=0
	TEST_BEG "DEC SP x2; POP BC"
	dec sp
	dec sp
	pop bc
	TEST_END 22

	; Y=1
	TEST_BEG "LDPUSH;RET"
	ld bc, +
	push bc
	ret
	+:
	TEST_END 31

	; Y=2
	TEST_BEG "EXX; DEC SP x2; POP DE"
	exx
	dec sp
	dec sp
	pop de
	exx
	TEST_END 30

	; Y=3
	TEST_BEG "EXX; EXX"
	exx
	exx
	TEST_END 8

	; Y=4
	TEST_BEG "EXX; DEC SP x2; POP HL"
	exx
	dec sp
	dec sp
	pop hl
	exx
	TEST_END 30
	TEST_BEG "DEC SP x2; POP IX"
	dec sp
	dec sp
	pop ix
	TEST_END 26
	TEST_BEG "DEC SP x2; POP IY"
	dec sp
	dec sp
	pop iy
	TEST_END 26

	; Y=5
	TEST_BEG "EXX; LD HL; JP (HL)"
	exx
	ld hl, +
	jp (hl)
	+:
	exx
	TEST_END 22
	TEST_BEG "LD IX; JP (IX)"
	ld ix, +
	jp (ix)
	+:
	TEST_END 22
	TEST_BEG "LD IY; JP (IY)"
	ld iy, +
	jp (iy)
	+:
	TEST_END 22

	; Y=6
	TEST_BEG "DEC SP x2; POP AF"
	dec sp
	dec sp
	pop af
	TEST_END 22

	; Y=7
	TEST_BEG "EXX; LD HL; LD SP, HL"
	exx
	ld hl, $DFF0
	ld sp, hl
	exx
	TEST_END 24

	call print_ln

	; Z=2

	; Y=0
	TEST_BEG "XOR A;JP NZ"
	xor a
	jp nz, +
	+:
	TEST_END 14
	TEST_BEG "XOR;INC;JP NZ"
	xor a
	inc a
	jp nz, +
	+:
	TEST_END 18

	; Y=1
	TEST_BEG "XOR A;JP Z"
	xor a
	jp z, +
	+:
	TEST_END 14
	TEST_BEG "XOR;INC;JP Z"
	xor a
	inc a
	jp z, +
	+:
	TEST_END 18

	; Y=2
	TEST_BEG "OR A;JP NC"
	or a
	jp nc, +
	+:
	TEST_END 14
	TEST_BEG "SCF;JP NC"
	scf
	jp nc, +
	+:
	TEST_END 14

	; Y=3
	TEST_BEG "OR A;JP C"
	or a
	jp c, +
	+:
	TEST_END 14
	TEST_BEG "SCF;JP C"
	scf
	jp c, +
	+:
	TEST_END 14

	; Y=4
	TEST_BEG "XOR A;JP PO"
	xor a
	jp po, +
	+:
	TEST_END 14
	TEST_BEG "XOR;XOR1;JP PO"
	xor a
	xor $01
	jp po, +
	+:
	TEST_END 21

	; Y=5
	TEST_BEG "XOR A;JP PE"
	xor a
	jp pe, +
	+:
	TEST_END 14
	TEST_BEG "XOR;XOR1;JP PE"
	xor a
	xor $01
	jp pe, +
	+:
	TEST_END 21

	; Y=6
	TEST_BEG "XOR A;JP P"
	xor a
	jp p, +
	+:
	TEST_END 14
	TEST_BEG "XOR;DEC;JP P"
	xor a
	dec a
	jp p, +
	+:
	TEST_END 18

	; Y=7
	TEST_BEG "XOR A;JP M"
	xor a
	jp m, +
	+:
	TEST_END 14
	TEST_BEG "XOR;DEC;JP M"
	xor a
	dec a
	jp m, +
	+:
	TEST_END 18

	call print_ln

	; Z=3

	; Y=0
	TEST_BEG "JP"
	jp +
	+:
	TEST_END 10

	; Y=1 is CB, we skip that for a later test

	; Y=2
	TEST_BEG "OUT (N), A"
	out ($DC), a ; this ignores writes
	TEST_END 11

	; Y=3
	TEST_BEG "IN A, (N)"
	in a, ($DC) ; no side-effects from reading this
	TEST_END 11

	; Y=4
	TEST_BEG "EXX; EX (SP), HL"
	exx
	ex (sp), hl
	exx
	TEST_END 27

	; Y=5
	TEST_BEG "EXX; EX DE, HL"
	exx
	ex de, hl
	exx
	TEST_END 12

	; Y=6
	; Y=7
	TEST_BEG "DI; DI; EI"
	di
	di
	ei
	TEST_END 12
	TEST_BEG "DI; EI; EI"
	di
	ei
	ei
	TEST_END 12

	call print_ln

	; Z=4

	; Y=0
	TEST_BEG "XOR A;CALL NZ;LD SP"
	xor a
	call nz, +
	+: ld sp, $DFF0
	TEST_END 24
	TEST_BEG "XOR;INC;CALL NZ;LD SP"
	xor a
	inc a
	call nz, +
	+: ld sp, $DFF0
	TEST_END 35

	; Y=1
	TEST_BEG "XOR A;CALL Z;LD SP"
	xor a
	call z, +
	+: ld sp, $DFF0
	TEST_END 31
	TEST_BEG "XOR;INC;CALL Z;LD SP"
	xor a
	inc a
	call z, +
	+: ld sp, $DFF0
	TEST_END 28

	; Y=2
	TEST_BEG "OR A;CALL NC;LD SP"
	or a
	call nc, +
	+: ld sp, $DFF0
	TEST_END 31
	TEST_BEG "SCF;CALL NC;LD SP"
	scf
	call nc, +
	+: ld sp, $DFF0
	TEST_END 24

	; Y=3
	TEST_BEG "OR A;CALL C;LD SP"
	or a
	call c, +
	+: ld sp, $DFF0
	TEST_END 24
	TEST_BEG "SCF;CALL C;LD SP"
	scf
	call c, +
	+: ld sp, $DFF0
	TEST_END 31

	; Y=4
	TEST_BEG "XOR A;CALL PO;LD SP"
	xor a
	call po, +
	+: ld sp, $DFF0
	TEST_END 24
	TEST_BEG "XOR;XOR1;CALL PO;LD SP"
	xor a
	xor $01
	call po, +
	+: ld sp, $DFF0
	TEST_END 38

	; Y=5
	TEST_BEG "XOR A;CALL PE;LD SP"
	xor a
	call pe, +
	+: ld sp, $DFF0
	TEST_END 31
	TEST_BEG "XOR;XOR1;CALL PE;LD SP"
	xor a
	xor $01
	call pe, +
	+: ld sp, $DFF0
	TEST_END 31

	; Y=6
	TEST_BEG "XOR A;CALL P;LD SP"
	xor a
	call p, +
	+: ld sp, $DFF0
	TEST_END 31
	TEST_BEG "XOR;DEC;CALL P;LD SP"
	xor a
	dec a
	call p, +
	+: ld sp, $DFF0
	TEST_END 28

	; Y=7
	TEST_BEG "XOR A;CALL M;LD SP"
	xor a
	call m, +
	+: ld sp, $DFF0
	TEST_END 24
	TEST_BEG "XOR;DEC;CALL M;LD SP"
	xor a
	dec a
	call m, +
	+: ld sp, $DFF0
	TEST_END 35

	call print_ln

	; Z=5

	; Y=0
	TEST_BEG "PUSH BC; INC SP x2"
	push bc
	inc sp
	inc sp
	TEST_END 23

	; Y=1
	TEST_BEG "CALL; LD SP"
	call +
	+: ld sp, $DFF0
	TEST_END 27

	; Y=2
	TEST_BEG "PUSH DE; INC SP x2"
	push de
	inc sp
	inc sp
	TEST_END 23

	; Y=3 is DD - we integrate this with a bunch of tests

	; Y=4
	TEST_BEG "PUSH HL; INC SP x2"
	push hl
	inc sp
	inc sp
	TEST_END 23
	TEST_BEG "PUSH IX; INC SP x2"
	push ix
	inc sp
	inc sp
	TEST_END 27
	TEST_BEG "PUSH IY; INC SP x2"
	push iy
	inc sp
	inc sp
	TEST_END 27

	; Y=5 is ED - this is a separate set of tests

	; Y=6
	TEST_BEG "PUSH AF; INC SP x2"
	push af
	inc sp
	inc sp
	TEST_END 23

	; Y=7 is FD - we integrate this with a bunch of tests

	call print_ln

