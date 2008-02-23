# Makefile for GNU tar library.			-*- Makefile -*-

# Copyright (C) 1994, 1995, 1996, 1997, 1999, 2000, 2001, 2003, 2004,
# 2005, 2006, 2007 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

include gnulib.mk

rmt-command.h : Makefile
	rm -f $@-t $@
	echo "#ifndef DEFAULT_RMT_COMMAND" >> $@-t
	echo "# define DEFAULT_RMT_COMMAND \"$(DEFAULT_RMT_DIR)/`echo rmt | sed '$(transform)'`$(EXEEXT)\"" >> $@-t
	echo "#endif" >> $@-t
	mv $@-t $@
BUILT_SOURCES += rmt-command.h
CLEANFILES += rmt-command.h rmt-command.h-t

noinst_HEADERS += system.h system-ioctl.h rmt.h paxlib.h stdopen.h
libtar_a_SOURCES += \
  paxerror.c paxexit.c paxlib.h paxnames.c \
  prepargs.c prepargs.h \
  rtapelib.c \
  rmt.h \
  stdopen.c stdopen.h \
  system.h system-ioctl.h

libtar_a_LIBADD += $(LIBOBJS)
libtar_a_DEPENDENCIES += $(LIBOBJS)
