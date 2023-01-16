pif.sm5.rom: pif.sm5.asm
	@bass -strict -o $@ $^
	@sha256sum -c sha256sums.txt
