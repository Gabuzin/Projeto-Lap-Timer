################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../libs/imu_orientation/imu_orientation.c 

OBJS += \
./libs/imu_orientation/imu_orientation.o 

C_DEPS += \
./libs/imu_orientation/imu_orientation.d 


# Each subdirectory must supply rules for building sources it contributes
libs/imu_orientation/%.o libs/imu_orientation/%.su libs/imu_orientation/%.cyclo: ../libs/imu_orientation/%.c libs/imu_orientation/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I"C:/Users/Gabriel D/Documents/GitHub/Projeto-Lap-Timer/STM32/libs/ICM20948" -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../libs/imu_orientation -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-libs-2f-imu_orientation

clean-libs-2f-imu_orientation:
	-$(RM) ./libs/imu_orientation/imu_orientation.cyclo ./libs/imu_orientation/imu_orientation.d ./libs/imu_orientation/imu_orientation.o ./libs/imu_orientation/imu_orientation.su

.PHONY: clean-libs-2f-imu_orientation

