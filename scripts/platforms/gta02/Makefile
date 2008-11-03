# Makefile to set up the GTA02 platform

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

include $(BASE_DIR)/Rules.make

SCRIPT_DIR:=$(DEST_DIR)
DEST_DIR:=$(SCRIPT_DIR)-sx1

# Actually get the files that we want 

HTML_FILES =  `find $(BASE_DIR)/data/html -path $(BASE_DIR)/data/html/.svn -prune -path $(BASE_DIR)/data/html/CVS -prune -o -type f -print`
MP3_FILES   = `find $(BASE_DIR)/data/media -name *.mp3`
MPEG_FILES  = `find $(BASE_DIR)/data/media -name *.mpeg -o -name *.mpg`
FONT_FILES  = `find $(BASE_DIR)/data/fonts -name *.ttf`

# Empty targets

all:
clean:

install: $(INSTALL_DIRS) $(SAMPLE_TARGET)
	@ cp run_flphone.sh $(INSTALL_DIR)
	@ chmod uga+x $(INSTALL_DIR)/run_flphone.sh

copy-samples: $(HTML_DIR) $(MP3_DIR) $(MPEG_DIR) $(FONT_DIR) $(THEME_DIR) $(ETC_DIR)
#	@ cp $(HTML_FILES) $(HTML_DIR)
#	@ cp $(MP3_FILES) $(MP3_DIR)
#	@ cp $(MPEG_FILES) $(MPEG_DIR)
	@ cp $(FONT_FILES) $(FONT_DIR)
	@ cp $(BASE_DIR)/data/480x640/activetheme/* $(THEME_DIR)
	@ cp $(BASE_DIR)/etc/* $(ETC_DIR)

$(HTML_DIR) $(MP3_DIR) $(MPEG_DIR) $(FONT_DIR) $(INSTALL_DIRS) $(THEME_DIR) $(ETC_DIR):
	@ mkdir -p $@

.PHONY: samples
