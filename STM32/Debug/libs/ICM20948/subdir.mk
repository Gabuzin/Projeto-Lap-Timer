################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../libs/ICM20948/ICM_20948.c 

OBJS += \
./libs/ICM20948/ICM_20948.o 

C_DEPS += \
./libs/ICM20948/ICM_20948.d 


# Each subdirectory must supply rules for building sources it contributes
libs/ICM20948/%.o libs/ICM20948/%.su libs/ICM20948/%.cyclo: ../libs/ICM20948/%.c libs/ICM20948/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I"C:/Users/Gabriel D/Documents/GitHub/Lap-Timer/STM/libs/ICM20948" -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../libs/imu_orientation -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-libs-2f-ICM20948

clean-libs-2f-ICM20948:
	-$(RM) ./libs/ICM20948/ICM_20948.cyclo ./libs/ICM20948/ICM_20948.d ./libs/ICM20948/ICM_20948.o ./libs/ICM20948/ICM_20948.su

.PHONY: clean-libs-2f-ICM20948

