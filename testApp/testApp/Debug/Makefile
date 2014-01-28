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
../../../FirmwareLib/FirmwareLib/SerialUSB.c \
../Node.c \
../Synch.c \
../../../FirmwareLib/FirmwareLib/ADC.c \
../../../FirmwareLib/FirmwareLib/adc_driver.c \
../../../FirmwareLib/FirmwareLib/chb.c \
../../../FirmwareLib/FirmwareLib/chb_buf.c \
../../../FirmwareLib/FirmwareLib/chb_drvr.c \
../../../FirmwareLib/FirmwareLib/chb_eeprom.c \
../../../FirmwareLib/FirmwareLib/chb_spi.c \
../../../FirmwareLib/FirmwareLib/clksys_driver.c \
../../../FirmwareLib/FirmwareLib/E-000001-000009__firmware_rev_1_0.c \
../../../FirmwareLib/FirmwareLib/FAT32.c \
../../../FirmwareLib/FirmwareLib/SD_Card.c


PREPROCESSING_SRCS += 


ASM_SRCS += 


OBJS +=  \
SerialUSB.o \
Node.o \
Synch.o \
ADC.o \
adc_driver.o \
chb.o \
chb_buf.o \
chb_drvr.o \
chb_eeprom.o \
chb_spi.o \
clksys_driver.o \
E-000001-000009__firmware_rev_1_0.o \
FAT32.o \
SD_Card.o


OBJS_AS_ARGS +=  \
SerialUSB.o \
Node.o \
Synch.o \
ADC.o \
adc_driver.o \
chb.o \
chb_buf.o \
chb_drvr.o \
chb_eeprom.o \
chb_spi.o \
clksys_driver.o \
E-000001-000009__firmware_rev_1_0.o \
FAT32.o \
SD_Card.o


C_DEPS +=  \
SerialUSB.d \
Node.d \
Synch.d \
ADC.d \
adc_driver.d \
chb.d \
chb_buf.d \
chb_drvr.d \
chb_eeprom.d \
chb_spi.d \
clksys_driver.d \
E-000001-000009__firmware_rev_1_0.d \
FAT32.d \
SD_Card.d


C_DEPS_AS_ARGS +=  \
SerialUSB.d \
Node.d \
Synch.d \
ADC.d \
adc_driver.d \
chb.d \
chb_buf.d \
chb_drvr.d \
chb_eeprom.d \
chb_spi.d \
clksys_driver.d \
E-000001-000009__firmware_rev_1_0.d \
FAT32.d \
SD_Card.d


OUTPUT_FILE_PATH +=testApp.elf

OUTPUT_FILE_PATH_AS_ARGS +=testApp.elf

ADDITIONAL_DEPENDENCIES:=

OUTPUT_FILE_DEP:= ./makedep.mk

# AVR32/GNU C Compiler





























./%.o: ../../../FirmwareLib/FirmwareLib/%.c
	@echo Building file: $<
	@echo Invoking: AVR/GNU C Compiler : (AVR_8_bit_GNU_Toolchain_3.4.0_663) 4.6.2
	$(QUOTE)C:\Program Files (x86)\Atmel\Atmel Studio 6.0\extensions\Atmel\AVRGCC\3.4.0.65\AVRToolchain\bin\avr-gcc.exe$(QUOTE) -funsigned-char -funsigned-bitfields -I"../../../FirmwareLib/FirmwareLib"  -O0 -fpack-struct -fshort-enums -g3 -Wall -c -std=gnu99 -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"  -mmcu=atxmega256a3   -o"$@" "$<" 
	@echo Finished building: $<

./%.o: .././%.c
	@echo Building file: $<
	@echo Invoking: AVR/GNU C Compiler : (AVR_8_bit_GNU_Toolchain_3.4.0_663) 4.6.2
	$(QUOTE)C:\Program Files (x86)\Atmel\Atmel Studio 6.0\extensions\Atmel\AVRGCC\3.4.0.65\AVRToolchain\bin\avr-gcc.exe$(QUOTE) -funsigned-char -funsigned-bitfields -I"../../../FirmwareLib/FirmwareLib"  -O0 -fpack-struct -fshort-enums -g3 -Wall -c -std=gnu99 -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"  -mmcu=atxmega256a3   -o"$@" "$<" 
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
	$(QUOTE)C:\Program Files (x86)\Atmel\Atmel Studio 6.0\extensions\Atmel\AVRGCC\3.4.0.65\AVRToolchain\bin\avr-gcc.exe$(QUOTE) -o$(OUTPUT_FILE_PATH_AS_ARGS) $(OBJS_AS_ARGS) $(USER_OBJS) $(LIBS) -Wl,-Map="testApp.map" -Wl,--start-group -Wl,-lm  -Wl,--end-group  -mmcu=atxmega256a3  
	@echo Finished building target: $@
	"C:\Program Files (x86)\Atmel\Atmel Studio 6.0\extensions\Atmel\AVRGCC\3.4.0.65\AVRToolchain\bin\avr-objcopy.exe" -O ihex -R .eeprom -R .fuse -R .lock -R .signature  "testApp.elf" "testApp.hex"
	"C:\Program Files (x86)\Atmel\Atmel Studio 6.0\extensions\Atmel\AVRGCC\3.4.0.65\AVRToolchain\bin\avr-objcopy.exe" -j .eeprom  --set-section-flags=.eeprom=alloc,load --change-section-lma .eeprom=0  --no-change-warnings -O ihex "testApp.elf" "testApp.eep" || exit 0
	"C:\Program Files (x86)\Atmel\Atmel Studio 6.0\extensions\Atmel\AVRGCC\3.4.0.65\AVRToolchain\bin\avr-objdump.exe" -h -S "testApp.elf" > "testApp.lss"
	"C:\Program Files (x86)\Atmel\Atmel Studio 6.0\extensions\Atmel\AVRGCC\3.4.0.65\AVRToolchain\bin\avr-size.exe" "testApp.elf"
	





# Other Targets
clean:
	-$(RM) $(OBJS_AS_ARGS)$(C_DEPS_AS_ARGS) $(EXECUTABLES) 
	rm -rf "testApp.elf" "testApp.a" "testApp.hex" "testApp.lss" "testApp.eep" "testApp.map"
	