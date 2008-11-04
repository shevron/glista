uidir = $(datadir)/glista
ui_DATA = glista.ui glista-icon.png

EXTRA_DIST = \
	$(ui_DATA) \
	glista.desktop.in

desktopdir = $(datadir)/applications
desktop_DATA = glista.desktop

glista.desktop:
	cat $@.in > $@
	echo "Icon=$(uidir)/glista-icon.png" >> $@
	
%.ui: %.glade 
	$(GTKBLDRCONV) $< $@

CLEANFILES = \
	glista.desktop
	
