################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/NetworkSniffer.cpp \
../src/consoleGUI.cpp \
../src/ipv4_clss.cpp \
../src/ipv6_clss.cpp 

OBJS += \
./src/NetworkSniffer.o \
./src/consoleGUI.o \
./src/ipv4_clss.o \
./src/ipv6_clss.o 

CPP_DEPS += \
./src/NetworkSniffer.d \
./src/consoleGUI.d \
./src/ipv4_clss.d \
./src/ipv6_clss.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


