#
AUTOMAKE_OPTIONS = ../src/ansi2knr

SUBDIRS = posix

noinst_LIBRARIES = libgreputils.a

INCLUDES = -I.. -I$(srcdir) -I../intl
DEFS = -DLIBDIR=\"$(libdir)\" @DEFS@

libgreputils_a_SOURCES = \
  closeout.c error.c exclude.c hard-locale.c \
  isdir.c obstack.c quotearg.c savedir.c xmalloc.c xstrtol.c  xstrtoumax.c

libgreputils_a_LIBADD = @LIBOBJS@ @ALLOCA@
noinst_HEADERS = \
  closeout.h error.h exclude.h fnmatch.h getopt.h \
  hard-locale.h obstack.h quotearg.h regex.h savedir.h xalloc.h xstrtol.h

EXTRA_DIST = strtol.c malloc.c realloc.c
