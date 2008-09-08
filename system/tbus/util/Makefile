#tbus/util/Makefile

TARGET=tbus-send
INSTALL_BINDIR=$(INSTALL_DIR)/sbin

SRC=${shell ls *.c} 
OBJS=${SRC:.c=.o}

LIBS+= -static -ltpl -ltbus

include $(BASE_DIR)/Rules.make
