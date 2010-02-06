CC=avr-gcc
CFLAGS=-g -Os -Wall -I../include -I../mixer -mcall-prologues -mmcu=atmega16 -DF_CPU="16000000UL"

OBJ2HEX=avr-objcopy 
TARGET=w2w

program: $(TARGET).hex 
	sudo avrdude -p m16 -P usb -c avrispmkII -Uflash:w:$(TARGET).hex -B 1.0

$(TARGET).hex: $(TARGET).obj
	$(OBJ2HEX) -R .eeprom -O ihex $< $@

$(TARGET).obj: $(TARGET).o 
	$(CC) $(CFLAGS) -o $@ -Wl,-Map,$(TARGET).map $(TARGET).o 

$debug.obj: debug.c 
	$(CC) $(CFLAGS) -o $@ debug.c

clean:
	rm -f *.hex *.obj *.o *.map
