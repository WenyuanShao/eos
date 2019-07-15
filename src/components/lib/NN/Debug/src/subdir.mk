################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/arm_nnexamples_cifar10.cpp 

# C_SRCS += \
# ../src/NN.c 

OBJS += \
./src/arm_nnexamples_cifar10.o 
# ./src/NN.o \

CPP_DEPS += \
./src/arm_nnexamples_cifar10.d 

# C_DEPS += \
# ./src/NN.d 
LIBDIR = /home/shaowy/clone/composite/src/components/lib
CXX_MUSL_FLAG = -nostdinc -nostdlib -fno-stack-protector -fno-threadsafe-statics -fno-exceptions
CXX_MUSL_INC = -I$(LIBDIR)/musl-1.1.11/include -I$(LIBDIR)/libcxx/include -I$(LIBDIR)/libcxx/libstdc++-v3-4.8/include
MUSL_CC = $(LIBDIR)/musl-1.1.11/bin/musl-gcc

# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -std=c99 -DARM_MATH_CM3 -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/DSP/Include -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/Core/Include -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/NN/Include -O3 -g3 -Wall -c -fmessage-length=0 -Wno-unused-function -Wno-unused-variable -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DARM_MATH_CM3 -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/DSP/Include -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/Core/Include -I/home/shaowy/workspace/NN/src/CMSIS_5/CMSIS/NN/Include $(CXX_MUSL_INC) $(CXX_MUSL_FLAG) -O3 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


