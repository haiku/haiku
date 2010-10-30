## $Id: Makefile.am,v 1.27 2007/12/02 02:44:29 rlk Exp $
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

pkgdatadir = $(datadir)/$(PACKAGE)/samples

TESTS = run-testpattern-1 run-testpattern-2


## Programs

if BUILD_TESTPATTERN
bin_PROGRAMS = testpattern
noinst_PROGRAMS = printers printer_options
noinst_SCRIPTS = run-testpattern-2
endif

AM_LFLAGS = -i
AM_YFLAGS = -d

testpattern_SOURCES = testpattern.c testpatterny.y testpatternl.l testpattern.h
testpattern_LDADD = $(GUTENPRINT_LIBS)

testpatternl.o: testpatterny.o
testpattern.o: testpatterny.o

printers_LDADD = $(GUTENPRINT_LIBS)
printer_options_LDADD = $(GUTENPRINT_LIBS)


## Data

if INSTALL_SAMPLES
pkgdata_DATA = testpattern.sample extended.sample
endif


## Clean

MAINTAINERCLEANFILES = Makefile.in testpatternl.c testpatterny.c testpatterny.h

EXTRA_DIST =  testpatterny.h $(pkgdata_DATA) run-testpattern run-testpattern-1

