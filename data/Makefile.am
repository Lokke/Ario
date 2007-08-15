schemadir = @GCONF_SCHEMA_FILE_DIR@
schema_DATA = ario.schemas

artdir = $(pkgdatadir)/art
art_DATA = icon.png		\
	   play.png		\
	   shuffle.png		\
	   volume-medium.png    \
	   volume-zero.png	\
	   repeat.png		\
	   volume-max.png	\
	   volume-min.png

gladedir = $(pkgdatadir)/glade
glade_DATA = connection-prefs.glade	\
	     cover-prefs.glade		\
	     server-prefs.glade		\
	     cover-progress.glade	\
	     cover-select.glade

uidir = $(pkgdatadir)/ui
ui_DATA = ario-ui.xml

install-data-local: $(schema_DATA)
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(schema_DATA);

EXTRA_DIST = $(art_DATA)	\
	     $(glade_DATA)	\
	     $(schema_DATA)	\
	     $(ui_DATA)
