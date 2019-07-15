################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_f32.c \
../src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_q15.c \
../src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_q31.c \
../src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_f32.c \
../src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_q15.c \
../src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_q31.c \
../src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/arm_sin_cos_f32.c \
../src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/arm_sin_cos_q31.c 

OBJS += \
./src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_f32.o \
./src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_q15.o \
./src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_q31.o \
./src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_f32.o \
./src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_q15.o \
./src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_q31.o \
./src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/arm_sin_cos_f32.o \
./src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/arm_sin_cos_q31.o 

C_DEPS += \
./src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_f32.d \
./src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_q15.d \
./src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_q31.d \
./src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_f32.d \
./src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_q15.d \
./src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_q31.d \
./src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/arm_sin_cos_f32.d \
./src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/arm_sin_cos_q31.d 


# Each subdirectory must supply rules for building sources it contributes
src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/%.o: ../src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -std=c99 -DARM_MATH_CM3 -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/DSP/Include -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/Core/Include -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/NN/Include -O3 -g3 -Wall -c -fmessage-length=0 -Wno-unused-function -Wno-unused-variable -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


