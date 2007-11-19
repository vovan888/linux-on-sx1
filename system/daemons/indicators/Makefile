#server/Makefile

TARGET=srv_indicators
INSTALL_BINDIR=$(INSTALL_DIR)/sbin

SRC=${shell ls *.c} 
OBJS=${SRC:.c=.o}

ifeq ($(CONFIG_PAR),y)
LIBS+=-lpar
endif

ifeq ($(CONFIG_COLOSSEUM),y)
LIBS+=-lipc
endif

LIBS += -ldl

include $(BASE_DIR)/Rules.make
