	;
	; X = 1
	;

	; Y=0
	TEST_BEG "LD B, B:"
	ld b, b
	TEST_END 4
	TEST_BEG "LD B, C:"
	ld b, c
	TEST_END 4
	TEST_BEG "LD B, D:"
	ld b, d
	TEST_END 4
	TEST_BEG "LD B, E:"
	ld b, e
	TEST_END 4
	TEST_BEG "LD B, H:"
	ld b, h
	TEST_END 4
	TEST_BEG "LD B, IXH:"
	ld b, ixh
	TEST_END 8
	TEST_BEG "LD B, IYH:"
	ld b, iyh
	TEST_END 8
	TEST_BEG "LD B, L:"
	ld b, l
	TEST_END 4
	TEST_BEG "LD B, IXL:"
	ld b, ixl
	TEST_END 8
	TEST_BEG "LD B, IYL:"
	ld b, iyl
	TEST_END 8
	TEST_BEG "LD B, (HL):"
	ld b, (hl)
	TEST_END 7
	TEST_BEG "LD B, (IX+0):"
	ld b, (ix+0)
	TEST_END 19
	TEST_BEG "LD B, (IY+0):"
	ld b, (iy+0)
	TEST_END 19
	TEST_BEG "LD B, A:"
	ld b, a
	TEST_END 4

	; Y=1
	TEST_BEG "LD C, B:"
	ld c, b
	TEST_END 4
	TEST_BEG "LD C, C:"
	ld c, c
	TEST_END 4
	TEST_BEG "LD C, D:"
	ld c, d
	TEST_END 4
	TEST_BEG "LD C, E:"
	ld c, e
	TEST_END 4
	TEST_BEG "LD C, H:"
	ld c, h
	TEST_END 4
	TEST_BEG "LD C, IXH:"
	ld c, ixh
	TEST_END 8
	TEST_BEG "LD C, IYH:"
	ld c, iyh
	TEST_END 8
	TEST_BEG "LD C, L:"
	ld c, l
	TEST_END 4
	TEST_BEG "LD C, IXL:"
	ld c, ixl
	TEST_END 8
	TEST_BEG "LD C, IYL:"
	ld c, iyl
	TEST_END 8
	TEST_BEG "LD C, (HL):"
	ld c, (hl)
	TEST_END 7
	TEST_BEG "LD C, (IX+0):"
	ld c, (ix+0)
	TEST_END 19
	TEST_BEG "LD C, (IY+0):"
	ld c, (iy+0)
	TEST_END 19
	TEST_BEG "LD C, A:"
	ld c, a
	TEST_END 4

	; Y=2
	TEST_BEG "EXX; LD D, B; EXX:"
	exx
	ld d, b
	exx
	TEST_END 12
	TEST_BEG "EXX; LD D, C; EXX:"
	exx
	ld d, c
	exx
	TEST_END 12
	TEST_BEG "EXX; LD D, D; EXX:"
	exx
	ld d, d
	exx
	TEST_END 12
	TEST_BEG "EXX; LD D, E; EXX:"
	exx
	ld d, e
	exx
	TEST_END 12
	TEST_BEG "EXX; LD D, H; EXX:"
	exx
	ld d, h
	exx
	TEST_END 12
	TEST_BEG "EXX; LD D, IXH; EXX:"
	exx
	ld d, ixh
	exx
	TEST_END 16
	TEST_BEG "EXX; LD D, IYH; EXX:"
	exx
	ld d, iyh
	exx
	TEST_END 16
	TEST_BEG "EXX; LD D, L; EXX:"
	exx
	ld d, l
	exx
	TEST_END 12
	TEST_BEG "EXX; LD D, IXL; EXX:"
	exx
	ld d, ixl
	exx
	TEST_END 16
	TEST_BEG "EXX; LD D, IYL; EXX:"
	exx
	ld d, iyl
	exx
	TEST_END 16
	TEST_BEG "EXX; LD D, (HL); EXX:"
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
	TEST_BEG "EXX; LD D, A; EXX:"
	exx
	ld d, a
	exx
	TEST_END 12

	; Y=3
	TEST_BEG "EXX; LD E, B; EXX:"
	exx
	ld e, b
	exx
	TEST_END 12
	TEST_BEG "EXX; LD E, C; EXX:"
	exx
	ld e, c
	exx
	TEST_END 12
	TEST_BEG "EXX; LD E, D; EXX:"
	exx
	ld e, d
	exx
	TEST_END 12
	TEST_BEG "EXX; LD E, E; EXX:"
	exx
	ld e, e
	exx
	TEST_END 12
	TEST_BEG "EXX; LD E, H; EXX:"
	exx
	ld e, h
	exx
	TEST_END 12
	TEST_BEG "EXX; LD E, IXH; EXX:"
	exx
	ld e, ixh
	exx
	TEST_END 16
	TEST_BEG "EXX; LD E, IYH; EXX:"
	exx
	ld e, iyh
	exx
	TEST_END 16
	TEST_BEG "EXX; LD E, L; EXX:"
	exx
	ld e, l
	exx
	TEST_END 12
	TEST_BEG "EXX; LD E, IXL; EXX:"
	exx
	ld e, ixl
	exx
	TEST_END 16
	TEST_BEG "EXX; LD E, IYL; EXX:"
	exx
	ld e, iyl
	exx
	TEST_END 16
	TEST_BEG "EXX; LD E, (HL); EXX:"
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
	TEST_BEG "EXX; LD E, A; EXX:"
	exx
	ld e, a
	exx
	TEST_END 12

	; Y=4
	TEST_BEG "EXX; LD H, B; EXX:"
	exx
	ld h, b
	exx
	TEST_END 12
	TEST_BEG "LD IXH, B:"
	ld ixh, b
	TEST_END 8
	TEST_BEG "LD IYH, B:"
	ld iyh, b
	TEST_END 8
	TEST_BEG "EXX; LD H, C; EXX:"
	exx
	ld h, c
	exx
	TEST_END 12
	TEST_BEG "LD IXH, C:"
	ld ixh, c
	TEST_END 8
	TEST_BEG "LD IYH, C:"
	ld iyh, c
	TEST_END 8
	TEST_BEG "EXX; LD H, D; EXX:"
	exx
	ld h, d
	exx
	TEST_END 12
	TEST_BEG "LD IXH, D:"
	ld ixh, d
	TEST_END 8
	TEST_BEG "LD IYH, D:"
	ld iyh, d
	TEST_END 8
	TEST_BEG "EXX; LD H, E; EXX:"
	exx
	ld h, e
	exx
	TEST_END 12
	TEST_BEG "LD IXH, E:"
	ld ixh, e
	TEST_END 8
	TEST_BEG "LD IYH, E:"
	ld iyh, e
	TEST_END 8
	TEST_BEG "EXX; LD H, H; EXX:"
	exx
	ld h, h
	exx
	TEST_END 12
	TEST_BEG "LD IXH, IXH:"
	ld ixh, ixh
	TEST_END 8
	TEST_BEG "LD IYH, IYH:"
	ld iyh, iyh
	TEST_END 8
	TEST_BEG "EXX; LD H, L; EXX:"
	exx
	ld h, l
	exx
	TEST_END 12
	TEST_BEG "LD IXH, IXL:"
	ld ixh, ixl
	TEST_END 8
	TEST_BEG "LD IYH, IYL:"
	ld iyh, iyl
	TEST_END 8
	TEST_BEG "EXX; LD H, (HL); EXX:"
	exx
	ld h, (hl)
	exx
	TEST_END 15
	TEST_BEG "EXX; LD H, (IX+0); EXX:"
	exx
	ld h, (ix+0)
	exx
	TEST_END 27
	TEST_BEG "EXX; LD H, (IY+0); EXX:"
	exx
	ld h, (iy+0)
	exx
	TEST_END 27
	TEST_BEG "EXX; LD H, A; EXX:"
	exx
	ld h, a
	exx
	TEST_END 12
	TEST_BEG "LD IXH, A:"
	ld ixh, a
	TEST_END 8
	TEST_BEG "LD IYH, A:"
	ld iyh, a
	TEST_END 8

	; Y=5
	TEST_BEG "EXX; LD L, B; EXX:"
	exx
	ld l, b
	exx
	TEST_END 12
	TEST_BEG "LD IXL, B:"
	ld ixl, b
	TEST_END 8
	TEST_BEG "LD IYL, B:"
	ld iyl, b
	TEST_END 8
	TEST_BEG "EXX; LD L, C; EXX:"
	exx
	ld l, c
	exx
	TEST_END 12
	TEST_BEG "LD IXL, C:"
	ld ixl, c
	TEST_END 8
	TEST_BEG "LD IYL, C:"
	ld iyl, c
	TEST_END 8
	TEST_BEG "EXX; LD L, D; EXX:"
	exx
	ld l, d
	exx
	TEST_END 12
	TEST_BEG "LD IXL, D:"
	ld ixl, d
	TEST_END 8
	TEST_BEG "LD IYL, D:"
	ld iyl, d
	TEST_END 8
	TEST_BEG "EXX; LD L, E; EXX:"
	exx
	ld l, e
	exx
	TEST_END 12
	TEST_BEG "LD IXL, E:"
	ld ixl, e
	TEST_END 8
	TEST_BEG "LD IYL, E:"
	ld iyl, e
	TEST_END 8
	TEST_BEG "EXX; LD L, H; EXX:"
	exx
	ld l, h
	exx
	TEST_END 12
	TEST_BEG "LD IXL, IXH:"
	ld ixl, ixh
	TEST_END 8
	TEST_BEG "LD IYL, IYH:"
	ld iyl, iyh
	TEST_END 8
	TEST_BEG "EXX; LD L, L; EXX:"
	exx
	ld l, l
	exx
	TEST_END 12
	TEST_BEG "LD IXL, IXL:"
	ld ixl, ixl
	TEST_END 8
	TEST_BEG "LD IYL, IYL:"
	ld iyl, iyl
	TEST_END 8
	TEST_BEG "EXX; LD L, (HL); EXX:"
	exx
	ld l, (hl)
	exx
	TEST_END 15
	TEST_BEG "EXX; LD L, (IX+0); EXX:"
	exx
	ld l, (ix+0)
	exx
	TEST_END 27
	TEST_BEG "EXX; LD L, (IY+0); EXX:"
	exx
	ld l, (iy+0)
	exx
	TEST_END 27
	TEST_BEG "EXX; LD L, A; EXX:"
	exx
	ld l, a
	exx
	TEST_END 12
	TEST_BEG "LD IXL, A:"
	ld ixl, a
	TEST_END 8
	TEST_BEG "LD IYL, A:"
	ld iyl, a
	TEST_END 8

	call print_ln


