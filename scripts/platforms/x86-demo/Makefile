# Makefile to set up the X86 DEMO platform

# The default run_flphone.sh script template
RUN_TEMPLATE=run_flphone.sh.in

# A set of directories that we want to build

SYSLOG_DIR=$(INSTALL_DIR)/var/log
NETSCRIPT_DIR=$(INSTALL_DIR)/share/scripts
HTML_DIR=$(INSTALL_DIR)/share/html
MP3_DIR=$(INSTALL_DIR)/share/mp3
MPEG_DIR=$(INSTALL_DIR)/share/media
FONT_DIR=$(INSTALL_DIR)/share/fonts
THEME_DIR=$(INSTALL_DIR)/share/activetheme
ETC_DIR=$(INSTALL_DIR)/etc

INSTALL_DIRS=$(SYSLOG_DIR) $(NETSCRIPT_DIR) 
ifeq ($(CONFIG_SAMPLES),y)
SAMPLE_TARGET=copy-samples
endif

ifeq ($(CONFIG_176x220),y)
THEME_SRC=$(BASE_DIR)/data/176x220/activetheme
NXSCREENSIZE="-x 176 -y 220"
endif
ifeq ($(CONFIG_240x320),y)
THEME_SRC=$(BASE_DIR)/data/240x320/activetheme
NXSCREENSIZE="-x 240 -y 320"
endif
ifeq ($(CONFIG_480x640),y)
THEME_SRC=$(BASE_DIR)/data/480x640/activetheme
NXSCREENSIZE="-x 480 -y 640"
endif


include $(BASE_DIR)/Rules.make

SCRIPT_DIR:=$(DEST_DIR)
#DEST_DIR:=$(SCRIPT_DIR)/pda

# Actually get the files that we want 

HTML_FILES =  `find $(BASE_DIR)/data/html -path $(BASE_DIR)/data/html/.svn -prune -path $(BASE_DIR)/data/html/CVS -prune -o -type f -print`
MP3_FILES   = `find $(BASE_DIR)/data/media -name *.mp3`
MPEG_FILES  = `find $(BASE_DIR)/data/media -name *.mpeg -o -name *.mpg`
FONT_FILES  = `find $(BASE_DIR)/data/fonts -name *.ttf`

SCRIPT_TARGET=build-script

# Empty targets

all:
clean:

install: $(INSTALL_DIRS) $(SAMPLE_TARGET) $(SCRIPT_TARGET)

copy-samples: $(HTML_DIR) $(MP3_DIR) $(MPEG_DIR) $(FONT_DIR) $(THEME_DIR) $(ETC_DIR)
#	@ cp $(HTML_FILES) $(HTML_DIR)
#	@ cp $(MP3_FILES) $(MP3_DIR)
#	@ cp $(MPEG_FILES) $(MPEG_DIR)
	@ cp $(FONT_FILES) $(FONT_DIR)
	@ cp $(THEME_SRC)/* $(THEME_DIR)
	@ cp $(BASE_DIR)/etc/* $(ETC_DIR)

build-script: $(RUN_TEMPLATE)
	cat $(RUN_TEMPLATE) | \
	sed -e s%@installdir@%$(DEST_DIR)/%g | \
	sed -e s%@mwdir@%$(MWDIR)%g | \
	sed -e s%@screensize@%$(NXSCREENSIZE)%g | \
	sed -e s%@fltkdir@%$(FLTKDIR)%g > $(SCRIPT_DIR)/run_flphone.sh 

	@ chmod u+x $(SCRIPT_DIR)/run_flphone.sh

$(HTML_DIR) $(MP3_DIR) $(MPEG_DIR) $(FONT_DIR) $(INSTALL_DIRS) $(THEME_DIR) $(ETC_DIR):
	@ mkdir -p $@	

.PHONY: samples
