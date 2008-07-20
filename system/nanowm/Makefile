#Makefile

TARGET=nanowm
INSTALL_BINDIR=$(INSTALL_DIR)/sbin

SRC=${shell ls *.c} 
OBJS=${SRC:.c=.o}

LIBS= -lnano-X

# libs usage
LIBS+=-lini -lflphone -ltheme -ltbus -ltpl

include $(BASE_DIR)/Rules.make

