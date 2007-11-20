#server/Makefile

TARGET=indicatord
INSTALL_BINDIR=$(INSTALL_DIR)/sbin

SRC=${shell ls *.c} 
OBJS=${SRC:.c=.o}

LIBS+=-lipc -lini -lhelper -lnano-X

include $(BASE_DIR)/Rules.make
