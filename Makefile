#######################################################################################

# environment variable of the current user to locate the AVR8 toolchain
AVRPATH = $(AVR8TOOLCHAINBINDIR)

# the type of avr microcontroller
DEVICE = atmega8
# the frequency the microcontroller is clocked with
F_CPU = 16000000

EFUSE  = ""
HFUSE  = 0xd9
LFUSE  = 0xe1


# extra data section
# DEFINES += -DDEBUGSTACK=0xA5 -DMAINENDCYCLES=3750
# DEFINES += -DEXTRAPULLUP=D,5
# DEFINES += -DLED_DEBUG=D,3
# DEFINES += -DLED_RED=B,0
  DEFINES += -DLED_RED=B,3 -DOC2PWM_RED=32
  DEFINES += -DHIDMESSAGE=\"USB\ BuzzerBox\\nhttps://github.com/baerwolf/usbbuzzerbox\\n\\nStephan\ Baerwolf\ wuenscht\ ein\ Frohes\ Weihnachtsfest\ 2019\\nund\ einen\ guten\ Rutsch\ ins\ Jahr\ 2020.\\n\\nImmer\ schoen\ den\ Rechner\ beim\ Verlassen\ des\ Platzes\ sperren...\\n\\nStephan\ Baerwolf\ -\ stephan@matrixstorm.com\ ,\\nSchwansee\ im\ Dezember\ 2019\" -DHIDMESSAGETIME=10
  DEFINES += -DUSB_CFG_HID_NOMOUSE
  DEFINES += -DASCIIMAP_LAYOUT=ASCIIMAP_LAYOUT_DE
  DEFINES += -DBOOT_SECTION_START=0x1800 -D__bootloaderconfig_h_included__
  DEFINES += -DVUSB_CFG_IOPORTNAME=D -DVUSB_CFG_DMINUS_BIT=7 -DVUSB_CFG_DPLUS_BIT=2
  DEFINES += -DVUSB_CFG_HASNO_PULLUP_IOPORTNAME -DVUSB_CFG_HASNO_PULLUP_BIT

  DEFINES += -DEXTFUNC_NOEXT=0
# DEFINES += -DCPUCONTEXT_EXTRASYMBOLS=1
# DEFINES += -DEXTFUNCFAR=__attribute__\ \(\(section\ \(\".farfunc\"\)\)\) -Wl,--section-start=.farfunc=0x1300
# DEFINES += -DEXTFUNCNEAR=PROGMEM
# DEFINES += -DEXTFUNCNEAR=__attribute__\ \(\(section\ \(\".nearfunc\"\)\)\) -Wl,--section-start=.nearfunc=0x1000

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


MYCFLAGS = -Wall -g3 -ggdb -Os -fno-move-loop-invariants -fno-tree-scev-cprop -fno-inline-small-functions -ffunction-sections -fdata-sections -I. -Isource -Ilibraries/API -Ilibraries/USBaspLoader/firmware -Ilibraries/avrlibs-baerwolf/include -Ilibraries/v-usb/usbdrv -mmcu=$(DEVICE) -DF_CPU=$(F_CPU) $(CFLAGS)   $(DEFINES)
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


build/usbdrvasm.o: libraries/v-usb/usbdrv/usbdrvasm.S $(STDDEP) $(EXTRADEP)
	$(CC) -x assembler-with-cpp -c libraries/v-usb/usbdrv/usbdrvasm.S -o build/usbdrvasm.o $(MYCFLAGS)

build/oddebug.o: libraries/v-usb/usbdrv/oddebug.c $(STDDEP) $(EXTRADEP)
	$(CC) libraries/v-usb/usbdrv/oddebug.c -c -o build/oddebug.o $(MYCFLAGS)

build/usbdrv.o: libraries/v-usb/usbdrv/usbdrv.c $(STDDEP) $(EXTRADEP)
	$(CC) libraries/v-usb/usbdrv/usbdrv.c -c -o build/usbdrv.o $(MYCFLAGS)


build/hidcore.S: libraries/hid-KeyboardMouse/gcc-code/lib/hidcore.c $(STDDEP) $(EXTRADEP)
	$(CC) libraries/hid-KeyboardMouse/gcc-code/lib/hidcore.c -S -o build/hidcore.S $(MYCFLAGS)

build/hidcore.o: build/hidcore.S $(STDDEP) $(EXTRADEP)
	$(CC) build/hidcore.S -c -o build/hidcore.o $(MYCFLAGS)

build/asciimap.S: libraries/hid-KeyboardMouse/gcc-code/lib/asciimap.c $(STDDEP) $(EXTRADEP)
	$(CC) libraries/hid-KeyboardMouse/gcc-code/lib/asciimap.c -S -o build/asciimap.S $(MYCFLAGS)

build/asciimap.o: build/asciimap.S $(STDDEP) $(EXTRADEP)
	$(CC) build/asciimap.S -c -o build/asciimap.o $(MYCFLAGS)



build/apipage.S: libraries/API/apipage.c $(STDDEP) $(EXTRADEP)
	$(CC) libraries/API/apipage.c -S -o build/apipage.S $(MYCFLAGS)

build/apipage.o: build/apipage.S $(STDDEP) $(EXTRADEP)
	$(CC) build/apipage.S -c -o build/apipage.o $(MYCFLAGS)


build/extfunc.S: libraries/avrlibs-baerwolf/source/extfunc.c $(STDDEP) $(EXTRADEP)
	$(CC) libraries/avrlibs-baerwolf/source/extfunc.c -S -o build/extfunc.S $(MYCFLAGS)

build/extfunc.o: build/extfunc.S $(STDDEP) $(EXTRADEP)
	$(CC) build/extfunc.S -c -o build/extfunc.o $(MYCFLAGS)

build/cpucontext.S: libraries/avrlibs-baerwolf/source/cpucontext.c $(STDDEP) $(EXTRADEP)
	$(CC) libraries/avrlibs-baerwolf/source/cpucontext.c -S -o build/cpucontext.S $(MYCFLAGS)

build/cpucontext.o: build/cpucontext.S $(STDDEP) $(EXTRADEP)
	$(CC) build/cpucontext.S -c -o build/cpucontext.o $(MYCFLAGS)

build/hwclock.S: libraries/avrlibs-baerwolf/source/hwclock.c $(STDDEP) $(EXTRADEP)
	$(CC) libraries/avrlibs-baerwolf/source/hwclock.c -S -o build/hwclock.S $(MYCFLAGS)

build/hwclock.o: build/hwclock.S $(STDDEP) $(EXTRADEP)
	$(CC) build/hwclock.S -c -o build/hwclock.o $(MYCFLAGS)


build/button.S: source/button.c $(STDDEP) $(EXTRADEP)
	$(CC) source/button.c -S -o build/button.S $(MYCFLAGS)

build/button.o: build/button.S $(STDDEP) $(EXTRADEP)
	$(CC) build/button.S -c -o build/button.o $(MYCFLAGS)

build/main.S: source/main.c $(STDDEP) $(EXTRADEP)
	$(CC) source/main.c -S -o build/main.S $(MYCFLAGS)

build/main.o: build/main.S $(STDDEP) $(EXTRADEP)
	$(CC) build/main.S -c -o build/main.o $(MYCFLAGS)





MYOBJECTS = build/main.o build/button.o build/apipage.o build/extfunc.o build/cpucontext.o build/hwclock.o  build/usbdrv.o build/oddebug.o build/usbdrvasm.o build/hidcore.o build/asciimap.o
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
