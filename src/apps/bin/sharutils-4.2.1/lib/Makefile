# Generated automatically from Makefile.in by configure.
# Makefile for the support library subdirectory of the GNU Shar Utilities
# Copyright (C) 1995 Free Software Foundation, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

PACKAGE = sharutils
VERSION = 4.2.1

SHELL = /bin/sh

srcdir = .
top_srcdir = ..

subdir = lib

AR = ar
CC = gcc
RANLIB = ranlib

DEFS = -DHAVE_CONFIG_H
CFLAGS = -g -O
CPPFLAGS = 
LIBS = libshar.a

COMPILE = $(CC) -c $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) $(XCFLAGS)

# If your system doesn't have alloca, or the one provided is bad, define this.
ALLOCA = 
ALLOCA_SRC = alloca.c

# If your systems has the GNU getopt functions available you may comment
# out the following line.
GETOPT = getopt.o getopt1.o
GETOPTSRC = getopt.c getopt1.c

HEADERS = getopt.h md5.h pathmax.h system.h
SOURCES = $(ALLOCA_SRC) basename.c error.c $(GETOPTSRC) md5.c memset.c \
mktime.c stpcpy.c strftime.c whoami.c xgetcwd.c xmalloc.c xstrdup.c
OBJECTS = $(ALLOCA) error.o $(GETOPT) md5.o xgetcwd.o xmalloc.o xstrdup.o \
whoami.o  basename.o
DISTFILES = ChangeLog Makefile.in $(HEADERS) $(SOURCES)

.SUFFIXES:
.SUFFIXES: .c .o
.c.o:
	$(COMPILE) $<


INCLUDES = -I$(srcdir) -I.. -I../intl -I$(top_srcdir)/intl

all: libshar.a $(ANSI2KNR)

libshar.a: $(OBJECTS)
	rm -f libshar.a
	$(AR) cru libshar.a $(OBJECTS)
	$(RANLIB) libshar.a

ansi2knr: ansi2knr.o
	$(LINK) ansi2knr.o $(LIBS)

install install-exec install-data: all

uninstall:

check: all

$(OBJECTS): ../config.h
error.o getopt.o xmalloc.o: ../intl/libgettext.h
getopt.o: getopt.h

tags: TAGS

TAGS: $(HEADERS) $(SOURCES)
	here=`pwd`; cd $(srcdir) && etags -o $$here/TAGS $(HEADERS) $(SOURCES)

id: ID

ID: $(HEADERS) $(SOURCES)
	here=`pwd`; cd $(srcdir) && mkid -f$$here/ID $(HEADERS) $(SOURCES)

mostlyclean:
	rm -f *.a *.o core core.*

clean: mostlyclean

distclean: clean
	rm -f Makefile ID TAGS ansi2knr

maintainer-clean: distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

distdir = ../$(PACKAGE)-$(VERSION)/$(subdir)
dist: Makefile $(DISTFILES)
	for file in $(DISTFILES); do \
	  ln $(srcdir)/$$file $(distdir) 2> /dev/null \
	    || cp -p $(srcdir)/$$file $(distdir); \
	done

Makefile: $(srcdir)/Makefile.in ../config.status
	cd .. && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= ./config.status

# Tell versions [3.59,3.63) of GNU make not to export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
