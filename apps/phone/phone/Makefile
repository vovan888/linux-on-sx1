# apps/phone/Makefile

TARGET_CXX=phone

SRC=${shell ls *.cpp}
OBJS=${SRC:.cpp=.o}

SRC_C=${shell ls *.c}
OBJS+=${SRC_C:.c=.o}


LIBS+= -lfltk -lgsmd -ltbus -ltpl

ifeq ($(CONFIG_NANOX),y)
LIBS+=-lnano-X
endif

ifeq ($(CONFIG_X11),y)
LIBS+=-lX11
endif

include $(BASE_DIR)/Rules.make
