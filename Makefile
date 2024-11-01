PREFIX?=/usr/local

BINDIR?=$(PREFIX)/bin

INSTALL?=install
INSTALL_PROGRAM?=$(INSTALL)
INSTALL_DATA?=$(INSTALL) -m 644

.PHONY: clean

aseqwrite: aseqwrite.c
	gcc -O3 -o $@ $< -lasound

install: aseqwrite
	mkdir -p $(DESTDIR)$(BINDIR)
	$(INSTALL_PROGRAM) aseqwrite $(DESTDIR)$(BINDIR)/

clean:
	rm -f aseqwrite
