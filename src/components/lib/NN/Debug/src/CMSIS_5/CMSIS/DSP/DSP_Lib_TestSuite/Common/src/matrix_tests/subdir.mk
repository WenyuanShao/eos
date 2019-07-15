################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_add_tests.c \
../src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_cmplx_mult_tests.c \
../src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_init_tests.c \
../src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_inverse_tests.c \
../src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_mult_fast_tests.c \
../src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_mult_tests.c \
../src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_scale_tests.c \
../src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_sub_tests.c \
../src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_trans_tests.c \
../src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/matrix_test_common_data.c \
../src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/matrix_test_group.c 

OBJS += \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_add_tests.o \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_cmplx_mult_tests.o \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_init_tests.o \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_inverse_tests.o \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_mult_fast_tests.o \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_mult_tests.o \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_scale_tests.o \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_sub_tests.o \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_trans_tests.o \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/matrix_test_common_data.o \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/matrix_test_group.o 

C_DEPS += \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_add_tests.d \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_cmplx_mult_tests.d \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_init_tests.d \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_inverse_tests.d \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_mult_fast_tests.d \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_mult_tests.d \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_scale_tests.d \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_sub_tests.d \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/mat_trans_tests.d \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/matrix_test_common_data.d \
./src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/matrix_test_group.d 


# Each subdirectory must supply rules for building sources it contributes
src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/%.o: ../src/CMSIS_5/CMSIS/DSP/DSP_Lib_TestSuite/Common/src/matrix_tests/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -std=c99 -DARM_MATH_CM7 -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/DSP/Include -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/Core/Include -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/NN/Include -O0 -g3 -Wall -c -fmessage-length=0 -Wno-unused-function -Wno-unused-variable -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


