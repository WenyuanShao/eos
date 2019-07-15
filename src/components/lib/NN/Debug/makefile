################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/CMSIS_5/CMSIS/NN/Source/SoftmaxFunctions/subdir.mk
-include src/CMSIS_5/CMSIS/NN/Source/PoolingFunctions/subdir.mk
-include src/CMSIS_5/CMSIS/NN/Source/NNSupportFunctions/subdir.mk
-include src/CMSIS_5/CMSIS/NN/Source/FullyConnectedFunctions/subdir.mk
-include src/CMSIS_5/CMSIS/NN/Source/ConvolutionFunctions/subdir.mk
-include src/CMSIS_5/CMSIS/NN/Source/ActivationFunctions/subdir.mk
-include src/CMSIS_5/CMSIS/DSP/Source/TransformFunctions/subdir.mk
-include src/CMSIS_5/CMSIS/DSP/Source/SupportFunctions/subdir.mk
-include src/CMSIS_5/CMSIS/DSP/Source/MatrixFunctions/subdir.mk
-include src/CMSIS_5/CMSIS/DSP/Source/FilteringFunctions/subdir.mk
-include src/CMSIS_5/CMSIS/DSP/Source/FastMathFunctions/subdir.mk
-include src/CMSIS_5/CMSIS/DSP/Source/ControllerFunctions/subdir.mk
-include src/CMSIS_5/CMSIS/DSP/Source/CommonTables/subdir.mk
-include src/CMSIS_5/CMSIS/DSP/Source/BasicMathFunctions/subdir.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: NN

# Tool invocations
NN: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	# g++  -o "NN" $(OBJS) $(USER_OBJS) $(LIBS)
	ld -r -o "NN_ld.o"  $(OBJS) $(USER_OBJS) $(LIBS)
	$(AR) cr "libNN.a"  $(OBJS) $(USER_OBJS) $(LIBS)
	cp libNN.a ../../
	cp NN_ld.o ../../
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(CC_DEPS)$(C++_DEPS)$(EXECUTABLES)$(C_UPPER_DEPS)$(CXX_DEPS)$(OBJS)$(CPP_DEPS)$(C_DEPS) NN libNN.a NN_ld.o 
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
