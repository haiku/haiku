# Derived by hand from the generated readline-src/doc/Makefile
# This makefile for Readline library documentation is in -*- text -*- mode.
# Emacs likes it that way.

# Copyright (C) 1996-2002 Free Software Foundation, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA.

topdir = .
srcdir = .
VPATH = .

prefix = /usr/local
infodir = ${prefix}/info

mandir = ${prefix}/man
manpfx = man

man1ext = 1
man1dir = $(mandir)/$(manpfx)$(man1ext)
man3ext = 3
man3dir = $(mandir)/$(manpfx)$(man3ext)

SHELL = /bin/sh
RM = rm -f

INSTALL = /usr/bin/install -c
INSTALL_DATA = ${INSTALL} -m 644

BUILD_DIR = .
TEXINPUTDIR = $(srcdir)

MAKEINFO    = LANGUAGE= makeinfo
TEXI2DVI    = $(srcdir)/texi2dvi
TEXI2HTML   = $(srcdir)/texi2html
QUIETPS     = #set this to -q to shut up dvips
PSDPI       = 300	# I don't have any 600-dpi printers
DVIPS       = dvips -D ${PSDPI} $(QUIETPS) -o $@     # tricky

RLSRC = $(srcdir)/rlman.texinfo $(srcdir)/rluser.texinfo \
	$(srcdir)/rltech.texinfo $(srcdir)/manvers.texinfo \
	$(srcdir)/rluserman.texinfo
HISTSRC = $(srcdir)/hist.texinfo $(srcdir)/hsuser.texinfo \
	  $(srcdir)/hstech.texinfo $(srcdir)/manvers.texinfo

# This should be a program that converts troff to an ascii-readable format
NROFF       = groff -Tascii

# This should be a program that converts troff to postscript
GROFF       = groff

DVIOBJ = readline.dvi history.dvi rluserman.dvi
INFOOBJ = readline.info history.info rluserman.info
PSOBJ = readline.ps history.ps rluserman.ps
HTMLOBJ = readline.html history.html rluserman.html

INTERMEDIATE_OBJ = rlman.dvi hist.dvi rluserman.dvi

CREATED_DOCS = $(DVIOBJ) $(INFOOBJ) $(PSOBJ) $(HTMLOBJ)

.SUFFIXES:      .ps .txt .dvi

all: info dvi html ps 
nodvi: info html

readline.dvi: $(RLSRC)
	TEXINPUTS=.:$(TEXINPUTDIR):$$TEXINPUTS $(TEXI2DVI) $(srcdir)/rlman.texinfo
	mv rlman.dvi readline.dvi

readline.info: $(RLSRC)
	$(MAKEINFO) --no-split -I $(TEXINPUTDIR) -o $@ $(srcdir)/rlman.texinfo

rluserman.dvi: $(RLSRC)
	TEXINPUTS=.:$(TEXINPUTDIR):$$TEXINPUTS $(TEXI2DVI) $(srcdir)/rluserman.texinfo

rluserman.info: $(RLSRC)
	$(MAKEINFO) --no-split -I $(TEXINPUTDIR) -o $@ $(srcdir)/rluserman.texinfo

history.dvi: ${HISTSRC}
	TEXINPUTS=.:$(TEXINPUTDIR):$$TEXINPUTS $(TEXI2DVI) $(srcdir)/hist.texinfo
	mv hist.dvi history.dvi

history.info: ${HISTSRC}
	$(MAKEINFO) --no-split -I $(TEXINPUTDIR) -o $@ $(srcdir)/hist.texinfo

readline.ps:	readline.dvi
	$(RM) $@
	$(DVIPS) readline.dvi

rluserman.ps:	rluserman.dvi
	$(RM) $@
	$(DVIPS) rluserman.dvi

history.ps:	history.dvi
	$(RM) $@
	$(DVIPS) history.dvi

readline.html:	${RLSRC}
	$(TEXI2HTML) -menu -monolithic -I $(TEXINPUTDIR) $(srcdir)/rlman.texinfo
	sed -e 's:rlman.html:readline.html:' rlman.html > readline.html
	$(RM) rlman.html

rluserman.html:	${RLSRC}
	$(TEXI2HTML) -menu -monolithic -I $(TEXINPUTDIR) $(srcdir)/rluserman.texinfo

history.html:	${HISTSRC}
	$(TEXI2HTML) -menu -monolithic -I $(TEXINPUTDIR) $(srcdir)/hist.texinfo
	sed -e 's:hist.html:history.html:' hist.html > history.html
	$(RM) hist.html

info:	$(INFOOBJ)
dvi:	$(DVIOBJ)
ps:	$(PSOBJ)
html:	$(HTMLOBJ)

clean:
	$(RM) *.aux *.cp *.fn *.ky *.log *.pg *.toc *.tp *.vr *.cps *.pgs \
	      *.fns *.kys *.tps *.vrs *.o core

distclean: clean
	$(RM) $(CREATED_DOCS)
	$(RM) $(INTERMEDIATE_OBJ)
	$(RM) Makefile

mostlyclean: clean

maintainer-clean: clean
	$(RM) $(CREATED_DOCS)
	$(RM) $(INTERMEDIATE_OBJ)
	$(RM) Makefile

install:
	@echo "This documentation should not be installed."

uninstall:
