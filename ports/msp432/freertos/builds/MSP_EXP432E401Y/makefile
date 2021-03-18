include ../../../mkconf.mk

DRIVERLIB_DIR = $(SIMPLELINK_MSP432E4_SDK_INSTALL_DIR)
FREERTOSPORT_DIR = $(SIMPLELINK_MSP432E4_SDK_INSTALL_DIR)

VPATH = $(FREERTOS_INSTALL_DIR)/FreeRTOS/Source:$(FREERTOS_INSTALL_DIR)/FreeRTOS/Source/portable/GCC/ARM_CM4F:$(FREERTOS_INSTALL_DIR)/FreeRTOS/Source/portable/MemMang:$(SIMPLELINK_MSP432E4_SDK_INSTALL_DIR)/source:$(FREERTOSPORT_DIR)/kernel/freertos/dpl:$(FREERTOSPORT_DIR)/kernel/freertos/startup

CC = "$(GCC_ARMCOMPILER)/bin/arm-none-eabi-gcc"
AR = "$(GCC_ARMCOMPILER)/bin/arm-none-eabi-ar"


INC += -I.
INC += -I$(FREERTOS_INSTALL_DIR)/FreeRTOS/Source/include
INC += -I$(FREERTOS_INSTALL_DIR)/FreeRTOS/Source/portable/GCC/ARM_CM4F
INC += -I$(SIMPLELINK_MSP432E4_SDK_INSTALL_DIR)/source/ti/posix/gcc
INC += -I$(FREERTOSPORT_DIR)/source
INC += -I$(DRIVERLIB_DIR)/source
INC += -I$(DRIVERLIB_DIR)/source/third_party/CMSIS/Include
INC += -I$(GCC_ARMCOMPILER)/include

CFLAGS_CPU = -mcpu=cortex-m4 -march=armv7e-m -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
CFLAGS_GLOBAL = -Og -ffunction-sections -fdata-sections -g -gdwarf-3 -gstrict-dwarf -Wall

CFLAGS =  $(CFLAGS_CPU) $(INC) $(CFLAGS_GLOBAL) -std=c99


CFILES = \
	croutine.c \
	event_groups.c \
	list.c \
	queue.c \
	stream_buffer.c \
	tasks.c \
	timers.c \
	port.c \
	heap_4.c \
	ti/posix/freertos/clock.c \
	ti/posix/freertos/memory.c \
	ti/posix/freertos/mqueue.c \
	ti/posix/freertos/pthread_barrier.c \
	ti/posix/freertos/pthread_cond.c \
	ti/posix/freertos/pthread.c \
	ti/posix/freertos/pthread_mutex.c \
	ti/posix/freertos/pthread_rwlock.c \
	ti/posix/freertos/sched.c \
	ti/posix/freertos/semaphore.c \
	ti/posix/freertos/sleep.c \
	ti/posix/freertos/timer.c \
	ClockP_freertos.c \
	DebugP_freertos.c \
	MutexP_freertos.c \
	SemaphoreP_freertos.c \
	SystemP_freertos.c \
	HwiPMSP432E4_freertos.c \
	PowerMSP432E4_freertos.c \
	startup_msp432e4_gcc.c

OBJCFILES = $(subst /,_,$(CFILES:.c=.obj))

freertos.lib: $(OBJCFILES)
	@ echo ar $@ ...
	@$(RM) -rf $@
	@$(AR) -cr $@ $(OBJCFILES)



$(CFILES):
	@ echo "Error: can't find $@, please ensure FREERTOS_INSTALL_DIR in $(SIMPLELINK_MSP432E4_SDK_INSTALL_DIR)/imports.mak is correct (it is '$(FREERTOS_INSTALL_DIR)')"
	@ exit 1


# Create a CC rule for each source file, where the object file
# name contains the source file path (replacing / with _).
#
define CC_RULE
$(subst /,_,$(1)).obj: $(1).c
	@ echo cc $$@ ...
	$$(CC) $$(CFLAGS) -c $$< -o $$@
endef
$(foreach file,$(subst .c,,$(CFILES)),$(eval $(call CC_RULE,$(file))))

clean:
	@ echo cleaning ...
	@ echo $(RM)
	@ $(RM) -rf *.obj
	@ $(RM) -rf *.lib
