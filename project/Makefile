# makfile configuration
CPU             	= msp430g2553
CFLAGS          	= -mmcu=${CPU} -Os -I../h
LDFLAGS		= -L../lib -L/opt/ti/msp430_gcc/include/


#switch the compiler (for the internal make rules)
CC              = msp430-elf-gcc
AS              = msp430-elf-gcc -mmcu=${CPU} -c


all:pongmain.elf


#additional rules for files
pongmain.elf: ${COMMON_OBJECTS} pongmain.o wdt_handler.o buzzer.o
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $^ -lTimer -lLcd


load: pongmain.elf
	msp430loader.sh $^


clean:
	rm -f *.o *.elf
