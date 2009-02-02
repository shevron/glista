uidir = $(datadir)/glista
ui_DATA = glista.ui \
          glista-icons.xml \
          glista*.png

EXTRA_DIST = \
	$(ui_DATA) \
	glista.desktop.in

desktopdir = $(datadir)/applications
desktop_DATA = glista.desktop

@INTLTOOL_DESKTOP_RULE@
	echo "Icon=$(uidir)/glista48.png" >> $@
	
%.ui: %.glade 
	$(GTKBLDRCONV) $< $@

CLEANFILES = \
	glista.desktop
