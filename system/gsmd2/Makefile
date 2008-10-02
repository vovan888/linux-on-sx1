CFLAGS ?=

INCLUDES += 
INSTALL_BINDIR=$(INSTALL_DIR)/sbin
TARGET=gsmd2
CFLAGS += -D PLUGINDIR=\"$(INSTALL_DIR)/lib\"

#TARGET_EXTRAS += plugins/libgsmd-machine_generic.so
TARGET_EXTRAS += plugins/libgsmd-machine_sx1.so
#TARGET_EXTRAS += plugins/libgsmd-machine_tihtc.so 
#TARGET_EXTRAS += plugins/libgsmd-vendor_qc.so 
#TARGET_EXTRAS += plugins/libgsmd-vendor_telit.so 
#TARGET_EXTRAS += plugins/libgsmd-vendor_tihtc.so
#TARGET_EXTRAS += plugins/libgsmd-machine_gta01.so
#TARGET_EXTRAS += plugins/libgsmd-machine_telit.so
#TARGET_EXTRAS += plugins/libgsmd-vendor_bcm.so
TARGET_EXTRAS += plugins/libgsmd-vendor_siemens.so
#TARGET_EXTRAS += plugins/libgsmd-vendor_ti.so

INSTALL_EXTRAS=inst_plugins
CLEAN_EXTRAS=clean_plugins

OBJS-y = gsmd.o atcmd.o select.o machine.o vendor.o unsolicited.o log.o \
         usock_tbus.o talloc.o timer.o operator_cache.o ext_response.o \
         sms_cb.o sms_pdu.o strl.o

OBJS := $(OBJS-y)

LIBS+= -ltbus -ltpl -lflphone

LIBS+= -ldl
BUILD_CFLAGS=-rdynamic


include $(BASE_DIR)/Rules.make

clean_plugins:
	rm -rf plugins/*.o
	rm -rf plugins/*.so

inst_plugins:
	mkdir -p $(INSTALL_DIR)/lib
	cp plugins/*.so $(INSTALL_DIR)/lib/

plugins/libgsmd-vendor_siemens.so: plugins/vendor_siemens.o
	$(CC) -shared -o $@ $<

plugins/libgsmd-machine_sx1.so: plugins/machine_sx1.o
	$(CC) -shared -o $@ $<

plugins/libgsmd-machine_generic.so: plugins/machine_generic.o
	$(CC) -shared -o $@ $<
