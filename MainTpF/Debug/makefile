################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := project
BUILD_ARTIFACT_EXTENSION := axf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all:
	+@$(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

# Main-build Target
main-build: project.axf

# Tool invocations
project.axf: $(OBJS) $(USER_OBJS) makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -L"C:\Users\Andres\Documents\MCUXpressoIDE_25.6.136\workspace\game_lib\inc" -L"C:\Users\Andres\Documents\MCUXpressoIDE_25.6.136\workspace\game_lib\Debug" -L"C:\Users\Andres\Documents\MCUXpressoIDE_25.6.136\workspace\CMSISv2p00_LPC17xx\Debug" -L"C:\Users\Andres\Documents\MCUXpressoIDE_25.6.136\workspace\CMSISv2p00_LPC17xx\Drivers\inc" -Xlinker -Map="project.map" -Xlinker --cref -Xlinker --gc-sections -Xlinker -print-memory-usage -mcpu=cortex-m3 -mthumb -T project_Debug.ld -o "project.axf" $(OBJS) $(USER_OBJS) $(LIBS) -lgame_lib -lCMSISv2p00_LPC17xx
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) project.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "project.axf"; # arm-none-eabi-objcopy -v -O binary "project.axf" "project.bin" ; # checksum -p LPC1769 -d "project.bin";
	-@echo ' '

.PHONY: all clean dependents main-build post-build

-include ../makefile.targets
