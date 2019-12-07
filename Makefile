#######################################################################################

# environment variable of the current user to locate the AVR8 toolchain
AVRPATH = $(AVR8TOOLCHAINBINDIR)

# the type of avr microcontroller
DEVICE = atmega8
EFUSE  = ""
HFUSE  = 0xd9
LFUSE  = 0xe1

# the frequency the microcontroller is clocked with
F_CPU = 16000000

# extra data section
  DEFINES += -DBOOT_SECTION_START=0x1800 -D__bootloaderconfig_h_included__
# DEFINES += -D__AVR_LIBC_DEPRECATED_ENABLE__
# DEFINES += -DDATASECTION=__attribute__\ \(\(section\ \(\".extradata\"\)\)\)
# LDFLAGS += -Wl,--section-start=.extradata=0x6000

# where the firmware should be located within the flashmemory (in case you trampoline)
FLASHADDRESS = 0x0000

# (not important for compiling) - the device transporting firmware into the controller
PROGRAMMER = -c usbasp

#######################################################################################



# Tools:
ECHO=@echo
GCC=gcc
MAKE=@make
RM=@rm -f

DOX=@doxygen

CC=$(AVRPATH)avr-gcc
OBC=@$(AVRPATH)avr-objcopy
OBD=@$(AVRPATH)avr-objdump
SIZ=@$(AVRPATH)avr-size

AVRDUDE = avrdude $(PROGRAMMER) -p $(DEVICE)
AVRDUDE_FUSE = -U lfuse:w:$(LFUSE):m -U hfuse:w:$(HFUSE):m
ifneq ($(EFUSE), "")
AVRDUDE_FUSE += -U efuse:w:$(EFUSE):m
endif


MYCFLAGS = -Wall -g3 -ggdb -Os -fno-move-loop-invariants -fno-tree-scev-cprop -fno-inline-small-functions -ffunction-sections -fdata-sections -I. -Isource -Ilibraries/API -Ilibraries/USBaspLoader/firmware -Ilibraries/avrlibs-baerwolf/include -mmcu=$(DEVICE) -DF_CPU=$(F_CPU) $(CFLAGS)   $(DEFINES)
MYLDFLAGS = -Wl,--relax,--gc-sections $(LDFLAGS)


FLASHPREAMBLEDEFINE = 
ifneq ($(FLASHADDRESS), 0)
ifneq ($(FLASHADDRESS), 00)
ifneq ($(FLASHADDRESS), 000)
ifneq ($(FLASHADDRESS), 0000)
ifneq ($(FLASHADDRESS), 00000)
ifneq ($(FLASHADDRESS), 0x0)
ifneq ($(FLASHADDRESS), 0x00)
ifneq ($(FLASHADDRESS), 0x000)
ifneq ($(FLASHADDRESS), 0x0000)
ifneq ($(FLASHADDRESS), 0x00000)
FLASHPREAMBLE = 0x0000
FLASHPREAMBLEDEFINE = -DFLASHPREAMBLE=$(FLASHPREAMBLE)
MYLDFLAGS += -Wl,--section-start=.text=$(FLASHADDRESS)
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif




STDDEP	 = *.h source/*.h
EXTRADEP = Makefile


all: release/main.hex release/eeprom.hex release/main.bin release/eeprom.bin release/main.asm build/main.asm


build/apipage.S: libraries/API/apipage.c $(STDDEP) $(EXTRADEP)
	$(CC) libraries/API/apipage.c -S -o build/apipage.S $(MYCFLAGS)

build/apipage.o: build/apipage.S $(STDDEP) $(EXTRADEP)
	$(CC) build/apipage.S -c -o build/apipage.o $(MYCFLAGS)


build/extfunc.S: libraries/avrlibs-baerwolf/source/extfunc.c $(STDDEP) $(EXTRADEP)
	$(CC) libraries/avrlibs-baerwolf/source/extfunc.c -S -o build/extfunc.S $(MYCFLAGS)

build/extfunc.o: build/extfunc.S $(STDDEP) $(EXTRADEP)
	$(CC) build/extfunc.S -c -o build/extfunc.o $(MYCFLAGS)

build/hwclock.S: libraries/avrlibs-baerwolf/source/hwclock.c $(STDDEP) $(EXTRADEP)
	$(CC) libraries/avrlibs-baerwolf/source/hwclock.c -S -o build/hwclock.S $(MYCFLAGS)

build/hwclock.o: build/hwclock.S $(STDDEP) $(EXTRADEP)
	$(CC) build/hwclock.S -c -o build/hwclock.o $(MYCFLAGS)


build/main.S: source/main.c $(STDDEP) $(EXTRADEP)
	$(CC) source/main.c -S -o build/main.S $(MYCFLAGS)

build/main.o: build/main.S $(STDDEP) $(EXTRADEP)
	$(CC) build/main.S -c -o build/main.o $(MYCFLAGS)





MYOBJECTS = build/main.o build/apipage.o build/extfunc.o build/hwclock.o
release/main.elf: $(MYOBJECTS) $(STDDEP) $(EXTRADEP)
	$(CC) $(MYOBJECTS) -o release/main.elf $(MYCFLAGS) -Wl,-Map,release/main.map $(MYLDFLAGS)
	$(ECHO) "."
	$(SIZ) release/main.elf
	$(ECHO) "."

release/main.asm: release/main.elf $(STDDEP) $(EXTRADEP)
	$(OBD) -d release/main.elf > release/main.asm

build/main.asm: release/main.elf $(STDDEP) $(EXTRADEP)
	$(OBD) -dS release/main.elf > build/main.asm

release/main.hex: release/main.elf $(STDDEP) $(EXTRADEP)
	$(OBC) -R .eeprom -R .fuse -R .lock -R .signature -O ihex release/main.elf release/main.hex

release/eeprom.hex: release/main.elf $(STDDEP) $(EXTRADEP)
	$(OBC) -j .eeprom -O ihex release/main.elf release/eeprom.hex

release/main.bin: release/main.elf $(STDDEP) $(EXTRADEP)
	$(OBC) -R .eeprom -R .fuse -R .lock -R .signature -O binary release/main.elf release/main.bin

release/eeprom.bin: release/main.elf $(STDDEP) $(EXTRADEP)
	$(OBC) -j .eeprom -O binary release/main.elf release/eeprom.bin

disasm: release/main.elf $(STDDEP) $(EXTRADEP)
	$(OBD) -d release/main.elf

fuse:
	$(ECHO) "."
	$(AVRDUDE) $(AVRDUDE_FUSE)
	$(ECHO) "."

flash: all
	$(ECHO) "."
	$(AVRDUDE) -U flash:w:release/main.hex:i
	$(ECHO) "."

eeprom: all
	$(ECHO) "."
	$(AVRDUDE) -D -U eeprom:w:release/eeprom.hex:i
	$(ECHO) "."

deepclean: clean
	$(RM) source/*~
	$(RM) *~

clean:
	$(RM) build/*
	$(RM) release/*
