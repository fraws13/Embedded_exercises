################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Drivers/STM32G4xx_HAL_Driver/Src/subdir.mk
-include Core/Startup/subdir.mk
-include Core/Src/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := Riconoscimento(Short vs Long Press)
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
Riconoscimento(Short\ vs\ Long\ Press).elf \

MAP_FILES += \
Riconoscimento(Short\ vs\ Long\ Press).map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
Riconoscimento(Short\ vs\ Long\ Press).list \


# All Target
all: main-build

# Main-build Target
main-build: Riconoscimento(Short\ vs\ Long\ Press).elf secondary-outputs

# Tool invocations
Riconoscimento(Short\ vs\ Long\ Press).elf Riconoscimento(Short\ vs\ Long\ Press).map: $(OBJS) $(USER_OBJS) C:\Users\frank\Documents\Uni\Embedded\project\ location\esercitazioni\Riconoscimento(Short\ vs\ Long\ Press)\Riconoscimento(Short\ vs\ Long\ Press)\STM32G474RETX_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "Riconoscimento(Short vs Long Press).elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m4 -T"C:\Users\frank\Documents\Uni\Embedded\project location\esercitazioni\Riconoscimento(Short vs Long Press)\Riconoscimento(Short vs Long Press)\STM32G474RETX_FLASH.ld" --specs=nosys.specs -Wl,-Map="Riconoscimento(Short vs Long Press).map" -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

Riconoscimento(Short\ vs\ Long\ Press).list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "Riconoscimento(Short vs Long Press).list"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) Riconoscimento(Short\ vs\ Long\ Press).elf Riconoscimento(Short\ vs\ Long\ Press).list Riconoscimento(Short\ vs\ Long\ Press).map Riconoscimento(Short\\ vs\\ Long\\ Press).elf default.size.stdout
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified

-include ../makefile.targets
