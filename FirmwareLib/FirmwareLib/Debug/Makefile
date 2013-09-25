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
../ADC.c \
../adc_driver.c \
../chb.c \
../chb_buf.c \
../chb_drvr.c \
../chb_eeprom.c \
../chb_spi.c \
../clksys_driver.c \
../E-000001-000009__firmware_rev_1_0.c \
../FAT32.c \
../SD_Card.c \
../SerialUSB.c


PREPROCESSING_SRCS += 


ASM_SRCS += 


OBJS +=  \
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
SD_Card.o \
SerialUSB.o


OBJS_AS_ARGS +=  \
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
SD_Card.o \
SerialUSB.o


C_DEPS +=  \
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
SD_Card.d \
SerialUSB.d


C_DEPS_AS_ARGS +=  \
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
SD_Card.d \
SerialUSB.d


OUTPUT_FILE_PATH +=libFirmwareLib.a

OUTPUT_FILE_PATH_AS_ARGS +=libFirmwareLib.a

ADDITIONAL_DEPENDENCIES:=

OUTPUT_FILE_DEP:= ./makedep.mk

# AVR32/GNU C Compiler

























./%.o: .././%.c
	@echo Building file: $<
	@echo Invoking: AVR/GNU C Compiler : (AVR_8_bit_GNU_Toolchain_3.4.0_663) 4.6.2
	$(QUOTE)C:\Program Files (x86)\Atmel\Atmel Studio 6.0\extensions\Atmel\AVRGCC\3.4.0.65\AVRToolchain\bin\avr-gcc.exe$(QUOTE) -funsigned-char -funsigned-bitfields -O1 -fpack-struct -fshort-enums -g2 -Wall -c -std=gnu99 -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"  -mmcu=atxmega256a3   -o"$@" "$<" 
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
	@echo Invoking: AVR/GNU Archiver : GNU ar (AVR_8_bit_GNU_Toolchain_3.4.0_663) 2.22
	$(QUOTE)C:\Program Files (x86)\Atmel\Atmel Studio 6.0\extensions\Atmel\AVRGCC\3.4.0.65\AVRToolchain\bin\avr-ar.exe$(QUOTE) -r  -o$(OUTPUT_FILE_PATH_AS_ARGS) $(OBJS_AS_ARGS) $(USER_OBJS) $(LIBS)
	@echo Finished building target: $@
	




# Other Targets
clean:
	-$(RM) $(OBJS_AS_ARGS)$(C_DEPS_AS_ARGS) $(EXECUTABLES) 
	rm -rf "libFirmwareLib.elf" "libFirmwareLib.a" "libFirmwareLib.hex" "libFirmwareLib.lss" "libFirmwareLib.eep" "libFirmwareLib.map"
	