BUILD_VERBOSE=1
include ../../py/mkenv.mk

# include diffenten path py core make definitions
include mkconf.mk

CROSS = 0

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

# include py core make definitions
include $(TOP)/py/py.mk

FREERTOS_PROJECT := freertos/builds/MSP_EXP432E401Y
FREERTOS_LIB := freertos.lib

SYSCONFIG_PRODUCT := $(SIMPLELINK_MSP432E4_SDK_INSTALL_DIR)/.metadata/product.json


CC = "$(GCC_ARMCOMPILER)/bin/arm-none-eabi-gcc"
LNK = "$(GCC_ARMCOMPILER)/bin/arm-none-eabi-gcc"


SYSCFG_DIR = $(BUILD)/syscfg

BOARD_H = $(SYSCFG_DIR)/ti_drivers_config.h
BOARD_C = $(SYSCFG_DIR)/ti_drivers_config.c

ALL_H = $(BOARD_H)
YSOURCES =  $(BOARD_C) main_freertos.c micropython_main.c \
			lib/utils/printf.c \
			lib/utils/stdout_helpers.c \
			lib/utils/pyexec.c \
			lib/libc/string0.c \
			lib/mp-readline/readline.c


PY_PORT_SOURCE =  $(BUILD)/_frozen_mpy.c


PY_PORT_O = $(patsubst %.c, $(BUILD)/%.o, $(PY_PORT_SOURCE))


OBJ = $(PY_CORE_O) $(PY_PORT_O) $(patsubst %.c, $(BUILD)/%.o, $(YSOURCES))



NAME = msp432

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)
INC += -I$(SYSCFG_DIR)
INC += -I$(SIMPLELINK_MSP432E4_SDK_INSTALL_DIR)/source
INC += -I$(SIMPLELINK_MSP432E4_SDK_INSTALL_DIR)/source/third_party/CMSIS/Include
INC += -I$(SIMPLELINK_MSP432E4_SDK_INSTALL_DIR)/source/ti/posix/gcc
INC += -I$(FREERTOS_INSTALL_DIR)/FreeRTOS/Source/include
INC += -I$(FREERTOS_INSTALL_DIR)/FreeRTOS/Source/portable/GCC/ARM_CM4F
INC += -I$(GCC_ARMCOMPILER)/arm-none-eabi/include/newlib-nano
INC += -I$(GCC_ARMCOMPILER)/arm-none-eabi/include
INC += -I$(FREERTOS_PROJECT)

CFLAGS_CPU = -mcpu=cortex-m4 -march=armv7e-m -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
CFLAGS_GLOBAL = -Og -ffunction-sections -fdata-sections -g -gdwarf-3 -gstrict-dwarf -Wall

CFLAGS =  $(CFLAGS_CPU) $(INC) $(CFLAGS_GLOBAL) -std=c99

LDFLAGS = $(CFLAGS_GLOBAL) $(CFLAGS_CPU) \
	-nostartfiles \
	-static \
	-Wl,--gc-sections \
	"-L$(SIMPLELINK_MSP432E4_SDK_INSTALL_DIR)/source" \
	"-L$(FREERTOS_PROJECT)" \
	"-L$(GCC_ARMCOMPILER)/arm-none-eabi/lib/thumb/v7e-m/fpv4-sp/hard" \
	"-L$(GCC_ARMCOMPILER)/arm-none-eabi/lib" \
	--specs=nano.specs



LIBS =-Wl,-T,MSP_EXP432E401Y_FREERTOS.lds \
	-l:ti/display/lib/display.am4fg \
	-l:ti/grlib/lib/gcc/m4f/grlib.a \
	-l:third_party/spiffs/lib/gcc/m4f/spiffs.a \
	-l:ti/drivers/lib/drivers_msp432e4.am4fg \
	-l:third_party/fatfs/lib/gcc/m4f/fatfs.a \
	-l:$(FREERTOS_LIB) \
	-l:ti/devices/msp432e4/driverlib/lib/gcc/m4f/msp432e4_driverlib.a \
	-lgcc \
	-lc \
	-lm \
	-lnosys


all: $(BUILD)/$(NAME).out Debug Debug/$(NAME).out

ifeq ($(OS),Windows_NT)

$(BUILD)/_frozen_mpy.c: frozentest.mpy $(BUILD)/genhdr/qstrdefs.generated.h
	$(ECHO) "MISC freezing bytecode"
	$(PYTHON) $(TOP)/tools/mpy-tool.py -f -q $(BUILD)/genhdr/qstrdefs.preprocessed.h -mlongint-impl=none $< > $@

endif

$(FREERTOS_PROJECT)$(FREERTOS_LIB):
	$(Q)$(MAKE) -C $(FREERTOS_PROJECT) $(FREERTOS_LIB)



$(BOARD_C): portable.syscfg
	@ echo Generating configuration files...
	@ $(SYSCONFIG_TOOL) -s $(SYSCONFIG_PRODUCT) -o $(@D) --compiler gcc $<


Debug/$(NAME).out $(BUILD)/$(NAME).out: $(BOARD_C) $(OBJ) $(FREERTOS_PROJECT)$(FREERTOS_LIB)
	@ echo linking $@
	$(Q)$(LNK) -Wl,-Map,$(patsubst %.out,%.map,$(@)) $(LDFLAGS) -o $@  $(OBJ)  $(LIBS)

Debug:
	$(Q)$(MKDIR) $(@)

.PHONY: micropython.lib
micropython.lib: $(PY_CORE_O) $(PY_PORT_O)
	@ echo build micropython core lib ($@)
	@ $(RM) -rf $@
	@ $(AR) -cr $@ $(PY_CORE_O) $(PY_PORT_O)



CLEAN_EXTRA = Debug

clean_all: clean
	$(Q)$(MAKE) -C $(FREERTOS_PROJECT) clean



include $(TOP)/py/mkrules.mk

vpath %.c . $(TOP) $(USER_C_MODULES)
$(BUILD)/%.o: %.c
	@ echo Building $@
	$(Q)$(CC) $(CFLAGS) $< -c -o $@
