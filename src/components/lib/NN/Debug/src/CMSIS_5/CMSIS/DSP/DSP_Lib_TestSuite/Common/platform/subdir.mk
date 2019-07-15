################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMCM0.c \
../src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMCM23.c \
../src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMCM3.c \
../src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMCM33.c \
../src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMCM4.c \
../src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMCM7.c \
../src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMSC000.c \
../src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMSC300.c \
../src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMv8MBL.c \
../src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMv8MML.c \
../src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_generic.c 

S_UPPER_SRCS += \
../src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/startup_generic.S 

OBJS += \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/startup_generic.o \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMCM0.o \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMCM23.o \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMCM3.o \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMCM33.o \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMCM4.o \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMCM7.o \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMSC000.o \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMSC300.o \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMv8MBL.o \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMv8MML.o \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_generic.o 

C_DEPS += \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMCM0.d \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMCM23.d \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMCM3.d \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMCM33.d \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMCM4.d \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMCM7.d \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMSC000.d \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMSC300.d \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMv8MBL.d \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_ARMv8MML.d \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/system_generic.d 


# Each subdirectory must supply rules for building sources it contributes
src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/%.o: ../src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/%.S
	@echo 'Building file: $<'
	@echo 'Invoking: GCC Assembler'
	as  -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/%.o: ../src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/platform/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -std=c99 -DARM_MATH_CM7 -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/DSP/Include -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/Core/Include -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/NN/Include -O0 -g3 -Wall -c -fmessage-length=0 -Wno-unused-function -Wno-unused-variable -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


