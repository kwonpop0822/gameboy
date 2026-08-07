all: tycoon.gb

tycoon.gb: main.c
	lcc -Wa-l -Wp-lib -msm83:gb -o tycoon.gb main.c

clean:
	rm -f *.ihx *.sym *.asm *.lk *.lst *.map tycoon.gb
