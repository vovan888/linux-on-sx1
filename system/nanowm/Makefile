#Makefile

TARGET=nanowm
INSTALL_BINDIR=$(INSTALL_DIR)/sbin

SRC=${shell ls *.c} 
OBJS=${SRC:.c=.o}

LIBS= -lnano-X

# libini usage
LIBS+=-lini -lhelper -lipc

include $(BASE_DIR)/Rules.make

