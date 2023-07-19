PREFIX?=/usr/local

BINDIR?=$(PREFIX)/bin

INSTALL?=install
INSTALL_PROGRAM?=$(INSTALL)
INSTALL_DATA?=$(INSTALL) -m 644

aseqsend: aseqsend.c
	gcc -o $@ $< -lasound

install: aseqsend
	mkdir -p $(DESTDIR)$(BINDIR)
	$(INSTALL_PROGRAM) aseqsend $(DESTDIR)$(BINDIR)/
