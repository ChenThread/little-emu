	;
	; ED prefix
	; X = 2
	;

	; Z=0

	; Y=4
	TEST_BEG "EXX; PRIMED LDI"
	exx
	ld hl, scratch0
	ld de, scratch0
	ldi
	exx
	TEST_END 44

	; Y=5
	TEST_BEG "EXX; PRIMED LDIR BC=1"
	exx
	ld hl, scratch0
	ld de, scratch0
	ld bc, 1
	ldir
	exx
	TEST_END 54
	TEST_BEG "EXX; PRIMED LDIR BC=2"
	exx
	ld hl, scratch0
	ld de, scratch0
	ld bc, 2
	ldir
	exx
	TEST_END 75

	; Y=6
	TEST_BEG "EXX; PRIMED LDD"
	exx
	ld hl, scratch0
	ld de, scratch0
	ldd
	exx
	TEST_END 44

	; Y=7
	TEST_BEG "EXX; PRIMED LDDR BC=1"
	exx
	ld hl, scratch0+1
	ld de, scratch0+1
	ld bc, 1
	lddr
	exx
	TEST_END 54
	TEST_BEG "EXX; PRIMED LDDR BC=2"
	exx
	ld hl, scratch0+1
	ld de, scratch0+1
	ld bc, 2
	lddr
	exx
	TEST_END 75

	; Z=1
	TEST_BEG "EXX; PRIMED CPI"
	exx
	ld hl, scratch0
	ld de, scratch0
	cpi
	exx
	TEST_END 44

	; Y=5
	TEST_BEG "EXX; PRIMED CPIR BC=1"
	exx
	ld hl, scratch0
	ld de, scratch0
	ld bc, 1
	cpir
	exx
	TEST_END 54
	TEST_BEG "EXX; PRIMED CPIR BC=2"
	exx
	ld hl, scratch0
	ld de, scratch0
	ld bc, 2
	ld (hl), a
	cpl
	cpir
	exx
	TEST_END 86

	; Y=6
	TEST_BEG "EXX; PRIMED CPD"
	exx
	ld hl, scratch0
	ld de, scratch0
	cpd
	exx
	TEST_END 44

	; Y=7
	TEST_BEG "EXX; PRIMED CPDR BC=1"
	exx
	ld hl, scratch0+1
	ld de, scratch0+1
	ld bc, 1
	cpdr
	exx
	TEST_END 54
	TEST_BEG "EXX; PRIMED CPDR BC=2"
	exx
	ld hl, scratch0+1
	ld de, scratch0+1
	ld bc, 2
	ld (hl), a
	cpl
	cpdr
	exx
	TEST_END 86

	call print_ln

	; Z=2

	; Y=4
	TEST_BEG "EXX; PRIMED INI"
	exx
	ld hl, scratch0
	ld c, $DC
	ini
	exx
	TEST_END 41

	; Y=5
	TEST_BEG "EXX; PRIMED INIR B=1"
	exx
	ld hl, scratch0
	ld bc, $01DC
	inir
	exx
	TEST_END 44
	TEST_BEG "EXX; PRIMED INIR B=2"
	exx
	ld hl, scratch0
	ld bc, $02DC
	inir
	exx
	TEST_END 65

	; Y=6
	TEST_BEG "EXX; PRIMED IND"
	exx
	ld hl, scratch0
	ld c, $DC
	ind
	exx
	TEST_END 41

	; Y=7
	TEST_BEG "EXX; PRIMED INDR B=1"
	exx
	ld hl, scratch0+1
	ld bc, $01DC
	indr
	exx
	TEST_END 44
	TEST_BEG "EXX; PRIMED INDR B=2"
	exx
	ld hl, scratch0+1
	ld bc, $02DC
	indr
	exx
	TEST_END 65

	call print_ln

	; Z=3

	; Y=4
	TEST_BEG "EXX; PRIMED OUTI"
	exx
	ld hl, scratch0
	ld c, $DC
	outi
	exx
	TEST_END 41

	; Y=5
	TEST_BEG "EXX; PRIMED OTIR B=1"
	exx
	ld hl, scratch0
	ld bc, $01DC
	otir
	exx
	TEST_END 44
	TEST_BEG "EXX; PRIMED OTIR B=2"
	exx
	ld hl, scratch0
	ld bc, $02DC
	otir
	exx
	TEST_END 65

	; Y=6
	TEST_BEG "EXX; PRIMED OUTD"
	exx
	ld hl, scratch0
	ld c, $DC
	outd
	exx
	TEST_END 41

	; Y=7
	TEST_BEG "EXX; PRIMED OTDR B=1"
	exx
	ld hl, scratch0+1
	ld bc, $01DC
	otdr
	exx
	TEST_END 44
	TEST_BEG "EXX; PRIMED OTDR B=2"
	exx
	ld hl, scratch0+1
	ld bc, $02DC
	otdr
	exx
	TEST_END 65

	call print_ln


