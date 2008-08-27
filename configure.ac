dnl Process this file with autoconf to produce a configure script.

AC_INIT([glista], [0.3-dev], [shahar@prematureoptimization.org])
AC_PREREQ(2.57)
AC_CONFIG_SRCDIR(src/main.c)
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_INSTALL

dnl required dependencies
PKG_CHECK_MODULES(DEPS, gtk+-2.0 >= 2.12 glib-2.0 >= 2.6 libxml-2.0 >= 2.6)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

dnl Check for gtk-builder-convert
AC_CHECK_PROGS([GTKBLDRCONV], [gtk-builder-convert], [:])
if test "$GTKBLDRCONV" = :; then
  AC_MSG_ERROR([gtk-builder-convert is required by this package])
fi

AC_CHECK_LIB(gthread-2.0, g_thread_init)

AC_CONFIG_FILES([Makefile src/Makefile ui/Makefile])
AC_CONFIG_COMMANDS([default],[[ echo timestamp > stamp-h ]],[[]])
AC_OUTPUT

