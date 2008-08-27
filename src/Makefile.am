bin_PROGRAMS = glista

glista_SOURCES = main.c
glista_LDADD = $(DEPS_LIBS)

AM_CFLAGS = $(DEPOS_CFLAGS) \
            -export-dynamic

AM_CPPFLAGS = $(DEPS_CFLAGS) \
              -DGLISTA_UI_DIR=\""$(datadir)/glista"\"

