# apps/phone/Makefile

TARGET_CXX=phone

SRC=${shell ls *.cpp}
OBJS=${SRC:.cpp=.o}

#SRC_C=${shell ls *.c}
#OBJS+=${SRC_C:.c=.o}


LIBS+= -lfltk -ltbus -ltpl -lflgui -lflphone -ltheme

ifeq ($(CONFIG_NANOX),y)
LIBS+=-lnano-X
endif

include $(BASE_DIR)/Rules.make
