uidir = $(datadir)/glista

ui_DATA = glista.ui \
          glista-icon.png

EXTRA_DIST = $(ui_DATA)

%.ui: %.glade 
	$(GTKBLDRCONV) $< $@

