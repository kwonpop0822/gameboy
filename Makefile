CC = sdcc
CFLAGS = -mgbz80

all: tycoon.gb

tycoon.ihx: main.c
	$(CC) $(CFLAGS) main.c -o tycoon.ihx

tycoon.gb: tycoon.ihx
	makebin -yp tycoon.ihx tycoon.gb

clean:
	rm -f *.rel *.ihx *.sym *.asm *.lk *.lst *.map tycoon.gb
