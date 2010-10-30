## $Id: Makefile.am,v 1.4 2010/07/05 15:18:08 rlk Exp $
## Copyright (C) 2000 Roger Leigh
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


## Data

noinst_HEADERS = gutenprint-intl.h gutenprint-intl-internal.h

pkginclude_HEADERS = \
	gutenprint.h \
	gutenprint-module.h \
	array.h \
	bit-ops.h \
	channel.h \
	color.h \
	curve-cache.h \
	curve.h \
	dither.h \
	image.h \
	list.h \
	module.h \
	mxml.h \
	paper.h \
	path.h \
	printers.h \
	sequence.h \
	string-list.h \
	util.h \
	vars.h \
	weave.h \
	xml.h

nodist_pkginclude_HEADERS = gutenprint-version.h


## Clean

DISTCLEANFILES = gutenprint-version.h
MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = gutenprint-version.h.in

TAGS_FILES = gutenprint-version.h.in
