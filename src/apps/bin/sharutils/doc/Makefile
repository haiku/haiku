# Generated automatically from Makefile.in by configure.
# Makefile for GNU Shar utility package documentation.
# Copyright (C) 1995 Free Software Foundation, Inc.
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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

PACKAGE = sharutils
VERSION = 4.2.1

SHELL = /bin/sh
srcdir = .
top_srcdir = ..

transform = s,x,x,
INSTALL = /bin/install -c
INSTALL_DATA = ${INSTALL} -m 644
MAKEINFO = makeinfo
TEXI2DVI = texi2dvi
TOUCH = echo timestamp >

DISTFILES = ChangeLog Makefile.in mdate-sh sharutils.texi remsync.texi \
shar.1 unshar.1 uuencode.1 uuencode.5 \
texinfo.tex version.texi sharutils.info remsync.info stamp-vti
MAN1PAGES = shar unshar uuencode uudecode
MAN5PAGES =  uuencode

prefix = /usr/local
infodir = $(prefix)/info
mandir = $(prefix)/man
man1dir = $(mandir)/man1
man1ext = .1
man5dir = $(mandir)/man5
man5ext = .5
subdir = doc

.SUFFIXES:

default: all

all info check: sharutils.info remsync.info

sharutils.info: sharutils.texi version.texi
	cd $(srcdir) && $(MAKEINFO) sharutils.texi

remsync.info: remsync.texi version.texi
	cd $(srcdir) && $(MAKEINFO) remsync.texi

dvi: sharutils.dvi remsync.dvi

sharutils.dvi: sharutils.texi texinfo.tex version.texi
	$(TEXI2DVI) $(srcdir)/sharutils.texi

remsync.dvi: remsync.texi texinfo.tex
	$(TEXI2DVI) $(srcdir)/remsync.texi

version.texi: stamp-vti
stamp-vti: sharutils.texi $(top_srcdir)/configure.in
	echo "@set EDITION $(VERSION)" > version.tmp
	echo "@set UPDATED `cd $(srcdir) \
	  && $(SHELL) mdate-sh sharutils.texi`" >> version.tmp
	echo "@set VERSION $(VERSION)" >> version.tmp
	if cmp -s version.tmp version.texi; then \
	  rm -f version.tmp; \
	else \
	  mv -f version.tmp version.texi; \
	fi
	$(TOUCH) $(srcdir)/stamp-vti

install install-exec install-data: all
install: install-exec install-data
install-exec:
install-data:
	$(top_srcdir)/mkinstalldirs $(infodir)
	cd $(srcdir) && \
	  for file in sharutils.info* remsync.info*; do \
	    $(INSTALL_DATA) $$file $(infodir)/$$file; \
	  done

uninstall:
	rm -f $(infodir)/sharutils.info* $(infodir)/remsync.info*


# man page installation is kept separate from install.  Better think
# GNU and switch to Info.  The following is for irreductible fellows.

install-man:
	$(top_srcdir)/mkinstalldirs $(man1dir)
	test -z "$(MAN5PAGES)" || $(top_srcdir)/mkinstalldirs $(man5dir)
	for name in $(MAN1PAGES); do \
	  if test $$name = uudecode; then \
	    rm -f $(man1dir)/`echo uudecode | sed '$(transform)'`$(man1ext); \
	    man1base=`echo $(man1dir) | sed 's@.*/\(.*\)@&1@'`; \
	    echo ".so $$man1base/uuencode$(man1ext)" \
	      > $(man1dir)/`echo uudecode | sed '$(transform)'`$(man1ext); \
	  else \
	    $(INSTALL_DATA) $(srcdir)/$$name.1 \
		$(man1dir)/`echo $$name | sed '$(transform)'`$(man1ext); \
	  fi; \
	done
	test -z "$(MAN5PAGES)" || \
	  for name in $(MAN5PAGES); do \
	    $(INSTALL_DATA) $(srcdir)/$$name.5 \
		$(man5dir)/`echo $$name | sed '$(transform)'`$(man5ext); \
	  done

uninstall-man:
	for name in $(MAN1PAGES); do \
	  rm -f $(man1dir)/`echo $$name | sed '$(transform)'`$(man1ext); \
	done
	test -z "$(MAN5PAGES)" || \
	  for name in $(MAN5PAGES); do \
	    rm -f $(man5dir)/`echo $$name | sed '$(transform)'`$(man5ext); \
	  done


TAGS ID:

mostlyclean:
	rm -f *.aux *.cp *.cps *.dvi *.fn *.fns *.ky *.log *.pg *.toc *.tp *.vr
	rm -f *.tmp

clean: mostlyclean

distclean: clean
	rm -f Makefile

maintainer-clean: distclean
	@echo "This command is intended only for maintainers to use;"
	@echo "rebuilding the deleted files may require special tools."
	rm -f stamp-vti version.texi sharutils.info* remsync.info*

distdir = ../$(PACKAGE)-$(VERSION)/$(subdir)
dist: Makefile $(DISTFILES)
	for file in $(DISTFILES); do \
	  ln $(srcdir)/$$file $(distdir) 2> /dev/null \
	    || cp -p $(srcdir)/$$file $(distdir); \
	done

Makefile: Makefile.in ../config.status
	cd .. && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= ./config.status

# Tell versions [3.59,3.63) of GNU make not to export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
