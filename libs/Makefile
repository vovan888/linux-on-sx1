# libs/Makefile
# This will build all of the nessesary libraries

# Set the compiler targets for FLNX configure

export CC=$(TARGET_CROSS)gcc
export CXX=$(TARGET_CROSS)g++
export AR=$(TARGET_CROSS)ar
export NM=$(TARGET_CROSS)nm
export RANLIB=$(TARGET_CROSS)ranlib
export LD=$(TARGET_CROSS)ld

# FLNX, libini, libgsm are always built
local-target-y=libini gsmd
local-target-$(CONFIG_FLNX) += flnx

target-y=libflphone libtheme

target-$(CONFIG_PIM_ADDRESS) += pim
target-$(CONFIG_PIM_NOTEPAD) += pim
target-$(CONFIG_PIM_TODO) += pim
target-$(CONFIG_PIM_SCHEDULE) += pim

target-$(CONFIG_APP_ALARM) += pim
target-$(CONFIG_APP_CALC) += pim
target-$(CONFIG_APP_MAIL) += pim
target-$(CONFIG_APP_SYSCONFIG) += pixlib pim
target-$(CONFIG_SYNC) += pim sync

ifeq ($(CONFIG_NANOX),y)
target-$(CONFIG_APP_ALARM) += wm
target-$(CONFIG_PIXILWM) += pixlib wm xml
target-$(CONFIG_APP_NETCONFIG) += pixlib wm
target-$(CONFIG_NANOX_PIXCAL) += pixlib
endif

# This will ensure that we only build one target at a time
DIRS=$(sort $(target-y))
LOCAL=$(sort $(local-target-y))

subdir-build = $(patsubst %,_subdir_%,$(DIRS))
subdir-clean = $(patsubst %,_clean_%,$(DIRS))
subdir-install = $(patsubst %,_install_%,$(DIRS))

local-build = $(patsubst %,_build_%,$(LOCAL))
local-clean = $(patsubst %,_clean_%,$(LOCAL))
local-install = $(patsubst %,_install_%,$(LOCAL))

all: $(subdir-build) $(local-build)
clean: $(subdir-clean) $(local-clean)
install: $(subdir-install) $(local-install)

$(subdir-build): dummy
	@ $(MAKE) -C $(patsubst _subdir_%,%,$@)

$(subdir-clean): dummy
	@ $(MAKE) -C $(patsubst _clean_%,%,$@) clean

$(subdir-install): dummy
	@ $(MAKE) -C $(patsubst _install_%,%,$@) install


###### Local targets #######

FLNX_DEBUG=no
ifeq ($(CONFIG_DEBUG),y)
FLNX_DEBUG=yes
endif

$(BASE_DIR)/libs/flnx/.configured: 
	(cd $(BASE_DIR)/libs/flnx; ./configure \
	--host=$(SYS) \
        --build=$(BUILD_SYS) \
	--enable-shared \
	--enable-gl=no \
	--enable-debug=$(FLNX_DEBUG) \
	--with-microwin=$(strip $(subst ",, $(MW_PREFIX))))
	touch $(BASE_DIR)/libs/flnx/.configured

_build_flnx: $(BASE_DIR)/libs/flnx/.configured
	make -C $(BASE_DIR)/libs/flnx/src -e 

	rm -f $(STAGE_DIR)/lib/libfltk.a $(STAGE_DIR)/lib/libfltk.so*
	ln -s $(BASE_DIR)/libs/flnx/lib/libfltk.a $(STAGE_DIR)/lib/libfltk.a
	ln -s $(BASE_DIR)/libs/flnx/src/libfltk.so.1.1 $(STAGE_DIR)/lib/libfltk.so
	ln -s $(BASE_DIR)/libs/flnx/src/libfltk.so.1.1 $(STAGE_DIR)/lib/libfltk.so.1.1

_install_flnx: dummy
	cp -fP flnx/src/libfltk.so* $(INSTALL_DIR)/lib/

_clean_flnx: dummy
	if [ -f $(BASE_DIR)/libs/flnx/makeinclude ]; then \
		make -C $(BASE_DIR)/libs/flnx distclean; \
	fi
	rm -f $(BASE_DIR)/libs/flnx/.configured

$(BASE_DIR)/libs/libini/.configured: 
	(cd $(BASE_DIR)/libs/libini; ./configure \
	--host=$(SYS) \
	--build=$(BUILD_SYS) \
	--enable-shared )
	touch $(BASE_DIR)/libs/libini/.configured

_build_libini: $(BASE_DIR)/libs/libini/.configured
	make -C $(BASE_DIR)/libs/libini -e 

	rm -f $(STAGE_DIR)/lib/libini.a $(STAGE_DIR)/lib/libini.so*
	ln -s $(BASE_DIR)/libs/libini/src/.libs/libini.a $(STAGE_DIR)/lib
	ln -s $(BASE_DIR)/libs/libini/src/.libs/libini.so $(STAGE_DIR)/lib
	ln -s $(BASE_DIR)/libs/libini/src/.libs/libini.so $(STAGE_DIR)/lib/libini.so.1

_install_libini: dummy
	cp -fP libini/src/.libs/libini.so* $(INSTALL_DIR)/lib/

_clean_libini: dummy
	make -C $(BASE_DIR)/libs/libini distclean
	rm -f $(BASE_DIR)/libs/libini/.configured
	
$(BASE_DIR)/libs/gsm/.configured: 
	(cd $(BASE_DIR)/libs/gsm; ./autogen.sh \
	./configure \
	--host=$(SYS) \
	--build=$(BUILD_SYS) \
	--libdir=/usr/flphone/ \
	--enable-shared )
	touch $(BASE_DIR)/libs/gsm/.configured

_build_gsmd: $(BASE_DIR)/libs/gsm/.configured
	make -C $(BASE_DIR)/libs/gsm/src -e 

	rm -f $(STAGE_DIR)/lib/libgsmd.a $(STAGE_DIR)/lib/libgsmd.so*
	ln -s $(BASE_DIR)/libs/gsm/src/libgsmd/.libs/libgsmd.a $(STAGE_DIR)/lib
	ln -s $(BASE_DIR)/libs/gsm/src/libgsmd/.libs/libgsmd.so $(STAGE_DIR)/lib
	ln -s $(BASE_DIR)/libs/gsm/src/libgsmd/.libs/libgsmd.so $(STAGE_DIR)/lib/libgsmd.so.1

_install_gsmd: dummy
	cp -fP gsm/src/libgsmd/.libs/libgsmd.so* $(INSTALL_DIR)/lib/
	cp -fP gsm/src/gsmd/gsmd $(INSTALL_DIR)/sbin
	cp -fP gsm/src/util/.libs/libgsmd-tool $(INSTALL_DIR)/sbin
	cp -fP gsm/src/gsmd/.libs/*.so* $(INSTALL_DIR)/lib

_clean_gsmd: dummy
	make -C $(BASE_DIR)/libs/gsm distclean
	rm -f $(BASE_DIR)/libs/gsm/.configured	

dummy:
