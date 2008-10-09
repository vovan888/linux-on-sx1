#CFLAGS ?=

INCLUDES += 
INSTALL_BINDIR=$(INSTALL_DIR)/sbin
TARGET=gsmd2
#CFLAGS += -D PLUGINDIR=\"$(INSTALL_DIR)/lib\"

#TARGET_EXTRAS += plugins/libgsmd-machine_generic.so
OBJS-y += plugins/machine_sx1.o
#TARGET_EXTRAS += plugins/libgsmd-machine_tihtc.so 
#TARGET_EXTRAS += plugins/libgsmd-vendor_qc.so 
#TARGET_EXTRAS += plugins/libgsmd-vendor_telit.so 
#TARGET_EXTRAS += plugins/libgsmd-vendor_tihtc.so
#TARGET_EXTRAS += plugins/libgsmd-machine_gta01.so
#TARGET_EXTRAS += plugins/libgsmd-machine_telit.so
#TARGET_EXTRAS += plugins/libgsmd-vendor_bcm.so
OBJS-y += plugins/vendor_siemens.o
#TARGET_EXTRAS += plugins/libgsmd-vendor_ti.so

#INSTALL_EXTRAS=inst_plugins
#CLEAN_EXTRAS=clean_plugins

OBJS-y += gsmd.o atcmd.o select.o machine.o vendor.o unsolicited.o log.o \
         usock_tbus.o talloc.o timer.o operator_cache.o ext_response.o \
         sms_cb.o sms_pdu.o strl.o

OBJS := $(OBJS-y)

LIBS+= -ltbus -ltpl -lflphone

#LIBS+= -ldl
#BUILD_CFLAGS=-rdynamic


include $(BASE_DIR)/Rules.make
