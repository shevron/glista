bin_PROGRAMS = glista

glista_SOURCES = main.c \
                 glista.h \
                 glista-ui.c \
                 glista-ui.h \
                 glista-storage.c \
                 glista-storage.h \
                 glista-unique.c \
                 glista-unique.h

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
