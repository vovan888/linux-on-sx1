#ipc_dsy

TARGET=ipc_dsy
INSTALL_BINDIR=$(INSTALL_DIR)/sbin

SRC=${shell ls *.c} 
OBJS=${SRC:.c=.o}

LIBS= -ltbus -lflphone -ltpl

include $(BASE_DIR)/Rules.make

