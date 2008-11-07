# apps/phone/Makefile

TARGET_CXX=pin-enter

SRC=${shell ls *.cpp}
OBJS=${SRC:.cpp=.o}

LIBS+= -lfltk -lflgui -ltbus -ltpl -ltheme -lflphone

ifeq ($(CONFIG_NANOX),y)
LIBS+=-lnano-X
endif

ifeq ($(CONFIG_X11),y)
LIBS+=-lX11
endif

include $(BASE_DIR)/Rules.make
