bin_PROGRAMS = glista

EXTRA_DIST = glista-ui-callbacks.c \
             glista-unique.c

glista_SOURCES = main.c \
                 glista.h \
                 glista-storage.c \
                 glista-storage.h

glista_LDADD = $(GTK_LIBS) \
               $(LIBXML_LIBS) \
               $(UNIQUE_LIBS) \
               $(GTKSPELL_LIBS)

AM_CFLAGS = $(DEPOS_CFLAGS) \
            -export-dynamic

AM_CPPFLAGS = $(GTK_CFLAGS) \
              $(LIBXML_CFLAGS) \
              $(UNIQUE_CFLAGS) \
              $(GTKSPELL_CFLAGS)
