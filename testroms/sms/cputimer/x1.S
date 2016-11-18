	;
	; X = 1
	;

	; Y=0
	TEST_BEG "LD B, B"
	ld b, b
	TEST_END 4
	TEST_BEG "LD B, C"
	ld b, c
	TEST_END 4
	TEST_BEG "LD B, D"
	ld b, d
	TEST_END 4
	TEST_BEG "LD B, E"
	ld b, e
	TEST_END 4
	TEST_BEG "LD B, H"
	ld b, h
	TEST_END 4
	TEST_BEG "LD B, IXH"
	ld b, ixh
	TEST_END 8
	TEST_BEG "LD B, IYH"
	ld b, iyh
	TEST_END 8
	TEST_BEG "LD B, L"
	ld b, l
	TEST_END 4
	TEST_BEG "LD B, IXL"
	ld b, ixl
	TEST_END 8
	TEST_BEG "LD B, IYL"
	ld b, iyl
	TEST_END 8
	TEST_BEG "LD B, (HL)"
	ld b, (hl)
	TEST_END 7
	TEST_BEG "LD B, (IX+0)"
	ld b, (ix+0)
	TEST_END 19
	TEST_BEG "LD B, (IY+0)"
	ld b, (iy+0)
	TEST_END 19
	TEST_BEG "LD B, A"
	ld b, a
	TEST_END 4

	; Y=1
	TEST_BEG "LD C, B"
	ld c, b
	TEST_END 4
	TEST_BEG "LD C, C"
	ld c, c
	TEST_END 4
	TEST_BEG "LD C, D"
	ld c, d
	TEST_END 4
	TEST_BEG "LD C, E"
	ld c, e
	TEST_END 4
	TEST_BEG "LD C, H"
	ld c, h
	TEST_END 4
	TEST_BEG "LD C, IXH"
	ld c, ixh
	TEST_END 8
	TEST_BEG "LD C, IYH"
	ld c, iyh
	TEST_END 8
	TEST_BEG "LD C, L"
	ld c, l
	TEST_END 4
	TEST_BEG "LD C, IXL"
	ld c, ixl
	TEST_END 8
	TEST_BEG "LD C, IYL"
	ld c, iyl
	TEST_END 8
	TEST_BEG "LD C, (HL)"
	ld c, (hl)
	TEST_END 7
	TEST_BEG "LD C, (IX+0)"
	ld c, (ix+0)
	TEST_END 19
	TEST_BEG "LD C, (IY+0)"
	ld c, (iy+0)
	TEST_END 19
	TEST_BEG "LD C, A"
	ld c, a
	TEST_END 4

	; Y=2
	TEST_BEG "EXX; LD D, B"
	exx
	ld d, b
	exx
	TEST_END 12
	TEST_BEG "EXX; LD D, C"
	exx
	ld d, c
	exx
	TEST_END 12
	TEST_BEG "EXX; LD D, D"
	exx
	ld d, d
	exx
	TEST_END 12
	TEST_BEG "EXX; LD D, E"
	exx
	ld d, e
	exx
	TEST_END 12
	TEST_BEG "EXX; LD D, H"
	exx
	ld d, h
	exx
	TEST_END 12
	TEST_BEG "EXX; LD D, IXH"
	exx
	ld d, ixh
	exx
	TEST_END 16
	TEST_BEG "EXX; LD D, IYH"
	exx
	ld d, iyh
	exx
	TEST_END 16
	TEST_BEG "EXX; LD D, L"
	exx
	ld d, l
	exx
	TEST_END 12
	TEST_BEG "EXX; LD D, IXL"
	exx
	ld d, ixl
	exx
	TEST_END 16
	TEST_BEG "EXX; LD D, IYL"
	exx
	ld d, iyl
	exx
	TEST_END 16
	TEST_BEG "EXX; LD D, (HL)"
	exx
	ld d, (hl)
	exx
	TEST_END 15
	TEST_BEG "EXX; LD D, (IX+0)"
	exx
	ld d, (ix+0)
	exx
	TEST_END 27
	TEST_BEG "EXX; LD D, (IY+0)"
	exx
	ld d, (iy+0)
	exx
	TEST_END 27
	TEST_BEG "EXX; LD D, A"
	exx
	ld d, a
	exx
	TEST_END 12

	; Y=3
	TEST_BEG "EXX; LD E, B"
	exx
	ld e, b
	exx
	TEST_END 12
	TEST_BEG "EXX; LD E, C"
	exx
	ld e, c
	exx
	TEST_END 12
	TEST_BEG "EXX; LD E, D"
	exx
	ld e, d
	exx
	TEST_END 12
	TEST_BEG "EXX; LD E, E"
	exx
	ld e, e
	exx
	TEST_END 12
	TEST_BEG "EXX; LD E, H"
	exx
	ld e, h
	exx
	TEST_END 12
	TEST_BEG "EXX; LD E, IXH"
	exx
	ld e, ixh
	exx
	TEST_END 16
	TEST_BEG "EXX; LD E, IYH"
	exx
	ld e, iyh
	exx
	TEST_END 16
	TEST_BEG "EXX; LD E, L"
	exx
	ld e, l
	exx
	TEST_END 12
	TEST_BEG "EXX; LD E, IXL"
	exx
	ld e, ixl
	exx
	TEST_END 16
	TEST_BEG "EXX; LD E, IYL"
	exx
	ld e, iyl
	exx
	TEST_END 16
	TEST_BEG "EXX; LD E, (HL)"
	exx
	ld e, (hl)
	exx
	TEST_END 15
	TEST_BEG "EXX; LD E, (IX+0)"
	exx
	ld e, (ix+0)
	exx
	TEST_END 27
	TEST_BEG "EXX; LD E, (IY+0)"
	exx
	ld e, (iy+0)
	exx
	TEST_END 27
	TEST_BEG "EXX; LD E, A"
	exx
	ld e, a
	exx
	TEST_END 12

	; Y=4
	TEST_BEG "EXX; LD H, B"
	exx
	ld h, b
	exx
	TEST_END 12
	TEST_BEG "LD IXH, B"
	ld ixh, b
	TEST_END 8
	TEST_BEG "LD IYH, B"
	ld iyh, b
	TEST_END 8
	TEST_BEG "EXX; LD H, C"
	exx
	ld h, c
	exx
	TEST_END 12
	TEST_BEG "LD IXH, C"
	ld ixh, c
	TEST_END 8
	TEST_BEG "LD IYH, C"
	ld iyh, c
	TEST_END 8
	TEST_BEG "EXX; LD H, D"
	exx
	ld h, d
	exx
	TEST_END 12
	TEST_BEG "LD IXH, D"
	ld ixh, d
	TEST_END 8
	TEST_BEG "LD IYH, D"
	ld iyh, d
	TEST_END 8
	TEST_BEG "EXX; LD H, E"
	exx
	ld h, e
	exx
	TEST_END 12
	TEST_BEG "LD IXH, E"
	ld ixh, e
	TEST_END 8
	TEST_BEG "LD IYH, E"
	ld iyh, e
	TEST_END 8
	TEST_BEG "EXX; LD H, H"
	exx
	ld h, h
	exx
	TEST_END 12
	TEST_BEG "LD IXH, IXH"
	ld ixh, ixh
	TEST_END 8
	TEST_BEG "LD IYH, IYH"
	ld iyh, iyh
	TEST_END 8
	TEST_BEG "EXX; LD H, L"
	exx
	ld h, l
	exx
	TEST_END 12
	TEST_BEG "LD IXH, IXL"
	ld ixh, ixl
	TEST_END 8
	TEST_BEG "LD IYH, IYL"
	ld iyh, iyl
	TEST_END 8
	TEST_BEG "EXX; LD H, (HL)"
	exx
	ld h, (hl)
	exx
	TEST_END 15
	TEST_BEG "EXX; LD H, (IX+0)"
	exx
	ld h, (ix+0)
	exx
	TEST_END 27
	TEST_BEG "EXX; LD H, (IY+0)"
	exx
	ld h, (iy+0)
	exx
	TEST_END 27
	TEST_BEG "EXX; LD H, A"
	exx
	ld h, a
	exx
	TEST_END 12
	TEST_BEG "LD IXH, A"
	ld ixh, a
	TEST_END 8
	TEST_BEG "LD IYH, A"
	ld iyh, a
	TEST_END 8

	; Y=5
	TEST_BEG "EXX; LD L, B"
	exx
	ld l, b
	exx
	TEST_END 12
	TEST_BEG "LD IXL, B"
	ld ixl, b
	TEST_END 8
	TEST_BEG "LD IYL, B"
	ld iyl, b
	TEST_END 8
	TEST_BEG "EXX; LD L, C"
	exx
	ld l, c
	exx
	TEST_END 12
	TEST_BEG "LD IXL, C"
	ld ixl, c
	TEST_END 8
	TEST_BEG "LD IYL, C"
	ld iyl, c
	TEST_END 8
	TEST_BEG "EXX; LD L, D"
	exx
	ld l, d
	exx
	TEST_END 12
	TEST_BEG "LD IXL, D"
	ld ixl, d
	TEST_END 8
	TEST_BEG "LD IYL, D"
	ld iyl, d
	TEST_END 8
	TEST_BEG "EXX; LD L, E"
	exx
	ld l, e
	exx
	TEST_END 12
	TEST_BEG "LD IXL, E"
	ld ixl, e
	TEST_END 8
	TEST_BEG "LD IYL, E"
	ld iyl, e
	TEST_END 8
	TEST_BEG "EXX; LD L, H"
	exx
	ld l, h
	exx
	TEST_END 12
	TEST_BEG "LD IXL, IXH"
	ld ixl, ixh
	TEST_END 8
	TEST_BEG "LD IYL, IYH"
	ld iyl, iyh
	TEST_END 8
	TEST_BEG "EXX; LD L, L"
	exx
	ld l, l
	exx
	TEST_END 12
	TEST_BEG "LD IXL, IXL"
	ld ixl, ixl
	TEST_END 8
	TEST_BEG "LD IYL, IYL"
	ld iyl, iyl
	TEST_END 8
	TEST_BEG "EXX; LD L, (HL)"
	exx
	ld l, (hl)
	exx
	TEST_END 15
	TEST_BEG "EXX; LD L, (IX+0)"
	exx
	ld l, (ix+0)
	exx
	TEST_END 27
	TEST_BEG "EXX; LD L, (IY+0)"
	exx
	ld l, (iy+0)
	exx
	TEST_END 27
	TEST_BEG "EXX; LD L, A"
	exx
	ld l, a
	exx
	TEST_END 12
	TEST_BEG "LD IXL, A"
	ld ixl, a
	TEST_END 8
	TEST_BEG "LD IYL, A"
	ld iyl, a
	TEST_END 8

	; Y=6
	TEST_BEG "EXX; HL; LD (HL), B"
	exx
	ld hl, scratch0
	ld (hl), b
	exx
	TEST_END 25
	TEST_BEG "IX; LD (IX+0), B"
	ld ix, scratch0
	ld (ix+0), b
	TEST_END 33
	TEST_BEG "IY; LD (IY+0), B"
	ld iy, scratch0
	ld (iy+0), b
	TEST_END 33
	TEST_BEG "EXX; HL; LD (HL), C"
	exx
	ld hl, scratch0
	ld (hl), c
	exx
	TEST_END 25
	TEST_BEG "IX; LD (IX+0), C"
	ld ix, scratch0
	ld (ix+0), c
	TEST_END 33
	TEST_BEG "IY; LD (IY+0), C"
	ld iy, scratch0
	ld (iy+0), c
	TEST_END 33
	TEST_BEG "EXX; HL; LD (HL), D"
	exx
	ld hl, scratch0
	ld (hl), d
	exx
	TEST_END 25
	TEST_BEG "IX; LD (IX+0), D"
	ld ix, scratch0
	ld (ix+0), d
	TEST_END 33
	TEST_BEG "IY; LD (IY+0), D"
	ld iy, scratch0
	ld (iy+0), d
	TEST_END 33
	TEST_BEG "EXX; HL; LD (HL), E"
	exx
	ld hl, scratch0
	ld (hl), e
	exx
	TEST_END 25
	TEST_BEG "IX; LD (IX+0), E"
	ld ix, scratch0
	ld (ix+0), e
	TEST_END 33
	TEST_BEG "IY; LD (IY+0), E"
	ld iy, scratch0
	ld (iy+0), e
	TEST_END 33
	TEST_BEG "EXX; HL; LD (HL), H"
	exx
	ld hl, scratch0
	ld (hl), h
	exx
	TEST_END 25
	TEST_BEG "IX; LD (IX+0), H"
	ld ix, scratch0
	ld (ix+0), h
	TEST_END 33
	TEST_BEG "IY; LD (IY+0), H"
	ld iy, scratch0
	ld (iy+0), h
	TEST_END 33
	TEST_BEG "EXX; HL; LD (HL), L"
	exx
	ld hl, scratch0
	ld (hl), l
	exx
	TEST_END 25
	TEST_BEG "IX; LD (IX+0), L"
	ld ix, scratch0
	ld (ix+0), l
	TEST_END 33
	TEST_BEG "IY; LD (IY+0), L"
	ld iy, scratch0
	ld (iy+0), l
	TEST_END 33
	; Skip HALT
	TEST_BEG "EXX; HL; LD (HL), A"
	exx
	ld hl, scratch0
	ld (hl), a
	exx
	TEST_END 25
	TEST_BEG "IX; LD (IX+0), A"
	ld ix, scratch0
	ld (ix+0), a
	TEST_END 33
	TEST_BEG "IY; LD (IY+0), A"
	ld iy, scratch0
	ld (iy+0), a
	TEST_END 33

	; Y=7
	TEST_BEG "LD A, B"
	ld a, b
	TEST_END 4
	TEST_BEG "LD A, C"
	ld a, c
	TEST_END 4
	TEST_BEG "LD A, D"
	ld a, d
	TEST_END 4
	TEST_BEG "LD A, E"
	ld a, e
	TEST_END 4
	TEST_BEG "LD A, H"
	ld a, h
	TEST_END 4
	TEST_BEG "LD A, IXH"
	ld a, ixh
	TEST_END 8
	TEST_BEG "LD A, IYH"
	ld a, iyh
	TEST_END 8
	TEST_BEG "LD A, L"
	ld a, l
	TEST_END 4
	TEST_BEG "LD A, IXL"
	ld a, ixl
	TEST_END 8
	TEST_BEG "LD A, IYL"
	ld a, iyl
	TEST_END 8
	TEST_BEG "LD A, (HL)"
	ld a, (hl)
	TEST_END 7
	TEST_BEG "LD A, (IX+0)"
	ld a, (ix+0)
	TEST_END 19
	TEST_BEG "LD A, (IY+0)"
	ld a, (iy+0)
	TEST_END 19
	TEST_BEG "LD A, A"
	ld a, a
	TEST_END 4

	call print_ln


