bin_PROGRAMS = glista

glista_SOURCES = main.c glista-storage.c

glista_LDADD = $(GTK_LIBS) \
               $(LIBXML_LIBS) \
               $(UNIQUE_LIBS) \
               $(GTKSPELL_LIBS)

AM_CFLAGS = $(DEPOS_CFLAGS) \
            -export-dynamic

AM_CPPFLAGS = $(GTK_CFLAGS) \
              $(LIBXML_CFLAGS) \
              $(UNIQUE_CFLAGS) \
              $(GTKSPELL_CFLAGS) \
              -DGLISTA_UI_DIR=\""$(datadir)/glista"\"
