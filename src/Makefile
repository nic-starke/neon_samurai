# Copyright 2021 Nicolaus Starke
#
# Author: Nicolaus Starke - mail@bxzn.one
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

TARGET 						= muffin
VERSION						= 0.1
AVR_GCC_PATH			= /opt/avr/avr-linux/bin/
AVR_LIBC_INC			= /opt/avr/avr-libc/avr/include/
AVRDUDE_PATH			= /usr/bin/
PROGRAMMER				= avrispmkII
PROGRAMMER_PORT		= usb
MCU 							= atxmega128a4u
PACKAGES					= src \

VPATH							= $(PACKAGES)
LIBRARY_PATH			= /opt/avr/avr-libc/avr/lib/avrxmega7
BUILD_ROOT				= build/
BUILD_DIR					= $(BUILD_ROOT)$(TARGET)/

C_FILES						= $(notdir $(wildcard $(patsubst %,%/*.c,$(PACKAGES))))
OBJ_FILES					= $(CC_FILES:.cc=.o) $(C_FILES:.c=.o) $(AS_FILES:.S=.o)
OBJS							= $(patsubst %,$(BUILD_DIR)%,$(OBJ_FILES))
DEPS							= $(OBJS:.o=.d)

TARGET_BIN				= $(BUILD_DIR)$(TARGET).bin
TARGET_ELF				= $(BUILD_DIR)$(TARGET).elf
TARGET_HEX				= $(BUILD_DIR)$(TARGET).hex
TARGET_SIZE				= $(BUILD_DIR)$(TARGET).size
TARGETS						= $(BUILD_DIR)$(TARGET).*
# DEP_FILE					= $(BUILD_DIR)depends.mk

CC								= $(AVR_GCC_PATH)avr-gcc
OBJCOPY						= $(AVR_GCC_PATH)avr-objcopy
OBJDUMP						= $(AVR_GCC_PATH)avr-objdump
AR								= $(AVR_GCC_PATH)avr-ar
SIZE							= $(AVR_GCC_PATH)avr-size
NM								= $(AVR_GCC_PATH)avr-nm
AVRDUDE						= $(AVRDUDE_PATH)avrdude
REMOVE						= rm -f
CAT								= cat

CFLAGS = -mmcu=$(MCU) -I. \
			-I$(AVR_LIBC_INC) \
			-O3 -Wall -Wextra -Wpedantic \
			-DBOARD=USER_BOARD \
			-DF_USB=48000000UL \
			-DF_CPU=32000000UL \
			-DUSE_LUFA_CONFIG_HEADER \
			-DARCH=ARCH_XMEGA \
			-DRELEASE \
			-DNDEBUG \
			-DATXMEGA128A4U \
			-fdata-sections \
			-ffunction-sections \
			-fpack-struct \
			-fshort-enums \
			-std=gnu99 \
			-mrelax \

LDFLAGS = -mmcu=$(MCU) -lm -O3 -Wl,--gc-sections$(EXTRA_LD_FLAGS) -B$(LIBRARY_PATH)

# Compilation

$(BUILD_DIR)%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)%.o: %.s
	$(CC) -c $(CFLAGS) $< -o $@

# Conversion

$(BUILD_DIR)%.hex: $(BUILD_DIR)%.elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

$(BUILD_DIR)%.bin: $(BUILD_DIR)%.elf
	$(OBJCOPY) -O binary -R .eeprom $< $@

$(BUILD_DIR)%.eep: $(BUILD_DIR)%.elf
	-$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
		--change-section-lma .eeprom=0 -O ihex $< $@

$(BUILD_DIR)%.lss: $(BUILD_DIR)%.elf
	$(OBJDUMP) -h -S $< > $@

$(BUILD_DIR)%.sym: $(BUILD_DIR)%.elf
	$(NM) -n $< > $@

# AVRDude
AVRDUDE_COM_OPTS = -V -p $(DMCU)
AVRDUDE_ISP_OPTS = -c $(PROGRAMMER) -P $(PROGRAMMER_PORT)

# Build Targets

all:    $(BUILD_DIR) $(TARGET_HEX)

$(BUILD_DIR):
		mkdir -p $(BUILD_DIR)

$(TARGET_ELF):  $(OBJS)
		$(CC) $(LDFLAGS) -o $@ $(OBJS) $(SYS_OBJS) -lc

$(DEP_FILE):  $(BUILD_DIR) $(DEPS)
		cat $(DEPS) > $(DEP_FILE)

bin:	$(TARGET_BIN)

upload:    $(TARGET_HEX)
		$(AVRDUDE) $(AVRDUDE_COM_OPTS) $(AVRDUDE_ISP_OPTS) \
			-U flash:w:$(TARGET_HEX):i

upload_boot:    $(TARGET_BIN)
		$(AVRDUDE) $(AVRDUDE_COM_OPTS) $(AVRDUDE_ISP_OPTS) \
			-U boot:w:$(TARGET_BIN)

clean:
		$(REMOVE) $(OBJS) $(TARGETS) $(DEP_FILE) $(DEPS)

depends:  $(DEPS)
		cat $(DEPS) > $(DEP_FILE)

$(TARGET_SIZE):  $(TARGET_ELF)
		$(SIZE) $(TARGET_ELF) > $(TARGET_SIZE)

$(BUILD_DIR)$(TARGET).top_symbols: $(TARGET_ELF)
		$(NM) $(TARGET_ELF) --size-sort -C -f bsd -r > $@

size: $(TARGET_SIZE)
		cat $(TARGET_SIZE) | awk '{ print $$1+$$2 }' | tail -n1 | figlet

ramsize: $(TARGET_SIZE)
		cat $(TARGET_SIZE) | awk '{ print $$2+$$3 }' | tail -n1 | figlet
size_report:  build/$(TARGET)/$(TARGET).lss build/$(TARGET)/$(TARGET).top_symbols

.PHONY: all clean depends upload


include $(DEP_FILE)