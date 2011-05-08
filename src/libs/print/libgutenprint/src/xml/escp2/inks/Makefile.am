## $Id: Makefile.am,v 1.13 2011/03/05 18:49:13 rlk Exp $
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

pkgxmldatadir = $(pkgdatadir)/@GUTENPRINT_MAJOR_VERSION@.@GUTENPRINT_MINOR_VERSION@/xml/escp2/inks

pkgxmldata_DATA =				\
	artisan.xml				\
	b500.xml				\
	c120.xml				\
	c64.xml					\
	c80.xml					\
	c82.xml					\
	claria.xml				\
	cmy.xml					\
	cmykrb.xml				\
	cmykro.xml				\
	cx3650.xml				\
	defaultblack.xml			\
	f360.xml				\
	f360_photo.xml				\
	f360_photo7_japan.xml			\
	f360_ultrachrome.xml			\
	f360_ultrachrome_k3.xml			\
	f360_ultrachrome_k3v.xml		\
	f360_ultrachrome_k3v_2.xml		\
	nx100.xml				\
	photo_gen0.xml				\
	photo_gen1.xml				\
	photo_gen2.xml				\
	photo_gen3.xml				\
	photo_gen3_4.xml			\
	photo_gen4.xml				\
	photo_pigment.xml			\
	picturemate_4.xml			\
	picturemate_6.xml			\
	pro_gen1.xml				\
	pro_gen2.xml				\
	pro_pigment.xml				\
	pro_ultrachrome.xml			\
	pro_ultrachrome_k3.xml			\
	pro_ultrachrome_k34.xml			\
	pro_ultrachrome_k3v.xml			\
	pro_ultrachrome_k3v4.xml		\
	standard.xml				\
	standard_gen0.xml			\
	ultrachrome.xml				\
	wf40.xml				\
	wf500.xml				\
	x80.xml

xml-stamp: $(pkgxmldata_DATA) Makefile.am
	-rm -f $@ $@.tmp
	touch $@.tmp
	for f in $(pkgxmldata_DATA) ; do echo $$f >> $@.tmp; done
	mv $@.tmp $@

all-local: xml-stamp

dist-hook: xml-stamp
CLEANFILES = xmli18n-tmp.h xml-stamp xml-stamp.tmp

EXTRA_DIST = $(pkgxmldata_DATA)
