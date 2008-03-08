# phone/Makefile

target-y=

target-$(CONFIG_DIALER) += dialer
target-$(CONFIG_INCOMING_CALL) += incoming
target-$(CONFIG_CALL_LOGS) += logs
target-$(CONFIG_MESSAGES) += messages
target-$(CONFIG_MESSAGES) += smseditor
target-$(CONFIG_MESSAGES) += smsviewer
#target-$(CONFIG_MESSAGES) += mmseditor
#target-$(CONFIG_MESSAGES) += mmsviewer
#target-$() += 
target-$(CONFIG_MENUPUSK) += menupusk
target-$(CONFIG_MAINMENU) += menu
target-$(CONFIG_TEST) += test

# This will ensure that we only build one target at a time
DIRS=$(sort $(target-y))

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

