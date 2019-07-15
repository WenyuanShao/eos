################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/CMSIS_5/CMSIS/NN/NN_Lib_Tests/nn_test/RTE/Device/ARMCM7_SP/startup_ARMCM7.c \
../src/CMSIS_5/CMSIS/NN/NN_Lib_Tests/nn_test/RTE/Device/ARMCM7_SP/system_ARMCM7.c 

OBJS += \
./src/CMSIS_5/CMSIS/NN/NN_Lib_Tests/nn_test/RTE/Device/ARMCM7_SP/startup_ARMCM7.o \
./src/CMSIS_5/CMSIS/NN/NN_Lib_Tests/nn_test/RTE/Device/ARMCM7_SP/system_ARMCM7.o 

C_DEPS += \
./src/CMSIS_5/CMSIS/NN/NN_Lib_Tests/nn_test/RTE/Device/ARMCM7_SP/startup_ARMCM7.d \
./src/CMSIS_5/CMSIS/NN/NN_Lib_Tests/nn_test/RTE/Device/ARMCM7_SP/system_ARMCM7.d 


# Each subdirectory must supply rules for building sources it contributes
src/CMSIS_5/CMSIS/NN/NN_Lib_Tests/nn_test/RTE/Device/ARMCM7_SP/%.o: ../src/CMSIS_5/CMSIS/NN/NN_Lib_Tests/nn_test/RTE/Device/ARMCM7_SP/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -std=c99 -DARM_MATH_CM7 -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/DSP/Include -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/Core/Include -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/NN/Include -O0 -g3 -Wall -c -fmessage-length=0 -Wno-unused-function -Wno-unused-variable -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


