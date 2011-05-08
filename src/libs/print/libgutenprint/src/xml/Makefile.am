## $Id: Makefile.am,v 1.6 2011/03/29 22:58:12 rlk Exp $
## Copyright (C) 2008 Robert Krawitz
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

## Process this file with automake to produce Makefile.in.

@SET_MAKE@

include $(top_srcdir)/scripts/global.mk

SUBDIRS = escp2

## Variables

pkgxmldatadir = $(pkgdatadir)/@GUTENPRINT_MAJOR_VERSION@.@GUTENPRINT_MINOR_VERSION@/xml

pkgxmldata_DATA =				\
	dither-matrix-1x1.xml			\
	dither-matrix-2x1.xml			\
	dither-matrix-4x1.xml			\
	papers.xml				\
	printers.xml

## Rules

noinst_PROGRAMS = extract-strings

extract_strings_SOURCES = extract-strings.c
extract_strings_LDADD = $(GUTENPRINT_LIBS)

xml-stamp: $(pkgxmldata_DATA) escp2/xml-stamp Makefile.am
	-rm -f $@ $@.tmp
	touch $@.tmp
	sed 's;^;escp2/;' escp2/xml-stamp >> $@.tmp
	for f in $(pkgxmldata_DATA) ; do echo $$f >> $@.tmp; done
	mv $@.tmp $@

all-local: xmli18n-tmp.h xml-stamp


xmli18n-tmp.h: xml-stamp extract-strings
	-rm -f $@ $@.tmp
	./extract-strings `cat xml-stamp | sed -e 's;^;$(srcdir)/;'` > $@.tmp
	mv $@.tmp $@


dist-hook: xmli18n-tmp.h xml-stamp
# xmli18n-tmp.h is needed by po/POTFILES.in at dist time

## Clean

CLEANFILES = xmli18n-tmp.h xmli18n-tmp.h.tmp xml-stamp xml-stamp.tmp

EXTRA_DIST = $(pkgxmldata_DATA)

MAINTAINERCLEANFILES = Makefile.in
