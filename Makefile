roms: pif.sm5.ntsc.rom pif.sm5.pal.rom
	@sha256sum -c sha256sums.txt

pif.sm5.ntsc.rom: pif.sm5.asm
	@bass -strict -c region=0 -o $@ $^

pif.sm5.pal.rom: pif.sm5.asm
	@bass -strict -c region=1 -o $@ $^

CFLAGS = -g -Wall -Wextra -Wpedantic

cmodel: cmodel.o

cmodel.o: cmodel.c cmodel.h

cmodel_cic: cmodel_cic.o

cmodel_cic.o: cmodel_cic.c cmodel.h

run: cmodel
	./cmodel input.txt

run_cic: cmodel_cic
	./cmodel_cic input_cic.txt

clean:
	rm -f pif.sm5.ntsc.rom pif.sm5.pal.rom cmodel cmodel.o

-include user.mk
