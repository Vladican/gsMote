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
SUBDIRS :=  \
../ChibiStack


# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS +=  \
../ADC.cpp \
../adc_driver.c \
../ChibiStack/chb.c \
../ChibiStack/chb_buf.c \
../ChibiStack/chb_drvr.c \
../ChibiStack/chb_eeprom.c \
../ChibiStack/chb_spi.c \
../clksys_driver.c \
../E-000001-000009_firmware_rev_1_0.cpp \
../FAT32.cpp \
../SD_Card.cpp


PREPROCESSING_SRCS += 


ASM_SRCS += 


OBJS +=  \
ADC.o \
adc_driver.o \
ChibiStack/chb.o \
ChibiStack/chb_buf.o \
ChibiStack/chb_drvr.o \
ChibiStack/chb_eeprom.o \
ChibiStack/chb_spi.o \
clksys_driver.o \
E-000001-000009_firmware_rev_1_0.o \
FAT32.o \
SD_Card.o


OBJS_AS_ARGS +=  \
ADC.o \
adc_driver.o \
ChibiStack/chb.o \
ChibiStack/chb_buf.o \
ChibiStack/chb_drvr.o \
ChibiStack/chb_eeprom.o \
ChibiStack/chb_spi.o \
clksys_driver.o \
E-000001-000009_firmware_rev_1_0.o \
FAT32.o \
SD_Card.o


C_DEPS +=  \
ADC.d \
adc_driver.d \
ChibiStack/chb.d \
ChibiStack/chb_buf.d \
ChibiStack/chb_drvr.d \
ChibiStack/chb_eeprom.d \
ChibiStack/chb_spi.d \
clksys_driver.d \
E-000001-000009_firmware_rev_1_0.d \
FAT32.d \
SD_Card.d


C_DEPS_AS_ARGS +=  \
ADC.d \
adc_driver.d \
ChibiStack/chb.d \
ChibiStack/chb_buf.d \
ChibiStack/chb_drvr.d \
ChibiStack/chb_eeprom.d \
ChibiStack/chb_spi.d \
clksys_driver.d \
E-000001-000009_firmware_rev_1_0.d \
FAT32.d \
SD_Card.d


OUTPUT_FILE_PATH +=libgsMoteFirmware.a

OUTPUT_FILE_PATH_AS_ARGS +=libgsMoteFirmware.a

ADDITIONAL_DEPENDENCIES:=

OUTPUT_FILE_DEP:= ./makedep.mk

# AVR32/GNU C Compiler























./%.o: .././%.c
	@echo Building file: $<
	@echo Invoking: AVR8/GNU C++ Compiler : (AVR_8_bit_GNU_Toolchain_3.4.0_663) 4.6.2
	$(QUOTE)C:\Program Files (x86)\Atmel\Atmel Studio 6.0\extensions\Atmel\AVRGCC\3.4.0.65\AVRToolchain\bin\avr-gcc.exe$(QUOTE) -funsigned-char -funsigned-bitfields -I".."  -O0 -fpack-struct -fshort-enums -g2 -Wall -c -std=gnu99 -v -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"  -mmcu=atxmega256a3   -o"$@" "$<" 
	@echo Finished building: $<

./%.o: .././%.cpp
	@echo Building file: $<
	@echo Invoking: AVR8/GNU C++ Compiler : (AVR_8_bit_GNU_Toolchain_3.4.0_663) 4.6.2
	$(QUOTE)C:\Program Files (x86)\Atmel\Atmel Studio 6.0\extensions\Atmel\AVRGCC\3.4.0.65\AVRToolchain\bin\avr-g++.exe$(QUOTE) -funsigned-char -funsigned-bitfields -I".."  -O0 -fpack-struct -fshort-enums -g2 -Wall -c -v -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"  -mmcu=atxmega256a3   -o"$@" "$<" 
	@echo Finished building: $<

ChibiStack/%.o: ../ChibiStack/%.c
	@echo Building file: $<
	@echo Invoking: AVR8/GNU C++ Compiler : (AVR_8_bit_GNU_Toolchain_3.4.0_663) 4.6.2
	$(QUOTE)C:\Program Files (x86)\Atmel\Atmel Studio 6.0\extensions\Atmel\AVRGCC\3.4.0.65\AVRToolchain\bin\avr-gcc.exe$(QUOTE) -funsigned-char -funsigned-bitfields -I".."  -O0 -fpack-struct -fshort-enums -g2 -Wall -c -std=gnu99 -v -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"  -mmcu=atxmega256a3   -o"$@" "$<" 
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
	@echo Invoking: AVR8/GNU Archiver : GNU ar (AVR_8_bit_GNU_Toolchain_3.4.0_663) 2.22
	$(QUOTE)C:\Program Files (x86)\Atmel\Atmel Studio 6.0\extensions\Atmel\AVRGCC\3.4.0.65\AVRToolchain\bin\avr-ar.exe$(QUOTE) -r  -o$(OUTPUT_FILE_PATH_AS_ARGS) $(OBJS_AS_ARGS) $(USER_OBJS) $(LIBS)
	@echo Finished building target: $@
	




# Other Targets
clean:
	-$(RM) $(OBJS_AS_ARGS)$(C_DEPS_AS_ARGS) $(EXECUTABLES) 
	rm -rf "libgsMoteFirmware.elf" "libgsMoteFirmware.a" "libgsMoteFirmware.hex" "libgsMoteFirmware.lss" "libgsMoteFirmware.eep" "libgsMoteFirmware.map"
	