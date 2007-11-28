# apps/test/Makefile

TARGET_CXX=dialer

SRC=${shell ls *.cxx}
OBJS=${SRC:.cxx=.o}

LIBS+=-lgsmd

ifeq ($(CONFIG_PAR),y)
LIBS+=-lpar
endif

ifeq ($(CONFIG_SYNC),y)
LIBS+=-lpixil-sync
endif

LIBS+=-lfltk

ifeq ($(CONFIG_NANOX),y)
LIBS+=-lnano-X
endif

ifeq ($(CONFIG_X11),y)
LIBS+=-lX11
endif

ifeq ($(CONFIG_COLOSSEUM),y)
LIBS+=-lipc
endif

LIBS+=$(LDADD)

include $(BASE_DIR)/Rules.make
