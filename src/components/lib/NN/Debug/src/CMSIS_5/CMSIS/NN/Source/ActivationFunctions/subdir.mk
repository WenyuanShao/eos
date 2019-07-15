################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/CMSIS_5/CMSIS/NN/Source/ActivationFunctions/arm_nn_activations_q15.c \
../src/CMSIS_5/CMSIS/NN/Source/ActivationFunctions/arm_nn_activations_q7.c \
../src/CMSIS_5/CMSIS/NN/Source/ActivationFunctions/arm_relu_q15.c \
../src/CMSIS_5/CMSIS/NN/Source/ActivationFunctions/arm_relu_q7.c 

OBJS += \
./src/CMSIS_5/CMSIS/NN/Source/ActivationFunctions/arm_nn_activations_q15.o \
./src/CMSIS_5/CMSIS/NN/Source/ActivationFunctions/arm_nn_activations_q7.o \
./src/CMSIS_5/CMSIS/NN/Source/ActivationFunctions/arm_relu_q15.o \
./src/CMSIS_5/CMSIS/NN/Source/ActivationFunctions/arm_relu_q7.o 

C_DEPS += \
./src/CMSIS_5/CMSIS/NN/Source/ActivationFunctions/arm_nn_activations_q15.d \
./src/CMSIS_5/CMSIS/NN/Source/ActivationFunctions/arm_nn_activations_q7.d \
./src/CMSIS_5/CMSIS/NN/Source/ActivationFunctions/arm_relu_q15.d \
./src/CMSIS_5/CMSIS/NN/Source/ActivationFunctions/arm_relu_q7.d 


# Each subdirectory must supply rules for building sources it contributes
src/CMSIS_5/CMSIS/NN/Source/ActivationFunctions/%.o: ../src/CMSIS_5/CMSIS/NN/Source/ActivationFunctions/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -std=c99 -DARM_MATH_CM3 -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/DSP/Include -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/Core/Include -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/NN/Include -O3 -g3 -Wall -c -fmessage-length=0 -Wno-unused-function -Wno-unused-variable -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


