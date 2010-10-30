## $Id: Makefile.am,v 1.6 2005/11/21 11:16:22 rleigh Exp $
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

pkgsampledatadir = $(datadir)/$(PACKAGE)/samples


## Data

if INSTALL_SAMPLES
pkgsampledata_DATA = colorbars4.png colorsweep.png profile.jpg
endif


## Clean

MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = $(pkgsampledata_DATA)
