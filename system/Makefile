# system/Makefile

target-y=

#note! sequence dependent - tbus should be listed first.
target-$(CONFIG_TBUS) += tbus
target-$(CONFIG_THAL) += t-hal
target-$(CONFIG_NANOWM) += nanowm
target-$(CONFIG_DAEMONS) += daemons
target-$(CONFIG_GSMMUX) += gsmmux
target-$(CONFIG_GSMD2) += gsmd2

target-y += platform

DIRS=$(target-y)
#DIRS=$(sort $(target-y))

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

dummy:

