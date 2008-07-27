# apps/test/Makefile

TARGET_CXX=dialer

SRC=${shell ls *.cxx}
OBJS=${SRC:.cxx=.o}

LIBS+=-lgsmd

LIBS+=-lfltk

ifeq ($(CONFIG_NANOX),y)
LIBS+=-lnano-X
endif

ifeq ($(CONFIG_X11),y)
LIBS+=-lX11
endif

LIBS+=$(LDADD)

include $(BASE_DIR)/Rules.make
