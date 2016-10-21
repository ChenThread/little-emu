	;
	; X = 2
	;

	; Y=0
	TEST_BEG "ADD A, B"
	add a, b
	TEST_END 4
	TEST_BEG "ADD A, C"
	add a, c
	TEST_END 4
	TEST_BEG "ADD A, D"
	add a, d
	TEST_END 4
	TEST_BEG "ADD A, E"
	add a, e
	TEST_END 4
	TEST_BEG "ADD A, H"
	add a, h
	TEST_END 4
	TEST_BEG "ADD A, IXH"
	add a, ixh
	TEST_END 8
	TEST_BEG "ADD A, IYH"
	add a, iyh
	TEST_END 8
	TEST_BEG "ADD A, L"
	add a, l
	TEST_END 4
	TEST_BEG "ADD A, IXL"
	add a, ixl
	TEST_END 8
	TEST_BEG "ADD A, IYL"
	add a, iyl
	TEST_END 8
	TEST_BEG "ADD A, (HL)"
	add a, (hl)
	TEST_END 7
	TEST_BEG "ADD A, (IX+0)"
	add a, (ix+0)
	TEST_END 19
	TEST_BEG "ADD A, (IY+0)"
	add a, (iy+0)
	TEST_END 19
	TEST_BEG "ADD A, A"
	add a, a
	TEST_END 4

	; Y=1
	TEST_BEG "ADC A, B"
	adc a, b
	TEST_END 4
	TEST_BEG "ADC A, C"
	adc a, c
	TEST_END 4
	TEST_BEG "ADC A, D"
	adc a, d
	TEST_END 4
	TEST_BEG "ADC A, E"
	adc a, e
	TEST_END 4
	TEST_BEG "ADC A, H"
	adc a, h
	TEST_END 4
	TEST_BEG "ADC A, IXH"
	adc a, ixh
	TEST_END 8
	TEST_BEG "ADC A, IYH"
	adc a, iyh
	TEST_END 8
	TEST_BEG "ADC A, L"
	adc a, l
	TEST_END 4
	TEST_BEG "ADC A, IXL"
	adc a, ixl
	TEST_END 8
	TEST_BEG "ADC A, IYL"
	adc a, iyl
	TEST_END 8
	TEST_BEG "ADC A, (HL)"
	adc a, (hl)
	TEST_END 7
	TEST_BEG "ADC A, (IX+0)"
	adc a, (ix+0)
	TEST_END 19
	TEST_BEG "ADC A, (IY+0)"
	adc a, (iy+0)
	TEST_END 19
	TEST_BEG "ADC A, A"
	adc a, a
	TEST_END 4

	; Y=2
	TEST_BEG "SUB B"
	sub b
	TEST_END 4
	TEST_BEG "SUB C"
	sub c
	TEST_END 4
	TEST_BEG "SUB D"
	sub d
	TEST_END 4
	TEST_BEG "SUB E"
	sub e
	TEST_END 4
	TEST_BEG "SUB H"
	sub h
	TEST_END 4
	TEST_BEG "SUB IXH"
	;sub ixh ; WLA-DX has a bug here!
	.db $dd
	sub h
	TEST_END 8
	TEST_BEG "SUB IYH"
	sub iyh
	TEST_END 8
	TEST_BEG "SUB L"
	sub l
	TEST_END 4
	TEST_BEG "SUB IXL"
	;sub ixl ; WLA-DX has a bug here!
	.db $dd
	sub l
	TEST_END 8
	TEST_BEG "SUB IYL"
	sub iyl
	TEST_END 8
	TEST_BEG "SUB (HL)"
	sub (hl)
	TEST_END 7
	TEST_BEG "SUB (IX+0)"
	sub (ix+0)
	TEST_END 19
	TEST_BEG "SUB (IY+0)"
	sub (iy+0)
	TEST_END 19
	TEST_BEG "SUB A"
	sub a
	TEST_END 4

	; Y=3
	TEST_BEG "SBC A, B"
	sbc a, b
	TEST_END 4
	TEST_BEG "SBC A, C"
	sbc a, c
	TEST_END 4
	TEST_BEG "SBC A, D"
	sbc a, d
	TEST_END 4
	TEST_BEG "SBC A, E"
	sbc a, e
	TEST_END 4
	TEST_BEG "SBC A, H"
	sbc a, h
	TEST_END 4
	TEST_BEG "SBC A, IXH"
	sbc a, ixh
	TEST_END 8
	TEST_BEG "SBC A, IYH"
	sbc a, iyh
	TEST_END 8
	TEST_BEG "SBC A, L"
	sbc a, l
	TEST_END 4
	TEST_BEG "SBC A, IXL"
	sbc a, ixl
	TEST_END 8
	TEST_BEG "SBC A, IYL"
	sbc a, iyl
	TEST_END 8
	TEST_BEG "SBC A, (HL)"
	sbc a, (hl)
	TEST_END 7
	TEST_BEG "SBC A, (IX+0)"
	sbc a, (ix+0)
	TEST_END 19
	TEST_BEG "SBC A, (IY+0)"
	sbc a, (iy+0)
	TEST_END 19
	TEST_BEG "SBC A, A"
	sbc a, a
	TEST_END 4

	; Y=4
	TEST_BEG "AND B"
	and b
	TEST_END 4
	TEST_BEG "AND C"
	and c
	TEST_END 4
	TEST_BEG "AND D"
	and d
	TEST_END 4
	TEST_BEG "AND E"
	and e
	TEST_END 4
	TEST_BEG "AND H"
	and h
	TEST_END 4
	TEST_BEG "AND IXH"
	and ixh
	TEST_END 8
	TEST_BEG "AND IYH"
	and iyh
	TEST_END 8
	TEST_BEG "AND L"
	and l
	TEST_END 4
	TEST_BEG "AND IXL"
	and ixl
	TEST_END 8
	TEST_BEG "AND IYL"
	and iyl
	TEST_END 8
	TEST_BEG "AND (HL)"
	and (hl)
	TEST_END 7
	TEST_BEG "AND (IX+0)"
	and (ix+0)
	TEST_END 19
	TEST_BEG "AND (IY+0)"
	and (iy+0)
	TEST_END 19
	TEST_BEG "AND A"
	and a
	TEST_END 4

	; Y=5
	TEST_BEG "XOR B"
	xor b
	TEST_END 4
	TEST_BEG "XOR C"
	xor c
	TEST_END 4
	TEST_BEG "XOR D"
	xor d
	TEST_END 4
	TEST_BEG "XOR E"
	xor e
	TEST_END 4
	TEST_BEG "XOR H"
	xor h
	TEST_END 4
	TEST_BEG "XOR IXH"
	xor ixh
	TEST_END 8
	TEST_BEG "XOR IYH"
	xor iyh
	TEST_END 8
	TEST_BEG "XOR L"
	xor l
	TEST_END 4
	TEST_BEG "XOR IXL"
	xor ixl
	TEST_END 8
	TEST_BEG "XOR IYL"
	xor iyl
	TEST_END 8
	TEST_BEG "XOR (HL)"
	xor (hl)
	TEST_END 7
	TEST_BEG "XOR (IX+0)"
	xor (ix+0)
	TEST_END 19
	TEST_BEG "XOR (IY+0)"
	xor (iy+0)
	TEST_END 19
	TEST_BEG "XOR A"
	xor a
	TEST_END 4

	; Y=6
	TEST_BEG "OR B"
	or b
	TEST_END 4
	TEST_BEG "OR C"
	or c
	TEST_END 4
	TEST_BEG "OR D"
	or d
	TEST_END 4
	TEST_BEG "OR E"
	or e
	TEST_END 4
	TEST_BEG "OR H"
	or h
	TEST_END 4
	TEST_BEG "OR IXH"
	or ixh
	TEST_END 8
	TEST_BEG "OR IYH"
	or iyh
	TEST_END 8
	TEST_BEG "OR L"
	or l
	TEST_END 4
	TEST_BEG "OR IXL"
	or ixl
	TEST_END 8
	TEST_BEG "OR IYL"
	or iyl
	TEST_END 8
	TEST_BEG "OR (HL)"
	or (hl)
	TEST_END 7
	TEST_BEG "OR (IX+0)"
	or (ix+0)
	TEST_END 19
	TEST_BEG "OR (IY+0)"
	or (iy+0)
	TEST_END 19
	TEST_BEG "OR A"
	or a
	TEST_END 4

	; Y=7
	TEST_BEG "CP B"
	cp b
	TEST_END 4
	TEST_BEG "CP C"
	cp c
	TEST_END 4
	TEST_BEG "CP D"
	cp d
	TEST_END 4
	TEST_BEG "CP E"
	cp e
	TEST_END 4
	TEST_BEG "CP H"
	cp h
	TEST_END 4
	TEST_BEG "CP IXH"
	cp ixh
	TEST_END 8
	TEST_BEG "CP IYH"
	cp iyh
	TEST_END 8
	TEST_BEG "CP L"
	cp l
	TEST_END 4
	TEST_BEG "CP IXL"
	cp ixl
	TEST_END 8
	TEST_BEG "CP IYL"
	cp iyl
	TEST_END 8
	TEST_BEG "CP (HL)"
	cp (hl)
	TEST_END 7
	TEST_BEG "CP (IX+0)"
	cp (ix+0)
	TEST_END 19
	TEST_BEG "CP (IY+0)"
	cp (iy+0)
	TEST_END 19
	TEST_BEG "CP A"
	cp a
	TEST_END 4

	call print_ln

