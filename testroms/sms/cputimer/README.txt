Z80 Cycle Timing Tester
by Ben "GreaseMonkey" Russell, 2016

DRAFT README FILE LIKELY TO BE WRONG IN PLACES

WARNING: This has not been tested on real hardware.

TODO:
* Verify some of the IXH/IXL/IYH/IYL instruction timings.
  Cogwheel/BizHawk says 9. Absolutely everything else says 8.

REQUIREMENTS:

You must have a VDP that can do vblank interrupts.

For reference:
* PAL:  228*313 cycles per frame
* NTSC: 228*262 cycles per frame

HOW IT WORKS:

	ld hl, int_fired
	ld (hl), $FF
	halt
	---:
	; TEST CODE GOES HERE
	inc de ; 6
	bit 0, (hl) ; 12
	jp z, --- ; 10

Interrupt adds 1 to (int_fired) and polls the VDP control port.

Pretty simple really.

HOW TO READ THE RESULTS:

First hex column is the result.
The column after that gives you what it thinks is the result:
* "pal" means it's what it expects for a PAL system
* "ntsc" means it's what it expects for a NTSC system
* A 16-bit hex result tells you what it expects for a PAL system

There is a 3 loop tolerance either way.

