CC := avr-gcc -c
CX := avr-g++ -c
LD := avr-gcc

CC_OPTS := -mmcu=atmega32u4  -gdwarf-2 -DF_CPU=16000000UL -Os  -funsigned-char -funsigned-bitfields -ffunction-sections -fpack-struct -fshort-enums  -Wall    

CX_OPTS := $(CC_OPTS) 

LD_OPTS :=  -mmcu=atmega32u4  -gdwarf-2 -DF_CPU=16000000UL -Os  -funsigned-char -funsigned-bitfields -ffunction-sections -fpack-struct -fshort-enums  -Wall  -Wl,--relax -Wl,--gc-sections

%-c.o: %.c
	$(CC) $(CC_OPTS) -o $@ $<

%-cpp.o: %.cpp
	$(CX) $(CX_OPTS) -o $@ $<

%.hex: %.elf
	avr-objcopy -O ihex -R .eeprom -R .fuse -R .lock -R .signature $^ $@

SRCS =	$(filter-out rawhid%,$(wildcard */*.c))
OBJS =	$(patsubst %.c,%-c.o,$(SRCS))

all:	$(OBJS)

clean:
	rm -f $(OBJS)



