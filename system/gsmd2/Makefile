#CFLAGS ?=

INCLUDES += 
INSTALL_BINDIR=$(INSTALL_DIR)/sbin
TARGET=gsmd2
#CFLAGS += -D PLUGINDIR=\"$(INSTALL_DIR)/lib\"

OBJS-$(CONFIG_PLATFORM_X86DEMO)	+= plugins/machine_generic.o
OBJS-$(CONFIG_PLATFORM_SX1) 	+= plugins/machine_sx1.o
OBJS-$(CONFIG_PLATFORM_GTA02)	+= plugins/machine_gta02.o
#TARGET_EXTRAS += plugins/libgsmd-machine_tihtc.so 
#TARGET_EXTRAS += plugins/libgsmd-vendor_qc.so 
#TARGET_EXTRAS += plugins/libgsmd-vendor_telit.so 
#TARGET_EXTRAS += plugins/libgsmd-vendor_tihtc.so
#TARGET_EXTRAS += plugins/libgsmd-machine_gta01.so
#TARGET_EXTRAS += plugins/libgsmd-machine_telit.so
#TARGET_EXTRAS += plugins/libgsmd-vendor_bcm.so
OBJS-$(CONFIG_PLATFORM_X86DEMO)	+= plugins/vendor_generic.o
OBJS-$(CONFIG_PLATFORM_SX1)	+= plugins/vendor_siemens.o
OBJS-$(CONFIG_PLATFORM_GTA02)	+= plugins/vendor_ti.o

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
