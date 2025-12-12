################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/KEYPAD.c \
../Core/Src/global.c \
../Core/Src/i2c_lcd.c \
../Core/Src/input_processing.c \
../Core/Src/input_reading.c \
../Core/Src/kmp.c \
../Core/Src/main.c \
../Core/Src/output_processing.c \
../Core/Src/scheduler.c \
../Core/Src/state_processing.c \
../Core/Src/stm32f1xx_hal_msp.c \
../Core/Src/stm32f1xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f1xx.c \
../Core/Src/timer.c 

OBJS += \
./Core/Src/KEYPAD.o \
./Core/Src/global.o \
./Core/Src/i2c_lcd.o \
./Core/Src/input_processing.o \
./Core/Src/input_reading.o \
./Core/Src/kmp.o \
./Core/Src/main.o \
./Core/Src/output_processing.o \
./Core/Src/scheduler.o \
./Core/Src/state_processing.o \
./Core/Src/stm32f1xx_hal_msp.o \
./Core/Src/stm32f1xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f1xx.o \
./Core/Src/timer.o 

C_DEPS += \
./Core/Src/KEYPAD.d \
./Core/Src/global.d \
./Core/Src/i2c_lcd.d \
./Core/Src/input_processing.d \
./Core/Src/input_reading.d \
./Core/Src/kmp.d \
./Core/Src/main.d \
./Core/Src/output_processing.d \
./Core/Src/scheduler.d \
./Core/Src/state_processing.d \
./Core/Src/stm32f1xx_hal_msp.d \
./Core/Src/stm32f1xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f1xx.d \
./Core/Src/timer.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

