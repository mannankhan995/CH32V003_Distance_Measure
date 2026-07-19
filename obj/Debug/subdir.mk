################################################################################
# MRS Version: 2.5.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Debug/debug.c 

C_DEPS += \
./Debug/debug.d 

OBJS += \
./Debug/debug.o 

DIR_OBJS += \
./Debug/*.o \

DIR_DEPS += \
./Debug/*.d \

DIR_EXPANDS += \
./Debug/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
Debug/%.o: ../Debug/%.c
	@	riscv-none-embed-gcc -march=rv32ecxw -mabi=ilp32e -msmall-data-limit=0 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/Users/HOME-PC-1/Documents/CH32V003EVT/PROJECTS/Distance_Measurement/CH32V003F4P/Debug" -I"c:/Users/HOME-PC-1/Documents/CH32V003EVT/PROJECTS/Distance_Measurement/CH32V003F4P/Core" -I"c:/Users/HOME-PC-1/Documents/CH32V003EVT/PROJECTS/Distance_Measurement/CH32V003F4P/User" -I"c:/Users/HOME-PC-1/Documents/CH32V003EVT/PROJECTS/Distance_Measurement/CH32V003F4P/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

