# ipc/Makefile
# This will build all of the nessesary libraries

DIRS=client server

subdir-build = $(patsubst %,_subdir_%,$(DIRS))
subdir-clean = $(patsubst %,_clean_%,$(DIRS))
subdir-install = $(patsubst %,_install_%,$(DIRS))

all: $(subdir-build)
clean: $(subdir-clean)
install: $(subdir-install)

$(subdir-build): dummy
	@ $(MAKE) -C $(patsubst _subdir_%,%,$@)

$(subdir-clean): dummy
	@ $(MAKE) -C $(patsubst _clean_%,%,$@) clean

$(subdir-install): dummy
	@ $(MAKE) -C $(patsubst _install_%,%,$@) install


$(BASE_DIR)/system/ipc/client/.configured: 
	 cd $(BASE_DIR)/system/ipc/client
#(cd $(BASE_DIR)/libs/ipc; ./configure \
#--host=$(SYS) \
#--build=$(BUILD_SYS) \
#--enable-shared )
	touch $(BASE_DIR)/system/ipc/client/.configured

_build_ipc: $(BASE_DIR)/system/ipc/client/.configured
	make -C $(BASE_DIR)/system/ipc/client/ -e 

	rm -f $(STAGE_DIR)/lib/ipc.a $(STAGE_DIR)/lib/libipc.so*
	ln -s $(BASE_DIR)/system/ipc/client/libipc.a $(STAGE_DIR)/lib
	ln -s $(BASE_DIR)/system/ipc/client/libipc.so $(STAGE_DIR)/lib

_install_ipc: dummy
	cp -fP client/libipc.so* $(INSTALL_DIR)/lib/

_clean_ipc: dummy
	if [ -f $(BASE_DIR)/system/ipc/client/makeinclude ]; then \
	    make -C $(BASE_DIR)/system/ipc/client distclean; \
	fi
	rm -f $(BASE_DIR)/system/ipc/client/.configured

dummy:

