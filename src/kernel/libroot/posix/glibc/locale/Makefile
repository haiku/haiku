# Copyright (C) 1991, 92, 1995-1999, 2000, 2001 Free Software Foundation, Inc.
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

#
#	Makefile for locales.
#
subdir	:= locale

headers		= locale.h bits/locale.h langinfo.h xlocale.h
distribute	= localeinfo.h categories.def iso-639.def iso-3166.def \
		  iso-4217.def weight.h weightwc.h strlen-hash.h elem-hash.h \
		  indigits.h indigitswc.h outdigits.h outdigitswc.h \
		  coll-lookup.h C-translit.h.in C-translit.h gen-translit.pl \
		  $(addprefix programs/, \
			      locale.c localedef.c \
			      $(localedef-modules:=.c) $(locale-modules:=.c) \
			      $(lib-modules:=.c) config.h simple-hash.h \
			      charmap-kw.gperf charmap-kw.h locfile-token.h \
			      locfile-kw.gperf locfile-kw.h linereader.h \
			      locfile.h charmap.h repertoire.h localedef.h \
			      3level.h charmap-dir.h)
routines	= setlocale findlocale loadlocale localeconv nl_langinfo \
		  nl_langinfo_l mb_cur_max \
		  newlocale duplocale freelocale
tests		= tst-C-locale
categories	= ctype messages monetary numeric time paper name \
		  address telephone measurement identification collate
aux		= $(categories:%=lc-%) $(categories:%=C-%) SYS_libc C_name \
		  xlocale
others		= localedef locale
#others-static	= localedef locale
install-bin	= localedef locale
extra-objs	= $(localedef-modules:=.o) $(locale-modules:=.o) \
		  $(lib-modules:=.o)

extra-libs	= libBrokenLocale
extra-libs-others = $(extra-libs)

libBrokenLocale-routines = broken_cur_max

subdir-dirs	= programs
vpath %.c programs
vpath %.h programs
vpath %.gperf programs

localedef-modules	:= $(categories:%=ld-%) charmap linereader locfile \
			   repertoire
locale-modules		:= locale-spec
lib-modules		:= charmap-dir simple-hash xmalloc xstrdup


GPERF = gperf
GPERFFLAGS = -acCgopt -k1,2,5,9,$$ -L ANSI-C

include ../Rules

programs/%-kw.h: programs/%-kw.gperf
	$(GPERF) $(GPERFFLAGS) -N $(@F:-kw.h=_hash) $< > $@.new
	mv -f $@.new $@

$(objpfx)localedef: $(localedef-modules:%=$(objpfx)%.o)
$(objpfx)locale: $(locale-modules:%=$(objpfx)%.o)
$(objpfx)localedef $(objpfx)locale: $(lib-modules:%=$(objpfx)%.o)

C-translit.h: C-translit.h.in gen-translit.pl
	$(PERL) gen-translit.pl < $< > $@.tmp
	mv -f $@.tmp $@
ifeq ($(with-cvs),yes)
	test ! -d CVS || cvs $(CVSOPTS) commit -mRegenerated $@
endif

localepath = "$(localedir):$(i18ndir)"

locale-CPPFLAGS := -DLOCALE_PATH='$(localepath)' \
		   -DLOCALEDIR='"$(localedir)"' \
		   -DLOCALE_ALIAS_PATH='"$(msgcatdir)"' \
		   -DCHARMAP_PATH='"$(i18ndir)/charmaps"' \
		   -DREPERTOIREMAP_PATH='"$(i18ndir)/repertoiremaps"' \
		   -DLOCSRCDIR='"$(i18ndir)/locales"' -DHAVE_CONFIG_H \
		   -Iprograms

CFLAGS-charmap.c = -Wno-write-strings -Wno-char-subscripts
CFLAGS-locfile.c = -Wno-write-strings -Wno-char-subscripts
CFLAGS-charmap-dir.c = -Wno-write-strings

# Depend on libc.so so a DT_NEEDED is generated in the shared objects.
# This ensures they will load libc.so for needed symbols if loaded by
# a statically-linked program that hasn't already loaded it.
$(objpfx)libBrokenLocale.so: $(common-objpfx)libc.so \
			     $(common-objpfx)libc_nonshared.a
