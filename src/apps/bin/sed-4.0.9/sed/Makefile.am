## Process this file with automake to produce Makefile.in
bin_PROGRAMS = sed

localedir = $(datadir)/locale

sed_SOURCES = sed.c compile.c execute.c regexp.c fmt.c
noinst_HEADERS = sed.h

AM_CPPFLAGS = -I$(top_srcdir)/lib -I$(top_srcdir)/intl \
	-I$(top_srcdir) -I$(top_builddir)/lib \
	-DLOCALEDIR=\"$(localedir)\"

sed_LDADD = ../lib/libsed.a @INTLLIBS@
sed_DEPENDENCIES = ../lib/libsed.a

$(PROGRAMS): $(LDADD)


