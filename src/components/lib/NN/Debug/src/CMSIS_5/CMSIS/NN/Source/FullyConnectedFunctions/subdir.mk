################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/CMSIS_5/CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_mat_q7_vec_q15.c \
../src/CMSIS_5/CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_mat_q7_vec_q15_opt.c \
../src/CMSIS_5/CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_q15.c \
../src/CMSIS_5/CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_q15_opt.c \
../src/CMSIS_5/CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_q7.c \
../src/CMSIS_5/CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_q7_opt.c 

OBJS += \
./src/CMSIS_5/CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_mat_q7_vec_q15.o \
./src/CMSIS_5/CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_mat_q7_vec_q15_opt.o \
./src/CMSIS_5/CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_q15.o \
./src/CMSIS_5/CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_q15_opt.o \
./src/CMSIS_5/CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_q7.o \
./src/CMSIS_5/CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_q7_opt.o 

C_DEPS += \
./src/CMSIS_5/CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_mat_q7_vec_q15.d \
./src/CMSIS_5/CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_mat_q7_vec_q15_opt.d \
./src/CMSIS_5/CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_q15.d \
./src/CMSIS_5/CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_q15_opt.d \
./src/CMSIS_5/CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_q7.d \
./src/CMSIS_5/CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_q7_opt.d 


# Each subdirectory must supply rules for building sources it contributes
src/CMSIS_5/CMSIS/NN/Source/FullyConnectedFunctions/%.o: ../src/CMSIS_5/CMSIS/NN/Source/FullyConnectedFunctions/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -std=c99 -DARM_MATH_CM3 -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/DSP/Include -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/Core/Include -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/NN/Include -O3 -g3 -Wall -c -fmessage-length=0 -Wno-unused-function -Wno-unused-variable -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


