# Generated automatically from Makefile.in by configure.
# Makefile for check subdirectory in GNU gettext package.
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

subdir = checks


TESTSRCS = testdata
DISTFILES = ChangeLog Makefile.in $(TESTSRCS)

all:

check:
	rm -f test.bin test.tmp
	../src/uudecode $(srcdir)/testdata
	../src/uuencode test.bin test.bin > test.tmp
	cmp $(srcdir)/testdata test.tmp || exit 1
	rm -f test.bin test.tmp
	@echo ==========================
	@echo All checks are successful.
	@echo ==========================

install uninstall tags TAGS id ID:

mostlyclean:
	rm -f core core.* *.tmp test.bin

clean: mostlyclean

distclean: clean
	rm -f Makefile

maintainer-clean: distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

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
