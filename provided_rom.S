	.section ".text"
	.global rom_buffer
	.global rom_buffer_end
rom_buffer:
	.incbin PROVIDED_ROM
rom_buffer_end:

