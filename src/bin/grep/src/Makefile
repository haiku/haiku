# Makefile.in generated automatically by automake 1.5 from Makefile.am.

# Copyright 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001
# Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.



SHELL = /bin/sh

srcdir = .
top_srcdir = ..

prefix = /usr/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include
pkgdatadir = $(datadir)/grep
pkglibdir = $(libdir)/grep
pkgincludedir = $(includedir)/grep
top_builddir = ..

ACLOCAL = ${SHELL} /boot/home/obosf/current/src/apps/bin/grep-2.5/missing --run aclocal
AUTOCONF = ${SHELL} /boot/home/obosf/current/src/apps/bin/grep-2.5/missing --run autoconf
AUTOMAKE = ${SHELL} /boot/home/obosf/current/src/apps/bin/grep-2.5/missing --run automake
AUTOHEADER = ${SHELL} /boot/home/obosf/current/src/apps/bin/grep-2.5/missing --run autoheader

INSTALL = /bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL}
INSTALL_HEADER = $(INSTALL_DATA)
transform = s,x,x,
NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :
host_alias = 
host_triplet = i586-pc-beos
AMTAR = ${SHELL} /boot/home/obosf/current/src/apps/bin/grep-2.5/missing --run tar
AWK = gawk
BUILD_INCLUDED_LIBINTL = yes
CATALOGS =  cs.gmo de.gmo el.gmo eo.gmo es.gmo et.gmo fr.gmo gl.gmo hr.gmo id.gmo it.gmo ja.gmo ko.gmo nl.gmo no.gmo pl.gmo pt_BR.gmo ru.gmo sl.gmo sv.gmo
CATOBJEXT = .gmo
CC = gcc
CPP = gcc -E
DATADIRNAME = share
DEPDIR = .deps
EXEEXT = 
GENCAT = gencat
GETCONF = 
GLIBC21 = no
GMOFILES =  cs.gmo de.gmo el.gmo eo.gmo es.gmo et.gmo fr.gmo gl.gmo hr.gmo id.gmo it.gmo ja.gmo ko.gmo nl.gmo no.gmo pl.gmo pt_BR.gmo ru.gmo sl.gmo sv.gmo
GMSGFMT = :
INSTALL_STRIP_PROGRAM = ${SHELL} $(install_sh) -c -s
INSTOBJEXT = .mo
INTLBISON = bison
INTLLIBS = $(top_builddir)/intl/libintl.a 
INTLOBJS = $(GETTOBJS)
INTL_LIBTOOL_SUFFIX_PREFIX = 
LIBICONV = 
MKINSTALLDIRS = ./mkinstalldirs
MSGFMT = :
OBJEXT = o
PACKAGE = grep
POFILES =  cs.po de.po el.po eo.po es.po et.po fr.po gl.po hr.po id.po it.po ja.po ko.po nl.po no.po pl.po pt_BR.po ru.po sl.po sv.po
POSUB = po
RANLIB = ranlib
SEP = :
U = 
USE_INCLUDED_LIBINTL = yes
USE_NLS = yes
VERSION = 2.5.1
am__include = include
am__quote = 
install_sh = /boot/home/obosf/current/src/apps/bin/grep-2.5/install-sh
datadir = $(prefix)/share

AUTOMAKE_OPTIONS = ansi2knr no-dependencies

LN = ln

bin_PROGRAMS = grep
bin_SCRIPTS = egrep fgrep
grep_SOURCES = grep.c dfa.c kwset.c search.c \
               grepmat.c

noinst_HEADERS = grep.h dfa.h kwset.h getpagesize.h system.h

LDADD = $(top_builddir)/intl/libintl.a  ../lib/libgreputils.a
localedir = $(datadir)/locale
INCLUDES = -I../intl -I$(top_srcdir)/lib -DLOCALEDIR=\"$(localedir)\"

EXTRA_DIST = \
             dosbuf.c \
             vms_fab.c vms_fab.h

subdir = src
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = $(top_builddir)/config.h
CONFIG_CLEAN_FILES =
bin_PROGRAMS = grep$(EXEEXT)
PROGRAMS = $(bin_PROGRAMS)

am_grep_OBJECTS = grep$U.$(OBJEXT) dfa$U.$(OBJEXT) kwset$U.$(OBJEXT) \
	search$U.$(OBJEXT) grepmat$U.$(OBJEXT)
grep_OBJECTS = $(am_grep_OBJECTS)
grep_LDADD = $(LDADD)
grep_DEPENDENCIES = ../lib/libgreputils.a
grep_LDFLAGS =
SCRIPTS = $(bin_SCRIPTS)


DEFS = -DHAVE_CONFIG_H
DEFAULT_INCLUDES =  -I. -I$(srcdir) -I$(top_builddir)
CPPFLAGS = 
LDFLAGS = 
LIBS = 
depcomp =
COMPILE = $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	$(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
CCLD = $(CC)
LINK = $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@
CFLAGS = -g -O2
DIST_SOURCES = $(grep_SOURCES)
HEADERS = $(noinst_HEADERS)

DIST_COMMON = $(noinst_HEADERS) Makefile.am Makefile.in ansi2knr.1 \
	ansi2knr.c
SOURCES = $(grep_SOURCES)

all: all-am

.SUFFIXES:
.SUFFIXES: .c .o .obj
$(srcdir)/Makefile.in:  Makefile.am  $(top_srcdir)/configure.in $(ACLOCAL_M4)
	cd $(top_srcdir) && \
	  $(AUTOMAKE) --gnu  src/Makefile
Makefile:  $(srcdir)/Makefile.in  $(top_builddir)/config.status
	cd $(top_builddir) && \
	  CONFIG_HEADERS= CONFIG_LINKS= \
	  CONFIG_FILES=$(subdir)/$@ $(SHELL) ./config.status
install-binPROGRAMS: $(bin_PROGRAMS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	@list='$(bin_PROGRAMS)'; for p in $$list; do \
	  p1=`echo $$p|sed 's/$(EXEEXT)$$//'`; \
	  if test -f $$p \
	  ; then \
	    f=`echo $$p1|sed '$(transform);s/$$/$(EXEEXT)/'`; \
	   echo " $(INSTALL_PROGRAM_ENV) $(INSTALL_PROGRAM) $$p $(DESTDIR)$(bindir)/$$f"; \
	   $(INSTALL_PROGRAM_ENV) $(INSTALL_PROGRAM) $$p $(DESTDIR)$(bindir)/$$f; \
	  else :; fi; \
	done

uninstall-binPROGRAMS:
	@$(NORMAL_UNINSTALL)
	@list='$(bin_PROGRAMS)'; for p in $$list; do \
	  f=`echo $$p|sed 's/$(EXEEXT)$$//;$(transform);s/$$/$(EXEEXT)/'`; \
	  echo " rm -f $(DESTDIR)$(bindir)/$$f"; \
	  rm -f $(DESTDIR)$(bindir)/$$f; \
	done

clean-binPROGRAMS:
	-test -z "$(bin_PROGRAMS)" || rm -f $(bin_PROGRAMS)
grep$(EXEEXT): $(grep_OBJECTS) $(grep_DEPENDENCIES) 
	@rm -f grep$(EXEEXT)
	$(LINK) $(grep_LDFLAGS) $(grep_OBJECTS) $(grep_LDADD) $(LIBS)
install-binSCRIPTS: $(bin_SCRIPTS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	@list='$(bin_SCRIPTS)'; for p in $$list; do \
	  f="`echo $$p|sed '$(transform)'`"; \
	  if test -f $$p; then \
	    echo " $(INSTALL_SCRIPT) $$p $(DESTDIR)$(bindir)/$$f"; \
	    $(INSTALL_SCRIPT) $$p $(DESTDIR)$(bindir)/$$f; \
	  elif test -f $(srcdir)/$$p; then \
	    echo " $(INSTALL_SCRIPT) $(srcdir)/$$p $(DESTDIR)$(bindir)/$$f"; \
	    $(INSTALL_SCRIPT) $(srcdir)/$$p $(DESTDIR)$(bindir)/$$f; \
	  else :; fi; \
	done

uninstall-binSCRIPTS:
	@$(NORMAL_UNINSTALL)
	@list='$(bin_SCRIPTS)'; for p in $$list; do \
	  f="`echo $$p|sed '$(transform)'`"; \
	  echo " rm -f $(DESTDIR)$(bindir)/$$f"; \
	  rm -f $(DESTDIR)$(bindir)/$$f; \
	done

mostlyclean-compile:
	-rm -f *.$(OBJEXT) core *.core

distclean-compile:
	-rm -f *.tab.c

ANSI2KNR = 
ansi2knr: ansi2knr.$(OBJEXT)
	$(LINK) ansi2knr.$(OBJEXT) $(LIBS)
ansi2knr.$(OBJEXT): $(CONFIG_HEADER)

clean-krextra:
	-rm -f ansi2knr

mostlyclean-kr:
	-rm -f *_.c

.c.o:
	$(COMPILE) -c `test -f $< || echo '$(srcdir)/'`$<

.c.obj:
	$(COMPILE) -c `cygpath -w $<`
dfa_.c: dfa.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/dfa.c; then echo $(srcdir)/dfa.c; else echo dfa.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > dfa_.c || rm -f dfa_.c
grep_.c: grep.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/grep.c; then echo $(srcdir)/grep.c; else echo grep.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > grep_.c || rm -f grep_.c
grepmat_.c: grepmat.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/grepmat.c; then echo $(srcdir)/grepmat.c; else echo grepmat.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > grepmat_.c || rm -f grepmat_.c
kwset_.c: kwset.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/kwset.c; then echo $(srcdir)/kwset.c; else echo kwset.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > kwset_.c || rm -f kwset_.c
search_.c: search.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/search.c; then echo $(srcdir)/search.c; else echo search.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > search_.c || rm -f search_.c
dfa_.$(OBJEXT) grep_.$(OBJEXT) grepmat_.$(OBJEXT) kwset_.$(OBJEXT) \
search_.$(OBJEXT) : $(ANSI2KNR)
uninstall-info-am:

tags: TAGS

ID: $(HEADERS) $(SOURCES) $(LISP) $(TAGS_FILES)
	list='$(SOURCES) $(HEADERS) $(TAGS_FILES)'; \
	unique=`for i in $$list; do \
	    if test -f "$$i"; then echo $$i; else echo $(srcdir)/$$i; fi; \
	  done | \
	  $(AWK) '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	mkid -fID $$unique $(LISP)

TAGS:  $(HEADERS) $(SOURCES)  $(TAGS_DEPENDENCIES) \
		$(TAGS_FILES) $(LISP)
	tags=; \
	here=`pwd`; \
	list='$(SOURCES) $(HEADERS) $(TAGS_FILES)'; \
	unique=`for i in $$list; do \
	    if test -f "$$i"; then echo $$i; else echo $(srcdir)/$$i; fi; \
	  done | \
	  $(AWK) '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	test -z "$(ETAGS_ARGS)$$unique$(LISP)$$tags" \
	  || etags $(ETAGS_ARGS) $$tags  $$unique $(LISP)

GTAGS:
	here=`CDPATH=: && cd $(top_builddir) && pwd` \
	  && cd $(top_srcdir) \
	  && gtags -i $(GTAGS_ARGS) $$here

distclean-tags:
	-rm -f TAGS ID GTAGS GRTAGS GSYMS GPATH

DISTFILES = $(DIST_COMMON) $(DIST_SOURCES) $(TEXINFOS) $(EXTRA_DIST)

top_distdir = ..
distdir = $(top_distdir)/$(PACKAGE)-$(VERSION)

distdir: $(DISTFILES)
	@for file in $(DISTFILES); do \
	  if test -f $$file; then d=.; else d=$(srcdir); fi; \
	  dir=`echo "$$file" | sed -e 's,/[^/]*$$,,'`; \
	  if test "$$dir" != "$$file" && test "$$dir" != "."; then \
	    $(mkinstalldirs) "$(distdir)/$$dir"; \
	  fi; \
	  if test -d $$d/$$file; then \
	    cp -pR $$d/$$file $(distdir) \
	    || exit 1; \
	  else \
	    test -f $(distdir)/$$file \
	    || cp -p $$d/$$file $(distdir)/$$file \
	    || exit 1; \
	  fi; \
	done
check-am: all-am
check: check-am
all-am: Makefile $(ANSI2KNR) $(PROGRAMS) $(SCRIPTS) $(HEADERS)

installdirs:
	$(mkinstalldirs) $(DESTDIR)$(bindir) $(DESTDIR)$(bindir)

install: install-am
install-exec: install-exec-am
install-data: install-data-am
uninstall: uninstall-am

install-am: all-am
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am

installcheck: installcheck-am
install-strip:
	$(MAKE) $(AM_MAKEFLAGS) INSTALL_PROGRAM="$(INSTALL_STRIP_PROGRAM)" \
	  `test -z '$(STRIP)' || \
	    echo "INSTALL_PROGRAM_ENV=STRIPPROG='$(STRIP)'"` install
mostlyclean-generic:

clean-generic:

distclean-generic:
	-rm -f Makefile $(CONFIG_CLEAN_FILES) stamp-h stamp-h[0-9]*

maintainer-clean-generic:
	@echo "This command is intended for maintainers to use"
	@echo "it deletes files that may require special tools to rebuild."
clean: clean-am

clean-am: clean-binPROGRAMS clean-generic clean-krextra mostlyclean-am

distclean: distclean-am

distclean-am: clean-am distclean-compile distclean-generic \
	distclean-tags

dvi: dvi-am

dvi-am:

info: info-am

info-am:

install-data-am:

install-exec-am: install-binPROGRAMS install-binSCRIPTS

install-info: install-info-am

install-man:

installcheck-am:

maintainer-clean: maintainer-clean-am

maintainer-clean-am: distclean-am maintainer-clean-generic

mostlyclean: mostlyclean-am

mostlyclean-am: mostlyclean-compile mostlyclean-generic mostlyclean-kr

uninstall-am: uninstall-binPROGRAMS uninstall-binSCRIPTS \
	uninstall-info-am

.PHONY: GTAGS all all-am check check-am clean clean-binPROGRAMS \
	clean-generic clean-krextra distclean distclean-compile \
	distclean-generic distclean-tags distdir dvi dvi-am info \
	info-am install install-am install-binPROGRAMS \
	install-binSCRIPTS install-data install-data-am install-exec \
	install-exec-am install-info install-info-am install-man \
	install-strip installcheck installcheck-am installdirs \
	maintainer-clean maintainer-clean-generic mostlyclean \
	mostlyclean-compile mostlyclean-generic mostlyclean-kr tags \
	uninstall uninstall-am uninstall-binPROGRAMS \
	uninstall-binSCRIPTS uninstall-info-am


egrep:
	echo '#!/bin/sh' >$@
	echo 'exec grep -E $${1+"$$@"}' >>$@
	chmod a+x $@

fgrep:
	echo '#!/bin/sh' >$@
	echo 'exec grep -F $${1+"$$@"}' >>$@
	chmod a+x $@
# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
