# Makefile for program source directory in GNU NLS utilities package.
# Copyright (C) 1995-1997, 2000, 2001 by Ulrich Drepper <drepper@gnu.ai.mit.edu>
#
# This file file be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU Public License
# but which still want to provide support for the GNU gettext functionality.
# Please note that the actual code is *not* freely available.

PACKAGE = sed
VERSION = 4.0.9

# These two variables depend on the location of this directory.
subdir = po
top_builddir = ..

SHELL = /bin/sh


srcdir = .
top_srcdir = ..


prefix = /usr/local
exec_prefix = ${prefix}
datadir = ${prefix}/share
localedir = $(datadir)/locale
gettextsrcdir = $(datadir)/gettext/po

INSTALL = /bin/install -c
INSTALL_DATA = ${INSTALL} -m 644
mkinstalldirs = $(mkdir_p)
mkdir_p = mkdir -m 0755 -p --

CC = gcc
GMSGFMT = :
MSGFMT = :
XGETTEXT = :
MSGMERGE = msgmerge

DEFS = -DHAVE_CONFIG_H
CFLAGS = -g -O2
CPPFLAGS = 

INCLUDES = -I.. -I$(top_srcdir)/intl

COMPILE = $(CC) -c $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) $(XCFLAGS)

POFILES =  ca.po cs.po da.po de.po el.po eo.po es.po et.po fi.po fr.po ga.po gl.po he.po hr.po hu.po id.po it.po ja.po ko.po nl.po pl.po pt_BR.po ro.po ru.po sk.po sl.po sr.po sv.po tr.po zh_CN.po
GMOFILES =  ca.gmo cs.gmo da.gmo de.gmo el.gmo eo.gmo es.gmo et.gmo fi.gmo fr.gmo ga.gmo gl.gmo he.gmo hr.gmo hu.gmo id.gmo it.gmo ja.gmo ko.gmo nl.gmo pl.gmo pt_BR.gmo ro.gmo ru.gmo sk.gmo sl.gmo sr.gmo sv.gmo tr.gmo zh_CN.gmo
DISTFILES = ChangeLog Makefile.in.in POTFILES.in $(PACKAGE).pot \
$(POFILES) $(GMOFILES)

POTFILES = \
     ../sed/compile.c \
     ../sed/execute.c \
     ../sed/regexp.c \
     ../sed/sed.c \
     ../lib/utils.c \
     ../lib/regcomp.c \
     ../lib/regexec.c \
     ../lib/regex_internal.c

CATALOGS =  ca.gmo cs.gmo da.gmo de.gmo el.gmo eo.gmo es.gmo et.gmo fi.gmo fr.gmo ga.gmo gl.gmo he.gmo hr.gmo hu.gmo id.gmo it.gmo ja.gmo ko.gmo nl.gmo pl.gmo pt_BR.gmo ro.gmo ru.gmo sk.gmo sl.gmo sr.gmo sv.gmo tr.gmo zh_CN.gmo

.SUFFIXES:
.SUFFIXES: .c .o .po .pox .gmo .mo

.c.o:
	$(COMPILE) $<

.po.pox:
	$(MAKE) $(PACKAGE).pot
	$(MSGMERGE) $< $(srcdir)/$(PACKAGE).pot -o $*.pox

.po.mo:
	$(MSGFMT) -o $@ $<

.po.gmo:
	file=$(srcdir)/`echo $* | sed 's,.*/,,'`.gmo \
	  && rm -f $$file && $(GMSGFMT) --statistics -o $$file $<


all: all-yes

all-yes: $(CATALOGS)
all-no:

# Note: Target 'all' must not depend on target '$(srcdir)/$(PACKAGE).pot',
# otherwise packages like GCC can not be built if only parts of the source
# have been downloaded.

$(srcdir)/$(PACKAGE).pot: $(POTFILES) $(srcdir)/POTFILES.in
	$(XGETTEXT) --default-domain=$(PACKAGE) --directory=$(top_srcdir) \
	  --add-comments --keyword=_ --keyword=N_ \
	  --files-from=$(srcdir)/POTFILES.in \
	&& test ! -f $(PACKAGE).po \
	   || ( rm -f $(srcdir)/$(PACKAGE).pot \
		&& mv $(PACKAGE).po $(srcdir)/$(PACKAGE).pot )


install: install-exec install-data
install-exec:
install-data: install-data-yes
	if test "$(PACKAGE)" = "gettext"; then \
	  $(mkinstalldirs) $(DESTDIR)$(gettextsrcdir); \
	  $(INSTALL_DATA) $(srcdir)/Makefile.in.in \
			  $(DESTDIR)$(gettextsrcdir)/Makefile.in.in; \
	else \
	  : ; \
	fi
install-data-no: all
install-data-yes: all
	$(mkinstalldirs) $(DESTDIR)$(datadir)
	@catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed 's/\.gmo$$//'`; \
	  dir=$(localedir)/$$lang/LC_MESSAGES; \
	  $(mkinstalldirs) $(DESTDIR)$$dir; \
	  if test -r $$cat; then \
	    $(INSTALL_DATA) $$cat $(DESTDIR)$$dir/$(PACKAGE).mo; \
	    echo "installing $$cat as $(DESTDIR)$$dir/$(PACKAGE).mo"; \
	  else \
	    $(INSTALL_DATA) $(srcdir)/$$cat $(DESTDIR)$$dir/$(PACKAGE).mo; \
	    echo "installing $(srcdir)/$$cat as" \
		 "$(DESTDIR)$$dir/$(PACKAGE).mo"; \
	  fi; \
	done

# Define this as empty until I found a useful application.
installcheck:

uninstall:
	catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed 's/\.gmo$$//'`; \
	  rm -f $(DESTDIR)$(localedir)/$$lang/LC_MESSAGES/$(PACKAGE).mo; \
	done
	if test "$(PACKAGE)" = "gettext"; then \
	  rm -f $(DESTDIR)$(gettextsrcdir)/Makefile.in.in; \
	else \
	  : ; \
	fi

check: all

dvi info tags TAGS ID:

mostlyclean:
	rm -f core core.* *.pox $(PACKAGE).po *.new.po
	rm -fr *.o

clean: mostlyclean

distclean: clean
	rm -f Makefile Makefile.in POTFILES *.mo

maintainer-clean: distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."
	rm -f $(GMOFILES)

distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)
dist distdir:
	$(MAKE) update-po
	@$(MAKE) dist2
# This is a separate target because 'update-po' must be executed before.
dist2: $(DISTFILES)
	dists="$(DISTFILES)"; \
	for file in $$dists; do \
	  if test -f $$file; then dir=.; else dir=$(srcdir); fi; \
	  cp -p $$dir/$$file $(distdir); \
	done

update-po: Makefile
	$(MAKE) $(PACKAGE).pot
	if test "$(PACKAGE)" = "gettext"; then PATH=`pwd`/../src:$$PATH; fi; \
	outdir=`pwd`; cd $(srcdir); \
	catalogs='$(GMOFILES)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed 's/\.gmo$$//'`; \
	  echo "$$lang:"; \
	  if $(MSGMERGE) $$lang.po $(PACKAGE).pot -o $$outdir/$$lang.pox; then \
	    if cmp -s $$outdir/$$lang.pox $$lang.po; then \
	      rm -f $$outdir/$$lang.pox; \
	    else \
	      if mv -f $$outdir/$$lang.pox $$lang.po; then \
	        :; \
	      else \
	        rm -f $$outdir/$$lang.pox; \
	        echo "msgmerge for $$cat failed! (cannot write $$lang.po)"; \
	      fi; \
	    fi; \
	  else \
	    echo "msgmerge for $$cat failed!"; \
	    rm -f $$outdir/$$lang.pox; \
	  fi; \
	done
	$(MAKE) update-gmo

update-gmo: Makefile $(GMOFILES)
	@:

Makefile: Makefile.in.in $(top_builddir)/config.status POTFILES.in
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@.in CONFIG_HEADERS= \
	       $(SHELL) ./config.status

# Tell versions [3.59,3.63) of GNU make not to export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
