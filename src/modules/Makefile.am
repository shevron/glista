# Makefile.am for glista modules

GLISTA_MOD_LDFLAGS = -module -avoid-version $(GTK_LIBS)

# dummy remind handler module - always built (until we remove it)
pkglib_LTLIBRARIES = glistareminder-dummy.la \
                     glistareminder-default.la

# libnotify module - built unless the user removes it or libnotify is missing
if ENABLE_LIBNOTIFY
pkglib_LTLIBRARIES += glistareminder-libnotify.la
endif 

glistareminder_dummy_la_SOURCES = glista-reminder-dummy.c
glistareminder_dummy_la_LDFLAGS = $(GLISTA_MOD_LDFLAGS)

glistareminder_default_la_SOURCES = glista-reminder-default.c
glistareminder_default_la_LDFLAGS = $(GLISTA_MOD_LDFLAGS)

glistareminder_libnotify_la_SOURCES = glista-reminder-libnotify.c
glistareminder_libnotify_la_LDFLAGS = $(GLISTA_MOD_LDFLAGS) \
                                      $(LIBNOTIFY_LIBS)

AM_CFLAGS = $(DEPOS_CFLAGS)

AM_CPPFLAGS = $(GTK_CFLAGS) \
              $(LIBNOTIFY_CFLAGS) \
              -DLOCALE_DIR=\""$(datadir)/locale"\"

# This is here to remove everything but the .so files we need, but
# I'm not sure this will not fuck things up... (?)
install-exec-hook:
	rm -f $(DESTDIR)$(pkglibdir)/*.la
	rm -f $(DESTDIR)$(pkglibdir)/*.a

