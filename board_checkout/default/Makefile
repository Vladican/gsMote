################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL := cmd.exe
RM := rm -rf

USER_OBJS :=

LIBS := 
PROJ := 

O_SRCS := 
C_SRCS := 
S_SRCS := 
S_UPPER_SRCS := 
OBJ_SRCS := 
ASM_SRCS := 
PREPROCESSING_SRCS := 
OBJS := 
OBJS_AS_ARGS := 
C_DEPS := 
C_DEPS_AS_ARGS := 
EXECUTABLES := 
OUTPUT_FILE_PATH :=
OUTPUT_FILE_PATH_AS_ARGS :=
AVR_APP_PATH :=$$$AVR_APP_PATH$$$
QUOTE := "
ADDITIONAL_DEPENDENCIES:=
OUTPUT_FILE_DEP:=

# Every subdirectory with source files must be described here
SUBDIRS := 


# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS +=  \
../chb.c \
../chb_buf.c \
../chb_drvr.c \
../chb_eeprom.c \
../chb_spi.c \
../board_checkout.c \
../adc_driver.c \
../clksys_driver.c \
../E-000001-000009__firmware_rev_1_0.c \
../FAT32.c


PREPROCESSING_SRCS += 


ASM_SRCS += 


OBJS +=  \
chb.o \
chb_buf.o \
chb_drvr.o \
chb_eeprom.o \
chb_spi.o \
board_checkout.o \
adc_driver.o \
clksys_driver.o \
E-000001-000009__firmware_rev_1_0.o \
FAT32.o


OBJS_AS_ARGS +=  \
chb.o \
chb_buf.o \
chb_drvr.o \
chb_eeprom.o \
chb_spi.o \
board_checkout.o \
adc_driver.o \
clksys_driver.o \
E-000001-000009__firmware_rev_1_0.o \
FAT32.o


C_DEPS +=  \
chb.d \
chb_buf.d \
chb_drvr.d \
chb_eeprom.d \
chb_spi.d \
board_checkout.d \
adc_driver.d \
clksys_driver.d \
E-000001-000009__firmware_rev_1_0.d \
FAT32.d


C_DEPS_AS_ARGS +=  \
chb.d \
chb_buf.d \
chb_drvr.d \
chb_eeprom.d \
chb_spi.d \
board_checkout.d \
adc_driver.d \
clksys_driver.d \
E-000001-000009__firmware_rev_1_0.d \
FAT32.d


OUTPUT_FILE_PATH +=board_checkout.elf

OUTPUT_FILE_PATH_AS_ARGS +=board_checkout.elf

ADDITIONAL_DEPENDENCIES:=

OUTPUT_FILE_DEP:= ./makedep.mk

# AVR32/GNU C Compiler





















./%.o: .././%.c
	@echo Building file: $<
	@echo Invoking: AVR/GNU C Compiler : (AVR_8_bit_GNU_Toolchain_3.4.0_663) 4.6.2
	$(QUOTE)C:\Program Files (x86)\Atmel\Atmel Studio 6.0\extensions\Atmel\AVRGCC\3.4.0.65\AVRToolchain\bin\avr-gcc.exe$(QUOTE) -funsigned-char -funsigned-bitfields -Os -fpack-struct -fshort-enums -Wall -c -gdwarf-2 -std=gnu99  -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"  -mmcu=atxmega256a3   -o"$@" "$<" 
	@echo Finished building: $<



# AVR32/GNU Preprocessing Assembler



# AVR32/GNU Assembler




ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: $(OUTPUT_FILE_PATH) $(ADDITIONAL_DEPENDENCIES)

$(OUTPUT_FILE_PATH): $(OBJS) $(USER_OBJS) $(OUTPUT_FILE_DEP)
	@echo Building target: $@
	@echo Invoking: AVR/GNU Linker : (AVR_8_bit_GNU_Toolchain_3.4.0_663) 4.6.2
	$(QUOTE)C:\Program Files (x86)\Atmel\Atmel Studio 6.0\extensions\Atmel\AVRGCC\3.4.0.65\AVRToolchain\bin\avr-gcc.exe$(QUOTE) -o$(OUTPUT_FILE_PATH_AS_ARGS) $(OBJS_AS_ARGS) $(USER_OBJS) $(LIBS) -Wl,-Map="board_checkout.map" -Wl,--start-group -Wl,-lm  -Wl,--end-group  -mmcu=atxmega256a3  
	@echo Finished building target: $@
	"C:\Program Files (x86)\Atmel\Atmel Studio 6.0\extensions\Atmel\AVRGCC\3.4.0.65\AVRToolchain\bin\avr-objcopy.exe" -O ihex -R .eeprom -R .fuse -R .lock -R .signature  "board_checkout.elf" "board_checkout.hex"
	"C:\Program Files (x86)\Atmel\Atmel Studio 6.0\extensions\Atmel\AVRGCC\3.4.0.65\AVRToolchain\bin\avr-objcopy.exe" -j .eeprom  --set-section-flags=.eeprom=alloc,load --change-section-lma .eeprom=0  --no-change-warnings -O ihex "board_checkout.elf" "board_checkout.eep" || exit 0
	"C:\Program Files (x86)\Atmel\Atmel Studio 6.0\extensions\Atmel\AVRGCC\3.4.0.65\AVRToolchain\bin\avr-objdump.exe" -h -S "board_checkout.elf" > "board_checkout.lss"
	"C:\Program Files (x86)\Atmel\Atmel Studio 6.0\extensions\Atmel\AVRGCC\3.4.0.65\AVRToolchain\bin\avr-size.exe" "board_checkout.elf"
	





# Other Targets
clean:
	-$(RM) $(OBJS_AS_ARGS)$(C_DEPS_AS_ARGS) $(EXECUTABLES) 
	rm -rf "board_checkout.elf" "board_checkout.a" "board_checkout.hex" "board_checkout.lss" "board_checkout.eep" "board_checkout.map"
	