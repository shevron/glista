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

dnl gtk related dependencies
PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.12 glib-2.0 >= 2.6 gthread-2.0)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

dnl libxml 
PKG_CHECK_MODULES(LIBXML, libxml-2.0 >= 2.6)
AC_SUBST(LIBXML_CFLAGS)
AC_SUBST(LIBXML_LIBS)

dnl Check for gtk-builder-convert
AC_CHECK_PROGS([GTKBLDRCONV], [gtk-builder-convert], [:])
if test "$GTKBLDRCONV" = :; then
  AC_MSG_ERROR([gtk-builder-convert is required by this package])
fi

dnl Add option to modify the configuration directory. 
dnl Default is defined in glista.h
AC_MSG_CHECKING([for user configuration directory])
AC_ARG_WITH([user-confdir], 
  [AS_HELP_STRING([--with-user-confdir=DIR], 
    [custom user configuration directory @<:@default=glista@:>@])],
  [
    if test "$withval" != "no"; then 
      if test "$withval" = "yes"; then
        AC_MSG_RESULT([default])
      else 
        AC_DEFINE_UNQUOTED(GLISTA_CONFIG_DIR, "$withval", 
          [User configuration directory])
        AC_MSG_RESULT($withval)
      fi
    else 
      AC_MSG_RESULT([default])
    fi
  ],[
    AC_MSG_RESULT([default])
  ])

AC_CONFIG_FILES([Makefile src/Makefile ui/Makefile])
AC_CONFIG_COMMANDS([default],[[ echo timestamp > stamp-h ]],[[]])
AC_OUTPUT
