# Variables
CFLAGS = -mmcu=atmega32u4 -Wall -Os   # Compiler flags
CFLAGS_USB = -Os -DF_CPU=16000000UL -mmcu=atmega32u4 -Ilib 

M_TARGET 			 = main_master           		 # Base name of the output executable
M_ELF           = main_master.elf       		 # ELF file
M_HEX           = main_master.hex       		 # HEX file
M_SRC           = main_master.c         		 # Source file
M_SRC           += helper.c i2c.c usb.c mapping.c macros.c     # Source file

S_TARGET 			 = main_slave           		 # Base name of the output executable
S_ELF           = main_slave.elf       		 		# ELF file
S_HEX           = main_slave.hex       		 		# HEX file
S_SRC           = main_slave.c         		 		# Source file
S_SRC           += helper.c i2c.c usb.c mapping.c	# Source file

# Default target
all: master mclean

# Rule to build the ELF file
master: $(M_SRC)
	avr-gcc $(CFLAGS) -o $(M_ELF) $(M_SRC)
	avr-objcopy -j .text -j .data -O ihex $(M_ELF) $(M_HEX)
	sudo avrdude -c avr109 -p ATMEGA32U4 -P /dev/ttyACM0 -U flash:w:main_master.hex;

slave: $(S_SRC)
	avr-gcc $(CFLAGS) -o $(S_ELF) $(S_SRC)
	avr-objcopy -j .text -j .data -O ihex $(S_ELF) $(S_HEX)
	
# Upload files to Master
mup:
	sudo avrdude -c avr109 -p ATMEGA32U4 -P /dev/ttyACM0 -U flash:w:main_master.hex;

# Upload files to Slave
sup:
	sudo avrdude -c avr109 -p ATMEGA32U4 -P /dev/ttyACM0 -U flash:w:main_slave.hex;

# Clean up all build files
clean: mclean sclean dmclean dsclean

# Clean master build files
mclean:
	rm -f $(M_ELF) $(M_HEX)

# Clean slave build files
sclean:
	rm -f $(S_ELF) $(S_HEX)

# Compile, link, generate hex and flash (for use by GUI tool)
master_flash: master mclean
