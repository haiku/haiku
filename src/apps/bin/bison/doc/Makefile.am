## Process this file with automake to produce Makefile.in -*-Makefile-*-
## Copyright (C) 2001, 2002, 2003 Free Software Foundation, Inc.

## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.

## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.

## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
## 02111-1307  USA

AM_MAKEINFOFLAGS = --no-split
info_TEXINFOS = bison.texinfo
man_MANS = bison.1
bison_TEXINFOS = gpl.texi fdl.texi

EXTRA_DIST = FAQ bison.1 refcard.tex

CLEANFILES = refcard.dvi refcard.log refcard.ps bison.fns

refcard.dvi: refcard.tex
	tex refcard.tex

refcard.ps: refcard.dvi
