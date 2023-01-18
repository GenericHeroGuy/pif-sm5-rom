pif.sm5.rom: pif.sm5.asm
	@bass -strict -o $@ $^
	@sha256sum -c sha256sums.txt

CFLAGS = -Wall -Wextra -Wpedantic

cmodel: cmodel.o

cmodel.o: cmodel.c cmodel.h

clean:
	rm -f pif.sm5.rom cmodel cmodel.o
