uidir = $(datadir)/ui

ui_DATA = glista.ui \
          glista-icon.svg

EXTRA_DIST = $(ui_DATA)

%.ui: %.glade 
	$(GTKBLDRCONV) $< $@

