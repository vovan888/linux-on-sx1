
TARGET=t-hal-daemon
INSTALL_BINDIR=$(INSTALL_DIR)/sbin

target-y= t-hal.c
target-$(CONFIG_PLATFORM_SX1)		+= mach_sx1.c
target-$(CONFIG_PLATFORM_X86DEMO)	+= mach_x86pc.c

SRC=$(target-y)
OBJS=${SRC:.c=.o}

LIBS+=-ltpl -ltbus

include $(BASE_DIR)/Rules.make
