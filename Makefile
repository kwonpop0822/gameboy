	CC = sdcc
	CFLAGS = -mgbz80 --no-std-crt0
	INCLUDES = -I$(PREFIX)/include -I$(PREFIX)/include/gb
	
	all: tycoon.gb
	
	main.rel: main.c
	$(CC) $(CFLAGS) $(INCLUDES) -c main.c -o main.rel
	
	tycoon.ihx: main.rel
	$(CC) $(CFLAGS) main.rel -o tycoon.ihx
	
	tycoon.gb: tycoon.ihx
	makebin -s 32768 tycoon.ihx tycoon.gb
	
	clean:
	rm -f *.rel *.ihx *.sym *.asm *.lk *.lst *.map tycoon.gb
