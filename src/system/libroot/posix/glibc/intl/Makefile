# Copyright (C) 1995-1999, 2000, 2001, 2002 Free Software Foundation, Inc.
# This file is part of the GNU C Library.

# The GNU C Library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# The GNU C Library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.

# You should have received a copy of the GNU Lesser General Public
# License along with the GNU C Library; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA.

# Makefile for intl subdirectory: message handling code from GNU gettext.

subdir = intl
headers = libintl.h
routines = bindtextdom dcgettext dgettext gettext	\
	   dcigettext dcngettext dngettext ngettext \
	   finddomain loadmsgcat localealias textdomain	\
	   l10nflist explodename plural plural-exp
distribute = gmo.h gettextP.h hash-string.h loadinfo.h locale.alias \
	     plural.y plural-exp.h po2test.sed tst-gettext.sh tst-translit.sh \
	     translit.po tst-gettext2.sh tstlang1.po tstlang2.po tstcodeset.po\
	     tst-codeset.sh plural-eval.c

test-srcs := tst-gettext tst-translit tst-gettext2 tst-codeset
tests = tst-ngettext

before-compile = $(objpfx)msgs.h

install-others = $(inst_msgcatdir)/locale.alias

generated = msgs.h mtrace-tst-gettext tst-gettext.mtrace
generated-dirs := domaindir localedir

include ../Makeconfig

ifneq (no,$(BISON))
plural.c: plural.y
	$(BISON) $(BISONFLAGS) $@ $^
ifeq ($(with-cvs),yes)
	test ! -d CVS || cvs $(CVSOPTS) commit -m'$(BISON) $(BISONFLAGS) $@ $^' $@
endif
endif
$(objpfx)plural.o: plural.c

include ../Rules

ifeq (no,$(cross-compiling))
ifeq (yes,$(build-shared))
ifneq ($(strip $(MSGFMT)),:)
tests: $(objpfx)tst-translit.out $(objpfx)tst-gettext2.out \
       $(objpfx)tst-codeset.out
ifneq (no,$(PERL))
tests: $(objpfx)mtrace-tst-gettext
endif
endif
$(objpfx)mtrace-tst-gettext: $(objpfx)tst-gettext.out
	$(common-objpfx)malloc/mtrace $(objpfx)tst-gettext.mtrace > $@
$(objpfx)tst-gettext.out: tst-gettext.sh $(objpfx)tst-gettext
	$(SHELL) -e $< $(common-objpfx) $(common-objpfx)intl/ \
	  $(objpfx)tst-gettext.mtrace
$(objpfx)tst-translit.out: tst-translit.sh $(objpfx)tst-translit
	$(SHELL) -e $< $(common-objpfx) $(common-objpfx)intl/
$(objpfx)tst-gettext2.out: tst-gettext2.sh $(objpfx)tst-gettext2
	$(SHELL) -e $< $(common-objpfx) $(common-objpfx)intl/
$(objpfx)tst-codeset.out: tst-codeset.sh $(objpfx)tst-codeset
	$(SHELL) -e $< $(common-objpfx) $(common-objpfx)intl/
endif
endif

$(objpfx)msgs.h: po2test.sed ../po/de.po
	$(make-target-directory)
	sed -f $^ > $@

CFLAGS-tst-gettext.c = -DTESTSTRS_H=\"$(objpfx)msgs.h\"
CFLAGS-tst-gettext2.c = -DOBJPFX=\"$(objpfx)\"
CFLAGS-tst-translit.c = -DOBJPFX=\"$(objpfx)\"
CFLAGS-tst-codeset.c = -DOBJPFX=\"$(objpfx)\"

$(objpfx)tst-translit.out: $(objpfx)tst-gettext.out
$(objpfx)tst-gettext2.out: $(objpfx)tst-gettext.out
$(objpfx)tst-codeset.out: $(objpfx)tst-gettext.out

CPPFLAGS += -D'LOCALEDIR="$(msgcatdir)"' \
	    -D'LOCALE_ALIAS_PATH="$(msgcatdir)"'
BISONFLAGS = --yacc --name-prefix=__gettext --output

$(inst_msgcatdir)/locale.alias: locale.alias $(+force)
	$(do-install)
