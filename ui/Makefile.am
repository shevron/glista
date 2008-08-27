uidir = $(datadir)/glista

ui_DATA = glista.ui \
          glista-icon.png

EXTRA_DIST = $(ui_DATA)

desktopdir = $(datadir)/applications
desktop_DATA = glista.desktop

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = glista-icon.png

%.ui: %.glade 
	$(GTKBLDRCONV) $< $@

