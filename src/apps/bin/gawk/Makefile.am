#
# Makefile.am --- automake input file for gawk
#
# Copyright (C) 2000-2003 the Free Software Foundation, Inc.
#
# This file is part of GAWK, the GNU implementation of the
# AWK Programming Language.
#
# GAWK is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# GAWK is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
#

## process this file with automake to produce Makefile.in

# Automatic de-ANSI-fication if needed, make .bz2 files also.
AUTOMAKE_OPTIONS = ansi2knr dist-bzip2

# This undocumented variable insures that aclocal runs
# correctly after changing configure.ac
ACLOCAL_AMFLAGS = -I m4

# This insures that make flags get passed down to child makes.
AM_MAKEFLAGS = 'CFLAGS=$(CFLAGS)' 'LDFLAGS=$(LDFLAGS)'

# Stuff to include in the dist that doesn't need it's own
# Makefile.am files
EXTRA_DIST = \
	COPYING \
	FUTURES \
	INSTALL \
	LIMITATIONS \
	NEWS \
	POSIX.STD \
	PROBLEMS \
	README_d \
	bisonfix.sed \
	config.guess \
	config.rpath  \
	config.sub \
	depcomp \
	extension \
	fixvers \
	m4 \
	missing \
	missing_d \
	pc \
	posix \
	regcomp.c \
	regex_internal.c \
	regex_internal.h \
	regexec.c \
	unsupported \
	vms \
	ylwrap

# The order to do things in.
# Build in intl first in case we need the local gettext version.
# Build explicitly  "." next in order to build gawk first, so
# that `make check' without a prior `make' works.
SUBDIRS = \
	intl \
	. \
	awklib \
	doc \
	po \
	test

# what to make and install
bin_PROGRAMS = gawk pgawk

# sources for both gawk and pgawk
base_sources = \
	array.c \
	awk.h \
	awkgram.y \
	builtin.c \
	custom.h \
	ext.c \
	field.c \
	gawkmisc.c \
	getopt.c \
	getopt.h \
	getopt1.c \
	gettext.h \
	io.c \
	main.c \
	msg.c \
	node.c \
	patchlev.h \
	protos.h \
	random.c \
	random.h \
	re.c \
	regex.c \
	regex.h \
	replace.c \
	version.in \
	version.c

gawk_SOURCES = $(base_sources) eval.c profile.c
pgawk_SOURCES = $(base_sources) eval_p.c profile_p.c

# Get extra libs as needed
LDADD = @LIBINTL@ @SOCKET_LIBS@

# Set autoconf Makefile.in variables correctly
datadir = @datadir@
awkdatadir = @datadir@/awk
libexecdir = @libexecdir@/awk

# stuff for compiling gawk/pgawk
DEFPATH="\".$(PATH_SEPARATOR)$(awkdatadir)\""

DEFS= -DDEFPATH=$(DEFPATH) -DHAVE_CONFIG_H -DGAWK -DLOCALEDIR="\"$(datadir)/locale\""

INCLUDES = -I$(srcdir)/intl

# We want hard links for install-exec-hook, below
LN= ln

# First, add a link from gawk to gawk-X.Y.Z
# Same for pgawk
# For systems where gawk is awk, add a link to awk
install-exec-hook:
	(fullname=gawk-`./gawk --version | sed 1q | awk '{print $$3}'` ; \
	cd $(DESTDIR)$(bindir); \
	$(LN) gawk $$fullname 2>/dev/null ; \
	$(LN) pgawk p$$fullname 2>/dev/null ; \
	if [ ! -f awk$(EXEEXT) ]; \
	then	$(LN_S) gawk$(EXEEXT) awk$(EXEEXT); \
	fi; exit 0)

# Undo the above when uninstalling
uninstall-links:
	(cd $(DESTDIR)$(bindir); \
	if [ -f awk$(EXEEXT) ] && cmp awk$(EXEEXT) gawk$(EXEEXT) > /dev/null; then rm -f awk$(EXEEXT); fi ; \
	fullname=gawk-`./gawk --version | sed 1q | ./gawk '{print $$3}'` ; \
	rm -f $$fullname p$$fullname; exit 0)

uninstall-recursive: uninstall-links

# force there to be a gawk executable before running tests
check-local: gawk$(EXEEXT) pgawk$(EXEEXT)

# A little extra clean up when making distributions.
# FIXME: most of this rule should go away upon switching to libtool.
dist-hook:
	cd $(distdir)/extension ; rm -f *.o *.so

# Special rules for individual files
awkgram.c: awkgram.y
	$(YACC) $(AM_YFLAGS) $(YFLAGS) $< && sed -f $(srcdir)/bisonfix.sed < y.tab.c > $*.c && $(RM) y.tab.c
	if test -f y.tab.h; then \
	if cmp -s y.tab.h $*.h; then rm -f y.tab.h; else mv y.tab.h $*.h; fi; \
	else :; fi

# These force version and patchlevel to be up to date based
# on what's in configure.ac.  No-one else but me should have
# to use them. So there.
patchlev.h: configure.ac
	$(srcdir)/fixvers -p

version.c: configure.ac version.in
	$(srcdir)/fixvers -v

# This is for my development & testing.
efence: gawk
	$(CC) $(LDFLAGS) -o gawk $$(ls *.o | grep -v '_p.o$$') $(LIBS) -lefence
