# contacts

TARGET_APP=contacts

SRC=${shell ls *.cpp}
OBJS=${SRC:.cpp=.o}

LIBS+= -lfltk -ltbus -ltpl -lflgui -lflphone -ltheme

ifeq ($(CONFIG_NANOX),y)
LIBS+=-lnano-X
endif

include $(BASE_DIR)/Rules.make
