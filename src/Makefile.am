SUBDIRS = modules

bin_PROGRAMS = glista

if ENABLE_LINKIFY
OPTIONAL_GLISTA = glista-textview-linkify.c \
                  glista-textview-linkify.h
else 
OPTIONAL_GLISTA = 
endif

glista_SOURCES = main.c \
                 glista.h \
                 glista-reminder.c \
                 glista-reminder.h \
                 glista-storage.c \
                 glista-storage.h \
                 glista-ui.c \
                 glista-ui.h \
                 glista-unique.c \
                 glista-unique.h \
				 glista-plugin.c \
				 glista-plugin.h \
                 $(OPTIONAL_GLISTA)

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
              -DLOCALE_DIR=\""$(datadir)/locale"\"
