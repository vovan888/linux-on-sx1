# apps/test/Makefile

TARGET_CXX=test

SRC=${shell ls *.cxx}
OBJS=${SRC:.cxx=.o}

LIBS=-lfltk -lflgui -ltbus -ltpl

ifeq ($(CONFIG_NANOX),y)
LIBS+=-lnano-X
endif

ifeq ($(CONFIG_X11),y)
LIBS+=-lX11
endif

include $(BASE_DIR)/Rules.make
