CFILES=L3SubBand.c L3bitstrea.c L3loop.c L3mdct.c Layer3.c Main.c bitstream.c formatbits.c huffman.c reservoir.c tables.c wave.c arm_muls.c

all: shine

shine: $(CFILES)
	$(CC) $(CFILES) -I. -o shine -lm
