## $Id: Makefile.am,v 1.10 2010/08/11 00:26:09 rlk Exp $
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

## Variables

pkgxmldatadir = $(pkgdatadir)/@GUTENPRINT_MAJOR_VERSION@.@GUTENPRINT_MINOR_VERSION@/xml/escp2/inputslots

pkgxmldata_DATA =				\
	artisan.xml				\
	b500.xml				\
	cd.xml					\
	cd_cutter_roll_feed.xml			\
	cd_roll_feed.xml			\
	cutter_roll_feed.xml			\
	default.xml				\
	pro3880.xml				\
	pro_roll_feed.xml			\
	pro_cutter_roll_feed.xml		\
	r1800.xml				\
	r2400.xml				\
	r2880.xml				\
	rx680.xml				\
	rx700.xml				\
	spro5000.xml				\
	standard_roll_feed.xml

xml-stamp: $(pkgxmldata_DATA) Makefile.am
	-rm -f $@ $@.tmp
	touch $@.tmp
	for f in $(pkgxmldata_DATA) ; do echo $$f >> $@.tmp; done
	mv $@.tmp $@

all-local: xml-stamp

dist-hook: xml-stamp
CLEANFILES = xmli18n-tmp.h xml-stamp xml-stamp.tmp

EXTRA_DIST = $(pkgxmldata_DATA)
