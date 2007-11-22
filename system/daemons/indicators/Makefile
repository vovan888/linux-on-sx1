#server/Makefile

TARGET=indicatord
INSTALL_BINDIR=$(INSTALL_DIR)/sbin

SRC=${shell ls *.c} 
OBJS=${SRC:.c=.o}

LIBS+=-lnano-X -lipc -lini -lhelper

include $(BASE_DIR)/Rules.make
