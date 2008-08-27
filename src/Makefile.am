bin_PROGRAMS = glista

glista_SOURCES = main.c
glista_LDADD = $(GTK_LIBS) \
               $(LIBXML_LIBS)

AM_CFLAGS = $(DEPOS_CFLAGS) \
            -export-dynamic

AM_CPPFLAGS = $(GTK_CFLAGS) \
              $(LIBXML_CFLAGS) \
              -DGLISTA_UI_DIR=\""$(datadir)/glista"\"
