# apps/test/Makefile

TARGET_CXX=menu

SRC=${shell ls *.c}
OBJS=${SRC:.c=.o}

LIBS+=-lflphone -lini

#LIBS+=-lfltk

ifeq ($(CONFIG_NANOX),y)
LIBS+=-lnano-X
endif

ifeq ($(CONFIG_COLOSSEUM),y)
LIBS+=-lipc
endif

include $(BASE_DIR)/Rules.make
