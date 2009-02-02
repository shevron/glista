dnl Process this file with autoconf to produce a configure script.

# Initialize autotools
AC_PREREQ(2.61)
AC_INIT([glista], [0.4], [shahar@prematureoptimization.org])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_INTLTOOL([0.23])

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

dnl check for gettext
ALL_LINGUAS="he sv"
AM_GLIB_GNU_GETTEXT
GETTEXT_PACKAGE=$PACKAGE
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["${GETTEXT_PACKAGE}"], [gettext domain])

# --- libunique ---
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
        export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:${withval}/lib/pkgconfig"
      fi
    fi
  ],[
    check_unique="yes"
  ])

if test -n "$check_unique"; then
  PKG_CHECK_MODULES([UNIQUE], unique-1.0 >= 1.0, [
    AC_DEFINE_UNQUOTED(HAVE_UNIQUE, [1], [Whether Unique is available])
  ], [
    AC_MSG_WARN([Single instance detection will be disabled])
  ])
else
  AC_MSG_CHECKING([for UNIQUE])
  AC_MSG_RESULT([disabled])
fi

AC_SUBST(UNIQUE_CFLAGS)
AC_SUBST(UNIQUE_LIBS)

# --- gtkspell ---
dnl Check for GtkSpell
AC_ARG_WITH([gtkspell],
  [AS_HELP_STRING([--with-gtkspell=DIR],
    [enable spell checking support @<:@default=yes@:>@])],
  [
    if test "x$withval" = "xno"; then
      check_gtkspell=""
    else
      check_gtkspell=$withval
      if test "x$withval" != "xyes"; then
        export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:${withval}/lib/pkgconfig"
      fi
    fi
  ],[
    check_gtkspell="yes"
  ])

if test -n "$check_gtkspell"; then
  PKG_CHECK_MODULES([GTKSPELL], gtkspell-2.0, [
    AC_DEFINE_UNQUOTED(HAVE_GTKSPELL, [1], [Whether GtkSpell is available])
  ], [
    AC_MSG_WARN([Spell checking will be disabled])
  ])
else
  AC_MSG_CHECKING([for GTKSPELL])
  AC_MSG_RESULT([disabled])
fi

AC_SUBST(GTKSPELL_CFLAGS)
AC_SUBST(GTKSPELL_LIBS)

# --- libnotify ---
dnl see if we have libnotify available
AC_ARG_WITH([libnotify],
  [AS_HELP_STRING([--with-libnotify=DIR],
    [enable libnotify support @<:@default=yes@:>@])],
  [
    if test "x$withval" = "xno"; then
      check_libnotify=""
    else
      check_libnotify=$withval
      if test "x$withval" != "xyes"; then
        export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:${withval}/lib/pkgconfig"
      fi
    fi
  ],[
    check_libnotify="yes"
  ])

if test -n "$check_libnotify"; then
  PKG_CHECK_MODULES([LIBNOTIFY], libnotify >= 0.4, [
    AC_DEFINE_UNQUOTED(HAVE_LIBNOTIFY, [1], [Whether libnotify is available])
    enable_libnotify="yes"
  ], [
    enable_libnotify="no"
  ])
else
  AC_MSG_CHECKING([for LIBNOTIFY])
  AC_MSG_RESULT([disabled])
  enable_libnotify="no"
fi

AC_SUBST(LIBNOTIFY_CFLAGS)
AC_SUBST(LIBNOTIFY_LIBS)
AM_CONDITIONAL(ENABLE_LIBNOTIFY, test "x$enable_libnotify" = "xyes")  


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
  
dnl allow disabling URL linkfication in text view
enable_linkify="yes"
AC_ARG_ENABLE([linkify-urls], 
  [AS_HELP_STRING([--enable-linkify-urls], 
    [Make URLs in notes clickable  @<:@default=yes@:>@])],
  [
    if test "x$enableval" = "xno"; then
      enable_linkify="no"
    fi
  ])
AM_CONDITIONAL(ENABLE_LINKIFY, test "x$enable_linkify" = "xyes")  
if test "x$enable_linkify" = "xyes"; then
  AC_DEFINE_UNQUOTED(ENABLE_LINKIFY, [1], [Whether to linkify URLs in notes])
fi


# Set some paths
AC_DEFINE_DIR(GLISTA_DATA_DIR, [{datadir}/glista], 
    [Glista system-wide shared data files])
AC_DEFINE_DIR(GLISTA_LIB_DIR, [{libdir}/glista], 
    [Glista dynamically-loadedd libraries and plug-ins])

AC_CONFIG_FILES([Makefile 
                 src/Makefile
                 src/modules/Makefile
                 ui/Makefile
                 po/Makefile.in])

AC_CONFIG_COMMANDS([default],[[ echo timestamp > stamp-h ]],[[]])

AC_OUTPUT
