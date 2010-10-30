## $Id: Makefile.am,v 1.9 2009/05/30 14:10:32 rlk Exp $
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

SUBDIRS =					\
	inks					\
	inputslots				\
	model					\
	media					\
	mediasizes				\
	qualitypresets				\
	weaves

xml-stamp: media/xml-stamp			\
	   model/xml-stamp			\
	   inputslots/xml-stamp			\
	   inks/xml-stamp			\
	   mediasizes/xml-stamp			\
	   qualitypresets/Makefile.am		\
	   weaves/Makefile.am			\
	   Makefile.am
	-rm -f $@ $@.tmp
	touch $@.tmp
	sed 's;^;media/;' media/xml-stamp >> $@.tmp
	sed 's;^;model/;' model/xml-stamp >> $@.tmp
	sed 's;^;inputslots/;' inputslots/xml-stamp >> $@.tmp
	sed 's;^;inks/;' inks/xml-stamp >> $@.tmp
	sed 's;^;mediasizes/;' mediasizes/xml-stamp >> $@.tmp
	sed 's;^;weaves/;' weaves/xml-stamp >> $@.tmp
	sed 's;^;qualitypresets/;' qualitypresets/xml-stamp >> $@.tmp
	mv $@.tmp $@

all-local: xml-stamp

dist-hook: xml-stamp
## Clean

CLEANFILES = xml-stamp

MAINTAINERCLEANFILES = Makefile.in
