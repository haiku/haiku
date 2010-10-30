## $Id: Makefile.am,v 1.22 2008/02/27 12:28:06 rlk Exp $
## Copyright (C) 2001 Andy Stewart and Roger Leigh
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

MANUAL_SRC_FILES = \
	copying.xml \
	dither.xml \
	escp2.xml \
	gutenprint.xml \
	gpl-appendix.xml \
	introduction.xml \
	new-printer.xml \
	problems.xml \
	using.xml \
	weave.xml

HTMLDIR = reference-html

STANDARD_TARGETS = html

DISTHOOKDIRS = $(HTMLDIR)


if MAINTAINER_MODE
MAINT_MODE = true
else
MAINT_MODE = false
endif

if MAINTAINER_MODE
ALL_LOCAL_TARGETS = docs
else
ALL_LOCAL_TARGETS = $(STANDARD_TARGETS)
endif

all-local: $(ALL_LOCAL_TARGETS)


## Rules

dist-hook: html-stamp
	for dir in $(DISTHOOKDIRS) ; do \
	  if test -d $(srcdir)/$$dir ; then \
	    mkdir $(distdir)/$$dir; \
	    for dirfile in $(srcdir)/$$dir/*; do \
	      if test -f $$dirfile ; then \
	        cp -p $$dirfile $(distdir)/$$dir ; \
	      fi ; \
	      if test -d $$dirfile ; then \
	        cp -pR $$dirfile $(distdir)/$$dir ; \
	      fi ; \
	    done \
	  fi \
	done
	for file in $(DISTHOOKFILES) ; do \
	  if test -f $$file ; then \
	    cp -p $$file $(distdir) ; \
	fi ; \
	done

docs: html pdf

pdf: gutenprint.pdf

gutenprint.pdf:  $(MANUAL_SRC_FILES)
	@if [ -z "$(DB2PDF)" ] ; then \
	  echo "No db2pdf: cannot build documentation!"; \
	  exit 1; \
	fi
	if test $(srcdir) = '.' ; then \
	  : ; \
	else \
	  for file in $(MANUAL_SRC_FILES) ; do \
	    if test -L $$file ; then \
	      $(RM) -f $$file ; \
	    fi ; \
	    $(LN_S) -f $(srcdir)/$$file $$file ; \
	  done ; \
	fi ; \
	$(DB2PDF) gutenprint.xml

html: html-stamp

# This ugly workaround with SOURCE is because db2html doesn't seem to like
# "./users-guide.sgml".  Note that db2ps does not have the same limitation.
# html-stamp should *only* be called by html--it's just a timestamp!

html-stamp: $(MANUAL_SRC_FILES)
	@if [ -z "$(DB2HTML)" ] ; then \
	  echo "No db2html: cannot build documentation!"; \
	  exit 1; \
	fi
	if test $(srcdir) = '.' ; then \
	  : ; \
	else \
	  for file in $(MANUAL_SRC_FILES) ; do \
	    if test -L $$file ; then \
	      $(RM) -f $$file ; \
	    fi ; \
	    $(LN_S) -f $(srcdir)/$$file $$file ; \
	  done ; \
	fi
	$(DB2HTML) gutenprint.xml
	-$(RM) -rf $(HTMLDIR)
	mv gutenprint $(HTMLDIR)
	chmod a+rx $(HTMLDIR)
	touch html-stamp

install-data-local: $(STANDARD_TARGETS)
	if test -n '$(STANDARD_TARGETS)' ; then \
	  $(mkdir_p) $(DESTDIR)$(datadir)/$(PACKAGE)/doc/$(HTMLDIR) ; \
	  if test -f gutenprint.pdf ; then \
	    $(INSTALL_DATA) gutenprint.pdf $(DESTDIR)$(datadir)/$(PACKAGE)/doc ; \
	  elif test -f $(srcdir)/gutenprint.pdf ; then \
	    $(INSTALL_DATA) $(srcdir)/gutenprint.pdf $(DESTDIR)$(datadir)/$(PACKAGE)/doc ; \
	  fi ; \
	  if test -d $(HTMLDIR) ; then \
	  HTMLGENDIR="$(HTMLDIR)" ; \
	  elif test -d $(srcdir)/$(HTMLDIR) ; then \
	    HTMLGENDIR="$(srcdir)/$(HTMLDIR)" ; \
	  else \
	    exit 1 ; \
	  fi ; \
	  for file in $$HTMLGENDIR/*.html $$HTMLGENDIR/*.css ; do \
	    if test -f $$file ; then \
	      $(INSTALL_DATA) $$file $(DESTDIR)$(datadir)/$(PACKAGE)/doc/$(HTMLDIR) ; \
	    fi ; \
	  done ; \
	fi

uninstall-local:
	-$(RM) -r $(DESTDIR)$(datadir)/$(PACKAGE)/doc

CLEAN_MANUALS = \
	$(RM) -f gutenprint.pdf gutenprint.ps;	\
	$(RM) -rf *html-stamp $(HTMLDIR)

clean-local:
	if test $(srcdir) = "." -a $(MAINT_MODE) = false ; then \
	  echo "clean-local: Not removing $(HTMLDIR)" ; \
	  echo 'clean-local: Not removing gutenprint.pdf' ; \
	  echo 'clean-local: Not removing gutenprint.ps' ; \
	else \
	  $(CLEAN_MANUALS) ; \
	fi
	if test $(srcdir) = "." ; then \
	  : ; \
	else \
	  $(RM) -f $(MANUAL_SRC_FILES) ; \
	fi
	-$(RM) -rf *.tex *.log *.aux *.dvi *.gz *.out *.junk *.out *.fot
	-$(RM) -rf db2html* DB2HTML* CATALOG.local

veryclean:
	$(MAKE) MAINT_MODE=true clean

maintainer-clean-local:
	$(CLEAN_MANUALS)


## Clean

MAINTAINERCLEANFILES = Makefile.in *html-stamp


EXTRA_DIST = \
	$(MANUAL_SRC_FILES) \
	gutenprint.pdf \
	html-stamp

.PHONY: docs html ps pdf
