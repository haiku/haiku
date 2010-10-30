## $Id: Makefile.am,v 1.23 2008/04/01 00:53:41 rlk Exp $
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

# Conditional installation of manpages is currently broken in automake...

if BUILD_CUPS
CUPS_MAN = \
	cups-calibrate.8 \
	cups-genppd.8 \
	cups-genppdupdate.8
endif

if BUILD_ESCPUTIL
ESCPUTIL_MAN = escputil.1
endif

if BUILD_GHOSTSCRIPT
GHOSTSCRIPT_MAN = ijsgutenprint.1
endif

man_MANS = $(CUPS_MAN) $(ESCPUTIL_MAN) $(GHOSTSCRIPT_MAN)

EXTRA_man_MANS = \
	cups-calibrate.8 \
	cups-genppd.8 \
	cups-genppdupdate.8 \
	escputil.1 \
	ijsgutenprint.1


## Clean

DISTCLEANFILES = $(MAIN_MAN) $(EXTRA_man_MANS)
MAINTAINERCLEANFILES = Makefile.in

