## $Id: Makefile.am,v 1.53 2010/08/07 02:30:38 rlk Exp $
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

DISTDIRS = foomatic-templates-ijs
LOCAL_CPPFLAGS = $(GUTENPRINT_CFLAGS)

if BUILD_FOOMATIC_3
BUILD_FOOMATIC = 1
endif

STP_ENV= STP_MODULE_PATH=$(top_builddir)/src/main/.libs:$(top_builddir)/src/main STP_DATA_PATH=$(top_srcdir)/src/xml

LC_C_ENV= LC_ALL=C LANG=C LANGUAGE=C

FOOMATIC_GENERATOR= $(STP_ENV) $(LC_C_ENV) ./foomatic-generator

## Programs

if BUILD_FOOMATIC
noinst_PROGRAMS = printer_options printer_margins paper_sizes gutenprint-version \
	printers
endif

printer_options_SOURCES = printer_options.c
printer_options_LDADD = $(GUTENPRINT_LIBS)

printer_margins_SOURCES = printer_margins.c
printer_margins_LDADD = $(GUTENPRINT_LIBS)

gutenprint_version_SOURCES = gutenprint-version.c
gutenprint_version_LDADD = $(GUTENPRINT_LIBS)

paper_sizes_SOURCES = paper_sizes.c
paper_sizes_LDADD = $(GUTENPRINT_LIBS)

printers_SOURCES = printers.c
printers_LDADD = $(GUTENPRINT_LIBS)


## Rules

if BUILD_FOOMATIC
INSTALL_DATA_LOCAL_DEPS = foomatic-data
else
INSTALL_DATA_LOCAL_DEPS =
endif

if BUILD_FOOMATIC_3
FOOMATIC_GENERATOR_FOOMATIC_3_OPTS = -f 3
else
FOOMATIC_GENERATOR_FOOMATIC_3_OPTS =
endif

all-local: $(INSTALL_DATA_LOCAL_DEPS)

install-data-local: $(INSTALL_DATA_LOCAL_DEPS)
	if test -n "$(INSTALL_DATA_LOCAL_DEPS)" ; then \
	  $(MAKE) install-kit FOOMATIC_DB=foomatic-db/gutenprint-ijs.@GUTENPRINT_MAJOR_VERSION@.@GUTENPRINT_MINOR_VERSION@ ; \
	  $(MAKE) install-kit FOOMATIC_DB=foomatic-db/gutenprint-ijs-simplified.@GUTENPRINT_MAJOR_VERSION@.@GUTENPRINT_MINOR_VERSION@ ; \
	fi

install-kit:
	if test -n "$(DESTDIR)" ; then \
	  $(mkdir_p) $(DESTDIR)`$(FOOMATIC_KITLOAD) -l` ; \
	  $(FOOMATIC_KITLOAD) -f -d $(DESTDIR) -k $(FOOMATIC_DB) ; \
	else \
	  $(FOOMATIC_KITLOAD) -f -k $(FOOMATIC_DB) ; \
	fi

clean-local:
	-$(RM) -r foomatic-db

foomatic-data: foomatic-stamp

foomatic-stamp: foomatic-generator $(noinst_PROGRAMS) mk_printerlist $(top_builddir)/src/xml/xml-stamp
	-$(RM) -r foomatic-db
	$(FOOMATIC_GENERATOR) -t $(srcdir)/foomatic-templates $(FOOMATIC_GENERATOR_EXTRA_OPTS) $(FOOMATIC_GENERATOR_FOOMATIC_3_OPTS)
	$(FOOMATIC_GENERATOR) -s -t $(srcdir)/foomatic-templates $(FOOMATIC_GENERATOR_EXTRA_OPTS) $(FOOMATIC_GENERATOR_FOOMATIC_3_OPTS)
	touch foomatic-stamp

dist-hook:
	for dir in $(DISTDIRS) ; do \
	  if test -d $(srcdir)/$$dir ; then \
	    mkdir $(distdir)/$$dir; \
	    for dirfile in $(srcdir)/$$dir/*.xml; do \
	      if test -f $$dirfile ; then \
	        cp -p $$dirfile $(distdir)/$$dir; \
	      fi \
	    done \
	  fi \
	done

$(top_builddir)/src/xml/xml-stamp:
	cd $(top_builddir)/src/xml; $(MAKE) xml-stamp

## Clean

CLEANFILES = foomatic-stamp
MAINTAINERCLEANFILES = Makefile.in
