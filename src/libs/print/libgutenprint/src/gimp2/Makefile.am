## $Id: Makefile.am,v 1.7 2006/07/04 02:57:58 rlk Exp $
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


## Variables



## Programs

if BUILD_GIMP2
if BUILD_GIMP2_AS_GUTENPRINT
gimp2_plug_in_PROGRAMS = gutenprint
else
gimp2_plug_in_PROGRAMS = print
endif
endif

LOCAL_CPPFLAGS = $(GUTENPRINT_CFLAGS) $(GIMP2_CFLAGS)

COMMON_PRINT_SOURCES = \
	print-image-gimp.c \
	print-intl.h \
	print.c \
	print_gimp.h


print_SOURCES = $(COMMON_PRINT_SOURCES) print-print.c
print_LDADD = $(GIMP2_LIBS) $(GUTENPRINTUI2_LIBS)
print_DEPENDENCIES = $(GUTENPRINTUI2_LIBS) $(GUTENPRINT_LIBS)

gutenprint_SOURCES = $(COMMON_PRINT_SOURCES) print-gutenprint.c
gutenprint_LDADD = $(print_LDADD)
gutenprint_DEPENDENCIES = $(print_DEPENDENCIES)

## Clean

MAINTAINERCLEANFILES = Makefile.in
CLEANFILES = print gutenprint
