bin_PROGRAMS = ario

ario_SOURCES = \
	main.c\
	shell.c\
	shell.h\
	browser.c\
	browser.h\
	mpd.c\
	mpd.h\
	libmpdclient.c\
	libmpdclient.h\
	header.c\
	header.h\
	eggtrayicon.c\
	eggtrayicon.h\
	tray-icon.c\
	tray-icon.h\
	playlist.c\
	playlist.h\
	rb-glade-helpers.c\
	rb-glade-helpers.h\
	util.c\
	util.h\
	preferences.c\
	preferences.h\
	cover.c\
	cover.h\
	volume.c\
	volume.h\
	shell-coverdownloader.c\
	shell-coverdownloader.h\
	shell-coverselect.c\
	shell-coverselect.h\
	status-bar.c\
	status-bar.h

ario_LDADD = $(DEPS_LIBS)

AM_CPPFLAGS = $(DEPS_CFLAGS) \
              -DLOCALE_DIR=\""$(datadir)/locale"\" -DPIXMAP_PATH=\""$(pkgdatadir)/art/"\" -DUI_PATH=\""$(pkgdatadir)/ui/"\" -DGLADE_PATH=\""$(pkgdatadir)/glade/"\"

