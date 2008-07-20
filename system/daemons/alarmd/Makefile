#alarmd/Makefile

TARGET=alarmserver
INSTALL_BINDIR=$(INSTALL_DIR)/sbin

SRC=${shell ls *.c} 
OBJS=${SRC:.c=.o}

LIBS+=-lini -lflphone -ltbus -ltpl

include $(BASE_DIR)/Rules.make
