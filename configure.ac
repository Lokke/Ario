AC_PREREQ(2.59)

m4_define(ario_version, 0.1)

PACKAGE=Ario
VERSION=0.1

AC_INIT(ario, ario_version)
AC_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE($PACKAGE,$VERSION)

AC_PROG_CC
AC_PROG_INSTALL
AC_STDC_HEADERS

AC_PROG_INTLTOOL([0.35.0])

PKG_CHECK_MODULES(DEPS, [gnome-vfs-2.0 gtk+-2.0 >= 2.6 glib-2.0 >= 2.4 gobject-2.0 >= 2.4 libglade-2.0 eel-2.0 libcurl])

AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

dnl i18n
GETTEXT_PACKAGE=Ario
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package])
AM_GLIB_GNU_GETTEXT


dnl Gconf
AC_PATH_PROG(GCONFTOOL, gconftool-2)
AM_GCONF_SOURCE_2

AC_OUTPUT([
Makefile
src/Makefile
data/Makefile
po/Makefile.in
data/ario.desktop.in
])

