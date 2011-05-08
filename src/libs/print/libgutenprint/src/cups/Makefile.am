## $Id: Makefile.am,v 1.120 2011/01/17 23:20:51 rlk Exp $
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

empty=
PACKAGE = cups

bindir = @cups_bindir@
sbindir = @cups_sbindir@

pkgdatadir = $(cups_conf_datadir)
pkglibdir = $(cups_conf_serverbin)
pkgsysconfdir = $(cups_conf_serverroot)

cupsexec_driverdir = $(pkglibdir)/driver
cupsexec_filterdir = $(pkglibdir)/filter
if CUPS_PPDS_AT_TOP_LEVEL
cups_modeldir = $(pkgdatadir)/model/
DONT_UNINSTALL_PPDS=true
else
cups_modeldir = $(pkgdatadir)/model/gutenprint/@GUTENPRINT_MAJOR_VERSION@.@GUTENPRINT_MINOR_VERSION@/
endif

LOCAL_CPPFLAGS = $(GUTENPRINT_CFLAGS) $(CUPS_CFLAGS)

STP_NONLS_ENV= STP_MODULE_PATH=$(top_builddir)/src/main/.libs:$(top_builddir)/src/main STP_DATA_PATH=$(top_srcdir)/src/xml

STP_ENV= $(STP_NONLS_ENV) STP_LOCALEDIR=$(top_srcdir)/src/cups/catalog

## Programs

if BUILD_TRANSLATED_CUPS_PPDS
if BUILD_GLOBALIZED_CUPS_PPDS
PPD = ppd-global
else
PPD = $(PPD_NLS_1)
endif
LINGUAS = $(ALL_LINGUAS)
TRANSLATE_PPDS = -DCUPS_TRANSLATED_PPDS
else
PPD = ppd-nonls
endif

if BUILD_SIMPLIFIED_CUPS_PPDS
BUILD_SIMPLE_PPDS=-DGENERATE_SIMPLIFIED_PPDS
endif

if BUILD_CUPS_1_2
cupsexec_driver_PROGRAMS = gutenprint.@GUTENPRINT_MAJOR_VERSION@.@GUTENPRINT_MINOR_VERSION@
endif

if BUILD_CUPS
bin_PROGRAMS = cups-calibrate
sbin_PROGRAMS = cups-genppd.@GUTENPRINT_MAJOR_VERSION@.@GUTENPRINT_MINOR_VERSION@
sbin_SCRIPTS = cups-genppdupdate
cupsexec_filter_PROGRAMS = rastertogutenprint.@GUTENPRINT_MAJOR_VERSION@.@GUTENPRINT_MINOR_VERSION@ commandtoepson commandtocanon

TESTS= test-ppds test-rastertogutenprint
noinst_SCRIPTS=test-rastertogutenprint
endif

if BUILD_GENPPD_STATIC
STATIC_LDOPTS=-static -export-dynamic
endif

cups_calibrate_SOURCES = cups-calibrate.c
cups_calibrate_LDADD = -lm

commandtocanon_SOURCES = commandtocanon.c
commandtocanon_LDADD = $(CUPS_LIBS)

commandtoepson_SOURCES = commandtoepson.c
commandtoepson_LDADD = $(CUPS_LIBS)

cups_genppd_@GUTENPRINT_MAJOR_VERSION@_@GUTENPRINT_MINOR_VERSION@_SOURCES = genppd.c i18n.c i18n.h
cups_genppd_@GUTENPRINT_MAJOR_VERSION@_@GUTENPRINT_MINOR_VERSION@_CFLAGS = -DALL_LINGUAS='"@ALL_LINGUAS@"' $(BUILD_SIMPLE_PPDS) $(TRANSLATE_PPDS)
cups_genppd_@GUTENPRINT_MAJOR_VERSION@_@GUTENPRINT_MINOR_VERSION@_LDADD = $(CUPS_LIBS) $(GENPPD_LIBS) $(GUTENPRINT_LIBS) @LIBICONV@
cups_genppd_@GUTENPRINT_MAJOR_VERSION@_@GUTENPRINT_MINOR_VERSION@_LDFLAGS = $(STATIC_LDOPTS)

gutenprint_@GUTENPRINT_MAJOR_VERSION@_@GUTENPRINT_MINOR_VERSION@_SOURCES = genppd.c i18n.c i18n.h
gutenprint_@GUTENPRINT_MAJOR_VERSION@_@GUTENPRINT_MINOR_VERSION@_CFLAGS = -DCUPS_DRIVER_INTERFACE -DALL_LINGUAS='"@ALL_LINGUAS@"' $(BUILD_SIMPLE_PPDS) $(TRANSLATE_PPDS)
gutenprint_@GUTENPRINT_MAJOR_VERSION@_@GUTENPRINT_MINOR_VERSION@_LDADD = $(CUPS_LIBS) $(GUTENPRINT_LIBS) @LIBICONV@
gutenprint_@GUTENPRINT_MAJOR_VERSION@_@GUTENPRINT_MINOR_VERSION@_LDFLAGS = $(STATIC_LDOPTS)

rastertogutenprint_@GUTENPRINT_MAJOR_VERSION@_@GUTENPRINT_MINOR_VERSION@_SOURCES = rastertoprinter.c i18n.c i18n.h
rastertogutenprint_@GUTENPRINT_MAJOR_VERSION@_@GUTENPRINT_MINOR_VERSION@_LDADD = $(CUPS_LIBS) $(GUTENPRINT_LIBS) @LIBICONV@
rastertogutenprint_@GUTENPRINT_MAJOR_VERSION@_@GUTENPRINT_MINOR_VERSION@_LDFLAGS = $(STATIC_LDOPTS)

## Data

if BUILD_CUPS
CUPS_PKG = calibrate.ppm
endif
if BUILD_CUPS
CUPS_CONF = command.types
endif

pkgdata_DATA = $(CUPS_PKG)
pkgsysconf_DATA = $(CUPS_CONF)

if BUILD_CUPS_PPDS
INSTALL_DATA_LOCAL_DEPS = ppd-stamp
endif

if USE_NLS
PPD_NLS_1 = ppd-nls
endif


install-data-local: $(INSTALL_DATA_LOCAL_DEPS)
	if test -n "$(CUPS_PKG)" -a -n "$(INSTALL_DATA_LOCAL_DEPS)" ; then \
	  $(mkdir_p) $(DESTDIR)$(cups_modeldir); \
	  cd ppd ; \
	  for language in * ; do \
	    cd ..; \
	    $(mkdir_p) $(DESTDIR)$(cups_modeldir)/$$language; \
	    cd ppd/$$language; \
	    for ppdfile in * ; do \
	      (cd ../..; $(INSTALL_DATA) ppd/$$language/$$ppdfile $(DESTDIR)$(cups_modeldir)/$$language) ; \
	    done; \
	  cd ..; \
	  done \
	fi
	$(mkdir_p) "$(DESTDIR)$(localedir)"
	for file in $(srcdir)/../../po/*.po; do \
	  lang=`basename $$file .po`; \
	  $(mkdir_p) "$(DESTDIR)$(localedir)/$$lang"; \
	  $(INSTALL_DATA) $$file "$(DESTDIR)$(localedir)/$$lang/gutenprint_$$lang.po"; \
	done

uninstall-local: $(INSTALL_DATA_LOCAL_DEPS)
# Remove PPDs installed in new PPD location
	if test -z "DONT_UNINSTALL_PPDS" ; then \
	  if test -n "$(INSTALL_DATA_LOCAL_DEPS)" ; then \
	    $(RM) -fr $(DESTDIR)$(cups_modeldir)/* ; \
	  fi ; \
	fi
	for file in $(srcdir)/../../po/*.po; do \
	  lang=`basename $$file .po`; \
	  $(RM) "$(DESTDIR)$(localedir)/$$lang/gutenprint_$$lang.po"; \
	done

install-data-hook:
# Remove unused directories in install tree
	-@echo 'Expect a number of "rmdir: Directory not empty" warnings'
	-@echo 'These messages are harmless and should be ignored.'
	-rmdir $(DESTDIR)$(cups_modeldir)
	-rmdir $(DESTDIR)$(pkgdatadir)
	-rmdir $(DESTDIR)$(cupsexec_driverdir)
	-rmdir $(DESTDIR)$(cupsexec_filterdir)
	-rmdir $(DESTDIR)$(bindir)
	-rmdir $(DESTDIR)$(pkglibdir)
	-rmdir $(DESTDIR)$(pkgsysconfdir)
	-rmdir `dirname $(DESTDIR)$(pkgdatadir)`
	-rmdir `dirname $(DESTDIR)$(pkglibdir)`
	-rmdir `dirname $(DESTDIR)$(pkgsysconfdir)`

.PHONY: ppd ppd-stamp-pre ppd-stamp-nonls ppd-stamp-nls ppd-stamp-phony ppd-catalog-clean ppd-clean

all-local: $(INSTALL_DATA_LOCAL_DEPS)

ppd: ppd-stamp

ppd-stamp: cups-genppd.@GUTENPRINT_MAJOR_VERSION@.@GUTENPRINT_MINOR_VERSION@ $(top_srcdir)/src/xml/xml-stamp
	$(MAKE) ppd-stamp-pre
	$(MAKE) ppd-stamp-phony
	touch ppd-stamp

ppd-stamp-phony: $(PPD)

ppd-catalog-clean:
	-$(RM) -r catalog

ppd-clean:
	-$(RM) -r ppd

ppd-stamp-pre: ppd-catalog-clean ppd-clean

ppd-catalog: ppd-catalog-clean
	$(mkdir_p) catalog
	for file in $(srcdir)/../../po/*.po; do \
	  lang=`basename $$file .po`; \
	  $(mkdir_p) "catalog/$$lang"; \
	  $(INSTALL_DATA) $$file "catalog/$$lang/gutenprint_$$lang.po"; \
	done

ppd-nonls: cups-genppd.@GUTENPRINT_MAJOR_VERSION@.@GUTENPRINT_MINOR_VERSION@
	$(mkdir_p) ppd/C
	$(MAKE) ppd-catalog-clean
	LC_ALL= LANG= LANGUAGE= $(STP_NONLS_ENV) ./cups-genppd.@GUTENPRINT_MAJOR_VERSION@.@GUTENPRINT_MINOR_VERSION@ @WHICH_PPDS@ -l C -p ppd/C

ppd-global: cups-genppd.@GUTENPRINT_MAJOR_VERSION@.@GUTENPRINT_MINOR_VERSION@
	@echo "Global PPDs:"
	$(mkdir_p) ppd/Global
	$(MAKE) ppd-catalog
	LC_ALL= LANG= LANGUAGE= $(STP_ENV) ./cups-genppd.@GUTENPRINT_MAJOR_VERSION@.@GUTENPRINT_MINOR_VERSION@ @WHICH_PPDS@ -p ppd/Global
	$(MAKE) ppd-catalog-clean

ppd-global-ln: cups-genppd.@GUTENPRINT_MAJOR_VERSION@.@GUTENPRINT_MINOR_VERSION@
	@echo "Global PPDs (localized numbers for testing):"
	$(mkdir_p) ppd/Global
	$(MAKE) ppd-catalog
	LC_ALL= LANG= LANGUAGE= $(STP_ENV) ./cups-genppd.@GUTENPRINT_MAJOR_VERSION@.@GUTENPRINT_MINOR_VERSION@ @WHICH_PPDS@ -p ppd/Global -N
	$(MAKE) ppd-catalog-clean

ppd-nls: cups-genppd.@GUTENPRINT_MAJOR_VERSION@.@GUTENPRINT_MINOR_VERSION@ ppd-nonls
	$(mkdir_p) ppd
	$(MAKE) ppd-catalog
	for language in `$(STP_ENV) ./cups-genppd.@GUTENPRINT_MAJOR_VERSION@.@GUTENPRINT_MINOR_VERSION@ -L` ; do \
	  mkdir ppd/$$language ; \
	  echo "$$language:" ; \
	  LC_ALL=$$language LANG=$$language LANGUAGE=$$language $(STP_ENV) ./cups-genppd.@GUTENPRINT_MAJOR_VERSION@.@GUTENPRINT_MINOR_VERSION@ @WHICH_PPDS@ -l $$language -p ppd/$$language; \
	done
	$(MAKE) ppd-catalog-clean

dist-hook:
	for dir in commands ; do \
	  if test -d $(srcdir)/$$dir ; then \
	    mkdir $(distdir)/$$dir; \
	    for dirfile in $(srcdir)/$$dir/*; do \
	      if test -f $$dirfile ; then \
	        cp -p $$dirfile $(distdir)/$$dir; \
	      fi \
	    done \
	  fi \
	done

clean-local: ppd-catalog-clean ppd-clean


## Clean

CLEANFILES = ppd-stamp
DISTCLEANFILES = cups-genppdupdate
MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = \
	COPYING \
	README \
	calibrate.ppm \
	command.txt \
	command.types \
	test-ppds
