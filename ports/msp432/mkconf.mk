SIMPLELINK_MSP432E4_SDK_INSTALL_DIR ?= c:\\ti\\simplelink_msp432e4_sdk_4_20_00_12
FREERTOS_INSTALL_DIR   ?= E:\\data\\yoctopuce\\yoctoprod\\FreeRTOS\\FreeRTOSv10.2.1
CCS_INSTALL_DIR        ?= c:\\ti\\ccs901\\ccs
SYSCONFIG_TOOL         ?= $(CCS_INSTALL_DIR)/utils/sysconfig/cli.js
GCC_ARMCOMPILER        ?= $(CCS_INSTALL_DIR)/tools/compiler/gcc-arm-none-eabi-7-2017-q4-major-win32

PYTHON = python.exe

ifeq ($(OS),Windows_NT)
XDCTOOLS ?= $(CCS_INSTALL_DIR)\\xdctools_3_55_00_11_core\\bin\\
CAT = "$(XDCTOOLS)cat"
RM = "$(XDCTOOLS)rm"
MKDIR = "$(XDCTOOLS)mkdir"
SED = "$(XDCTOOLS)sed"
TOUCH = "$(XDCTOOLS)touch"
AR = "$(GCC_ARMCOMPILER)/bin/arm-none-eabi-ar"
NM = "$(GCC_ARMCOMPILER)/bin/arm-none-eabi-nm"

endif


