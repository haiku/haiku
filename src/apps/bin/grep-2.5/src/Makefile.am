## Process this file with automake to create Makefile.in
AUTOMAKE_OPTIONS = ansi2knr no-dependencies

LN = ln

bin_PROGRAMS = grep
grep_SOURCES = grep.c dfa.c kwset.c search.c \
               grepmat.c
noinst_HEADERS = grep.h dfa.h kwset.h getpagesize.h system.h

LDADD = @INTLLIBS@ ../lib/libgreputils.a
datadir = $(prefix)/@DATADIRNAME@
localedir = $(datadir)/locale
INCLUDES = -I../intl -I$(top_srcdir)/lib -DLOCALEDIR=\"$(localedir)\"

EXTRA_DIST = \
             dosbuf.c \
             vms_fab.c vms_fab.h

install-exec-local:
	@SYMLINK@ @SYMLINKFROM@ $(bindir)/egrep
	@SYMLINK@ @SYMLINKFROM@ $(bindir)/fgrep
