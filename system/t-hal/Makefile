
TARGET=t-hal
INSTALL_BINDIR=$(INSTALL_DIR)/sbin

target-y= t-hal.c t-hal-ipc.c
target-$(CONFIG_PLATFORM_SX1)		+= mach_sx1.c
target-$(CONFIG_PLATFORM_X86DEMO)	+= mach_x86pc.c
target-$(CONFIG_PLATFORM_GTA02)		+= mach_gta02.c

SRC=$(target-y)
OBJS=${SRC:.c=.o}

LIBS+=-ltpl -ltbus -lflphone

include $(BASE_DIR)/Rules.make
