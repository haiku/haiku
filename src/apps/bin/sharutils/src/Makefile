# Generated automatically from Makefile.in by configure.
# Makefile for program source directory in GNU Shar utilities package.
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
PROGRAMS = shar unshar uudecode uuencode
SCRIPTS = mailshar mail-files remsync

SHELL = /bin/sh

srcdir = .
top_srcdir = ..

prefix = /usr/local
exec_prefix = ${prefix}
transform = s,x,x,
bindir = $(exec_prefix)/bin
datadir = $(prefix)/share
localedir = $(datadir)/locale
subdir = src

AR = ar
CC = gcc
INSTALL = /bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_SCRIPT = ${INSTALL} -m 755
RANLIB = ranlib

DEFS = -DLOCALEDIR=\"$(localedir)\" -DHAVE_CONFIG_H
LIBS = ../lib/libshar.a ../intl/libintl.a ../lib/libshar.a 
CFLAGS = -g -O
CPPFLAGS = 
LDFLAGS = 

COMPILE = $(CC) -c $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) $(XCFLAGS)
LINK = $(CC) $(LDFLAGS) -o $@

SOURCES = shar.c unshar.c uudecode.c uuencode.c
OBJECTS = shar.o unshar.o uudecode.o uuencode.o
COMSRCS = encode.c
COMOBJS = encode.o
DISTFILES = ChangeLog Makefile.in mail-files.in mailshar.in remsync.in \
$(SOURCES) $(COMSRCS)

.SUFFIXES:
.SUFFIXES: .c .o
.c.o:
	$(COMPILE) $<

INCLUDES = -I$(srcdir) -I.. -I$(top_srcdir)/lib -I../intl -I$(top_srcdir)/intl

all: $(PROGRAMS) $(SCRIPTS)


shar: shar.o
	$(LINK) shar.o $(COMOBJS) $(LIBS)

unshar: unshar.o
	$(LINK) unshar.o $(LIBS)

uudecode: uudecode.o
	$(LINK) uudecode.o $(LIBS)

uuencode: uuencode.o
	$(LINK) uuencode.o $(LIBS)


$(PROGRAMS): ../lib/libshar.a ../intl/libintl.a
shar: $(COMOBJS)

$(OBJECTS) $(COMOBJS): ../config.h ../lib/system.h

install: install-exec install-data
install-exec: all
	$(top_srcdir)/mkinstalldirs $(bindir)
	for name in $(PROGRAMS); do \
	  $(INSTALL_PROGRAM) $$name \
		$(bindir)/`echo $$name | sed '$(transform)'`; \
	done;
	for name in $(SCRIPTS); do \
	  $(INSTALL_SCRIPT) $$name \
		$(bindir)/`echo $$name | sed '$(transform)'`; \
	done
install-data:

uninstall:
	for name in $(PROGRAMS); do \
	  rm -f $(bindir)/`echo $$name | sed '$(transform)'`; \
	done

check: all

tags: TAGS

TAGS: $(HEADERS) $(SOURCES) $(COMSRCS)
	here=`pwd`; cd $(srcdir) && etags -o $$here/TAGS \
	  $(HEADERS) $(SOURCES) $(COMSRCS)

id: ID

ID: $(HEADERS) $(SOURCES) $(COMSRCS)
	here=`pwd`; cd $(srcdir) && mkid -f$$here/ID \
	  $(HEADERS) $(SOURCES) $(COMSRCS)

mostlyclean:
	rm -f *.o core core.*

clean: mostlyclean
	rm -f shar unshar uudecode uuencode

distclean: clean
	rm -f Makefile ID TAGS
	rm -f mailshar mail-files remsync

maintainer-clean: distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

distdir = ../$(PACKAGE)-$(VERSION)/$(subdir)
dist: Makefile $(DISTFILES)
	for file in $(DISTFILES); do \
	  ln $(srcdir)/$$file $(distdir) 2> /dev/null \
	    || cp -p $(srcdir)/$$file $(distdir); \
	done

Makefile: Makefile.in ../config.status
	cd .. && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= ./config.status

mailshar: $(srcdir)/mailshar.in ../config.status
	cd .. && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= ./config.status

mail-files: $(srcdir)/mail-files.in ../config.status
	cd .. && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= ./config.status

remsync: $(srcdir)/remsync.in ../config.status
	cd .. && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= ./config.status

# Tell versions [3.59,3.63) of GNU make not to export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
