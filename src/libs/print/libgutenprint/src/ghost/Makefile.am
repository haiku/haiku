## $Id: Makefile.am,v 1.38 2005/11/21 11:16:23 rleigh Exp $
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


## Clean

MAINTAINERCLEANFILES = Makefile.in

## Variables

LOCAL_CPPFLAGS = $(GUTENPRINT_CFLAGS) @IJS_CFLAGS@


## Programs

if BUILD_GHOSTSCRIPT
bin_PROGRAMS = ijsgutenprint.@GUTENPRINT_MAJOR_VERSION@.@GUTENPRINT_MINOR_VERSION@
ijsgutenprint_@GUTENPRINT_MAJOR_VERSION@_@GUTENPRINT_MINOR_VERSION@_SOURCES = ijsgutenprint.c
ijsgutenprint_@GUTENPRINT_MAJOR_VERSION@_@GUTENPRINT_MINOR_VERSION@_LDADD = $(GUTENPRINT_LIBS) $(IJS_LIBS)
ijsgutenprint_@GUTENPRINT_MAJOR_VERSION@_@GUTENPRINT_MINOR_VERSION@_LDFLAGS = $(STATIC_LDOPTS)
endif
