dnl Process this file with autoconf to produce a configure script.

# Initialize autotools
AC_PREREQ(2.61)
AC_INIT([glista], [0.3.1-dev], [shahar@prematureoptimization.org])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

dnl make sure we have gtk-builder-convert
AC_CHECK_PROGS([GTKBLDRCONV], [gtk-builder-convert], [:])
if test "$GTKBLDRCONV" = :; then
  AC_MSG_ERROR([gtk-builder-convert is required by this package])
fi

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for library functions.
AC_CHECK_FUNCS([strerror])

# Check for libraries

dnl check for gtk & related libraries
PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.12 glib-2.0 >= 2.6 gthread-2.0)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

dnl check for libxml2
PKG_CHECK_MODULES(LIBXML, libxml-2.0 >= 2.6)
AC_SUBST(LIBXML_CFLAGS)
AC_SUBST(LIBXML_LIBS)

dnl see if we have Unique available
AC_ARG_WITH([unique],
  [AS_HELP_STRING([--with-unique=DIR],
    [enable Unique support @<:@default=yes@:>@])],
  [
    if test "x$withval" = "xno"; then
      check_unique=""
    else
      check_unique=$withval
      if test "x$withval" != "xyes"; then
        export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:${withval}/pkgconfig"
      fi
    fi
  ],[
    check_unique="yes"
  ])

if test -n "$check_unique"; then
  PKG_CHECK_MODULES([Unique], unique-1.0 >= 1.0, [
    UNIQUE_CFLAGS=$Unique_CFLAGS
    UNIQUE_LIBS=$Unique_LIBS
    AC_DEFINE_UNQUOTED(HAVE_UNIQUE, [1], [Whether Unique is available])
  ], [
    AC_MSG_WARN([Single instance detection will be disabled])
  ])
else
  AC_MSG_CHECKING([for Unique])
  AC_MSG_RESULT([no])
  AC_MSG_WARN([Single instance detection will be disabled])
fi

AC_SUBST(UNIQUE_CFLAGS)
AC_SUBST(UNIQUE_LIBS)

dnl add option to modify the configuration directory. 
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


AC_CONFIG_FILES([Makefile 
                 src/Makefile 
                 ui/Makefile])

AC_CONFIG_COMMANDS([default],[[ echo timestamp > stamp-h ]],[[]])

AC_OUTPUT
