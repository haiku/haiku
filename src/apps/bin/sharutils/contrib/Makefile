# Generated automatically from Makefile.in by configure.
# Makefile for GNU shar utilities, contribution directory.
# Copyright (C) 1995 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

PACKAGE = sharutils
VERSION = 4.2.1

SHELL = /bin/sh
srcdir = .

subdir = contrib

.SUFFIXES:

DISTFILES = Makefile.in shar.sh shar2.sh bas-README uudecode.bas \
pas-README pas-R.Marks pas-diffs uudecode.pas uuencode.pas \
uudecode.pl uuencode.pl

all:

check:

install:

uninstall:

mostlyclean:

clean: mostlyclean

distclean: clean
	rm -f Makefile

maintainer-clean: distclean
	@echo "This command is intended only for maintainers to use;"
	@echo "rebuilding the deleted files may require special tools."

distdir = ../$(PACKAGE)-$(VERSION)/$(subdir)
dist: $(DISTFILES)
	@for file in $(DISTFILES); do \
	  ln $(srcdir)/$$file $(distdir) 2> /dev/null \
	    || cp -p $(srcdir)/$$file $(distdir); \
	done

Makefile: ../config.status Makefile.in
	cd .. && CONFIG_FILES=contrib/$@ CONFIG_HEADERS= ./config.status

# Tell versions [3.59,3.63) of GNU make not to export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
