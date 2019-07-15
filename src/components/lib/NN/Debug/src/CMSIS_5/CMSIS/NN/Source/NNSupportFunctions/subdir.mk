################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/CMSIS_5/CMSIS/NN/Source/NNSupportFunctions/arm_nn_mult_q15.c \
../src/CMSIS_5/CMSIS/NN/Source/NNSupportFunctions/arm_nn_mult_q7.c \
../src/CMSIS_5/CMSIS/NN/Source/NNSupportFunctions/arm_nntables.c \
../src/CMSIS_5/CMSIS/NN/Source/NNSupportFunctions/arm_q7_to_q15_no_shift.c \
../src/CMSIS_5/CMSIS/NN/Source/NNSupportFunctions/arm_q7_to_q15_reordered_no_shift.c 

OBJS += \
./src/CMSIS_5/CMSIS/NN/Source/NNSupportFunctions/arm_nn_mult_q15.o \
./src/CMSIS_5/CMSIS/NN/Source/NNSupportFunctions/arm_nn_mult_q7.o \
./src/CMSIS_5/CMSIS/NN/Source/NNSupportFunctions/arm_nntables.o \
./src/CMSIS_5/CMSIS/NN/Source/NNSupportFunctions/arm_q7_to_q15_no_shift.o \
./src/CMSIS_5/CMSIS/NN/Source/NNSupportFunctions/arm_q7_to_q15_reordered_no_shift.o 

C_DEPS += \
./src/CMSIS_5/CMSIS/NN/Source/NNSupportFunctions/arm_nn_mult_q15.d \
./src/CMSIS_5/CMSIS/NN/Source/NNSupportFunctions/arm_nn_mult_q7.d \
./src/CMSIS_5/CMSIS/NN/Source/NNSupportFunctions/arm_nntables.d \
./src/CMSIS_5/CMSIS/NN/Source/NNSupportFunctions/arm_q7_to_q15_no_shift.d \
./src/CMSIS_5/CMSIS/NN/Source/NNSupportFunctions/arm_q7_to_q15_reordered_no_shift.d 


# Each subdirectory must supply rules for building sources it contributes
src/CMSIS_5/CMSIS/NN/Source/NNSupportFunctions/%.o: ../src/CMSIS_5/CMSIS/NN/Source/NNSupportFunctions/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -std=c99 -DARM_MATH_CM3 -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/DSP/Include -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/Core/Include -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/NN/Include -O3 -g3 -Wall -c -fmessage-length=0 -Wno-unused-function -Wno-unused-variable -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


