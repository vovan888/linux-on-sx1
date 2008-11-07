# libs/Makefile
# This will build all of the nessesary libraries

# Set the compiler targets for FLNX configure

export CC=$(TARGET_CROSS)gcc
export CXX=$(TARGET_CROSS)g++
export AR=$(TARGET_CROSS)ar
export NM=$(TARGET_CROSS)nm
export RANLIB=$(TARGET_CROSS)ranlib
export LD=$(TARGET_CROSS)ld

local-target-y=
local-target-$(CONFIG_FLNX)	+= flnx
local-target-$(CONFIG_GSM)	+= gsmd
local-target-$(CONFIG_FFMPEG)	+= ffmpeg

target-y=
target-$(CONFIG_FLGUI)		+= flgui
target-$(CONFIG_LIBFLPHONE)	+= libflphone
target-$(CONFIG_LIBTHEME)	+= libtheme
target-$(CONFIG_LIBTPL)		+= libtpl

# This will ensure that we only build one target at a time
DIRS=$(sort $(target-y))
LOCAL=$(sort $(local-target-y))

subdir-build = $(patsubst %,_subdir_%,$(DIRS))
subdir-clean = $(patsubst %,_clean_%,$(DIRS))
subdir-install = $(patsubst %,_install_%,$(DIRS))

local-build = $(patsubst %,_build_%,$(LOCAL))
local-clean = $(patsubst %,_clean_%,$(LOCAL))
local-install = $(patsubst %,_install_%,$(LOCAL))

all: $(local-build) $(subdir-build)
clean: $(local-clean) $(subdir-clean)
install: $(local-install) $(subdir-install)

$(subdir-build): dummy
	@ $(MAKE) -C $(patsubst _subdir_%,%,$@)

$(subdir-clean): dummy
	@ $(MAKE) -C $(patsubst _clean_%,%,$@) clean

$(subdir-install): dummy
	@ $(MAKE) -C $(patsubst _install_%,%,$@) install


###### Local targets #######

FLNX_DEBUG=no
ifeq ($(CONFIG_FLNX_DEBUG),y)
FLNX_DEBUG=yes
endif

$(BASE_DIR)/libs/flnx/.configured: 
	(cd $(BASE_DIR)/libs/flnx; ./configure \
	--host=$(SYS) \
	--build=$(BUILD_SYS) \
	--enable-shared=yes \
	--enable-gl=no \
	--enable-debug=$(FLNX_DEBUG) \
	--with-microwin=$(strip $(subst ",, $(MW_PREFIX))))
	touch $(BASE_DIR)/libs/flnx/.configured

_build_flnx: $(BASE_DIR)/libs/flnx/.configured
	make -C $(BASE_DIR)/libs/flnx/src -e -j3

	rm -f $(STAGE_DIR)/lib/libfltk.a $(STAGE_DIR)/lib/libfltk.so*
	ln -fs $(BASE_DIR)/libs/flnx/lib/libfltk.a $(STAGE_DIR)/lib/libfltk.a
	ln -fs $(BASE_DIR)/libs/flnx/src/libfltk.so.1.1 $(STAGE_DIR)/lib/libfltk.so
	ln -fs $(BASE_DIR)/libs/flnx/src/libfltk.so.1.1 $(STAGE_DIR)/lib/libfltk.so.1.1

_install_flnx: dummy
	cp -fP flnx/src/libfltk.so* $(INSTALL_DIR)/lib/
	cp -fP flnx/src/libfltk_images.so* $(INSTALL_DIR)/lib/

_clean_flnx: dummy
	if [ -f $(BASE_DIR)/libs/flnx/makeinclude ]; then \
		make -C $(BASE_DIR)/libs/flnx distclean; \
	fi
	rm -f $(BASE_DIR)/libs/flnx/.configured

$(BASE_DIR)/libs/gsm/.configured: 
	(cd $(BASE_DIR)/libs/gsm; ./autogen.sh \
	./configure \
	--host=$(SYS) \
	--build=$(BUILD_SYS) \
	--libdir=/usr/flphone/ \
	--enable-shared )
	touch $(BASE_DIR)/libs/gsm/.configured

_build_gsmd: $(BASE_DIR)/libs/gsm/.configured
	make -C $(BASE_DIR)/libs/gsm/src -e -j3

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


FFMPEG_CROSS=
ifeq ($(CROSS_COMPILE),y)
FFMPEG_CROSS= --arch=arm --enable-cross-compile --cross-prefix=$(CROSS_COMPILER_PREFIX)
endif

$(BASE_DIR)/libs/ffmpeg/.configured: 
	(cd $(BASE_DIR)/libs/ffmpeg; ./configure \
	--enable-static \
	--enable-shared \
	$(FFMPEG_CROSS) \
	--disable-armv5te \
	--disable-armv6 \
	--disable-iwmmxt \
	--disable-ipv6 \
	--disable-mpegaudio-hp \
	--disable-ffmpeg \
	--disable-ffserver \
	--enable-hardcoded-tables \
	--disable-decoders \
	--enable-decoder=mp3 \
	--enable-decoder=vorbis \
	--enable-decoder=adpcm_ima_wav \
	--disable-encoders \
	--disable-muxers \
	--disable-demuxers \
	--enable-demuxer=avi \
	--enable-demuxer=flv \
	--enable-demuxer=mp3 \
	--enable-demuxer=mp4 \
	--enable-demuxer=ogg \
	--enable-demuxer=wav \
	--disable-bsfs \
	--disable-devices \
	--disable-filters \
	)
	touch $(BASE_DIR)/libs/ffmpeg/.configured

_build_ffmpeg: $(BASE_DIR)/libs/ffmpeg/.configured
	make -C $(BASE_DIR)/libs/ffmpeg -e -j3

	rm -f $(STAGE_DIR)/lib/libfltk.a $(STAGE_DIR)/lib/libfltk.so*
	ln -fs $(BASE_DIR)/libs/ffmpeg/lib/libfltk.a $(STAGE_DIR)/lib/libfltk.a
	ln -fs $(BASE_DIR)/libs/ffmpeg/src/libfltk.so.1.1 $(STAGE_DIR)/lib/libfltk.so
	ln -fs $(BASE_DIR)/libs/ffmpeg/src/libfltk.so.1.1 $(STAGE_DIR)/lib/libfltk.so.1.1

_install_ffmpeg: dummy
	cp -fP ffmpeg/src/libfltk.so* $(INSTALL_DIR)/lib/
	cp -fP ffmpeg/src/libfltk_images.so* $(INSTALL_DIR)/lib/

_clean_ffmpeg: dummy
	if [ -f $(BASE_DIR)/libs/ffmpeg/makeinclude ]; then \
		make -C $(BASE_DIR)/libs/ffmpeg distclean; \
	fi
	rm -f $(BASE_DIR)/libs/ffmpeg/.configured


dummy:
