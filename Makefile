####
# Adapted from
# Pixil Makefile
# Copyright 2002, Century Software
#
# Written by:  Jordan Crouse
# Released under the GPL (see LICENSE.GPL) for details
####

-include config

# Bring in the configuration file

BASE_DIR    = ${shell pwd}
STAGE_DIR   = $(BASE_DIR)/build
INCLUDE_DIR = $(BASE_DIR)/include
TOOL_DIR    = $(BASE_DIR)/scripts/tools
IMAGES_DIR  = $(BASE_DIR)/data/images
MCONFIG_DIR = $(BASE_DIR)/scripts/config
SYSDEP_DIR  = $(BASE_DIR)/scripts/sysdep

# The base directory for installing
#ROOT_DIR = $(strip $(subst ",, $(INSTALL_PREFIX)))

# install flfphone to the build/image
ROOT_DIR = $(BASE_DIR)/build/image

ifdef CONFIG_PLATFORM_X86DEMO
INSTALL_DIR = $(ROOT_DIR)
endif
ifdef CONFIG_PLATFORM_SX1
INSTALL_DIR = $(ROOT_DIR)-sx1
endif
ifdef CONFIG_PLATFORM_GTA02
INSTALL_DIR = $(ROOT_DIR)-gta02
endif

MAKEFILES = $(BASE_DIR)/config

# The cross compiler 
TARGET_CROSS=$(subst ",, $(strip $(CROSS_COMPILER_PREFIX)))

# The system building the kernel (we're going to assume)
# Intel until somebody proves us wrong

BUILD_SYS=i386-linux

ifeq ($(CROSS_COMPILE),y)
#ifeq ($(CONFIG_TARGET_ARM),y)
SYS=arm-linux
#endif
ifeq ($(CONFIG_TARGET_I386_UCLIBC),y)
SYS=i386-uclibc-linux
endif
else
SYS=$(BUILD_SYS)
endif

export TARGET_CROSS BASE_DIR STAGE_DIR INCLUDE_DIR PAR_DB PAR_CONFIG 
export TOOL_DIR INSTALL_DIR ROOT_DIR SYS BUILD_SYS

# Set up the list of directories based on the configuration
ifdef SUBDIRS
DIRS-y=$(SUBDIRS)
else
DIRS-y=libs system apps
endif

subdir-build = $(patsubst %,_subdir_%,$(DIRS-y))
subdir-clean = $(patsubst %,_clean_%,$(DIRS-y))
subdir-install = $(patsubst %,_install_%,$(DIRS-y))

# This avoids stripping the debugging information when we choose debugging 

STRIP_TARGET=

ifneq ($(CONFIG_DEBUG),y)
STRIP_TARGET=dostrip
endif

##### Toplevel targets #####

all: local-build
install: local-install 
clean: local-clean

distclean: local-clean packages-clean
	@ rm -rf config

##### Local targets #####

local-build: $(STAGE_DIR) $(INCLUDE_DIR)/flphone_config.h $(subdir-build) platform-build

local-install: $(INSTALL_DIR) $(subdir-install) $(STRIP_TARGET) platform-install
	@ mkdir -p $(INSTALL_DIR)/share/images
#	@ cp -r $(IMAGES_DIR)/* $(INSTALL_DIR)/share/images

local-clean:  $(subdir-clean) platform-clean
	@ rm -rf $(STAGE_DIR) $(INCLUDE_DIR)/flphone_config.h
	@ make -C scripts/config/ clean
 
dostrip:
	@ echo "Stripping binaries..."

	@ for fo in `find $(INSTALL_DIR) -type f -perm +111`; do \
		if file $$fo | grep "ELF" > /dev/null; then \
		$(TARGET_CROSS)strip $$fo; \
		fi; \
	done

$(subdir-build): dummy
	@ $(MAKE) -C $(patsubst _subdir_%,%,$@)

$(subdir-clean): dummy
	@ $(MAKE) -C $(patsubst _clean_%,%,$@) clean

$(subdir-install): dummy
	@ $(MAKE) -C $(patsubst _install_%,%,$@) install

platform-build:
	@ make -C scripts/platforms/ 

platform-clean:
	@ make -C scripts/platforms/ clean

platform-install: 
	@ make -C scripts/platforms/ install

packages:
	@ make -C packages

packages-clean:
	@ make -C packages packages-clean

$(STAGE_DIR):
	@ mkdir -p $(STAGE_DIR)/lib

$(INSTALL_DIR):
	@ mkdir -p $(INSTALL_DIR)/lib
	@ mkdir -p $(INSTALL_DIR)/bin
	@ mkdir -p $(INSTALL_DIR)/sbin
	@ mkdir -p $(INSTALL_DIR)/etc
	@ mkdir -p $(INSTALL_DIR)/share
	@ mkdir -p $(INSTALL_DIR)/share/data

$(INCLUDE_DIR)/flphone_config.h: $(BASE_DIR)/config
	@ perl $(TOOL_DIR)/config.pl $(BASE_DIR)/config \
	> $(INCLUDE_DIR)/flphone_config.h	

# configuration
# ------------------------------

$(MCONFIG_DIR)/conf:
	make -C $(MCONFIG_DIR) conf

$(MCONFIG_DIR)/mconf:
	make -C $(MCONFIG_DIR) conf mconf

menuconfig: $(BASE_DIR)/config $(MCONFIG_DIR)/mconf
	@$(MCONFIG_DIR)/mconf $(SYSDEP_DIR)/Config.in

textconfig: $(BASE_DIR)/config $(MCONFIG_DIR)/conf
	@$(MCONFIG_DIR)/conf $(SYSDEP_DIR)/Config.in

oldconfig: $(MCONFIG_DIR)/conf
	@$(MCONFIG_DIR)/conf -o $(SYSDEP_DIR)/Config.in

defconfig: $(BASE_DIR)/config 

$(BASE_DIR)/config:
	@ cp $(SYSDEP_DIR)/defconfig $(BASE_DIR)/config

defconfig_host:
	@ cp -f $(SYSDEP_DIR)/defconfig.host $(BASE_DIR)/config

defconfig_sx1:
	@ cp -f $(SYSDEP_DIR)/defconfig.sx1 $(BASE_DIR)/config

dummy:

.PHONY: menuconfig textconfig oldconfig defconfig dummy packages 

.PHONY: doc doc-clean

doc:
	doxygen Doxyfile
doc-clean:
	rm -rf ./doc/html
