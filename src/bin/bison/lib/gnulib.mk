# This file is generated automatically by "bootstrap".
BUILT_SOURCES += $(ALLOCA_H)
EXTRA_DIST += alloca_.h

# We need the following in order to create an <alloca.h> when the system
# doesn't have one that works with the given compiler.
all-local $(lib_OBJECTS): $(ALLOCA_H)
alloca.h: alloca_.h
	cp $(srcdir)/alloca_.h $@-t
	mv $@-t $@
MOSTLYCLEANFILES += alloca.h alloca.h-t

lib_SOURCES += argmatch.h argmatch.c

lib_SOURCES += dirname.h dirname.c basename.c stripslash.c


lib_SOURCES += exit.h

lib_SOURCES += exitfail.h exitfail.c


lib_SOURCES += getopt.h getopt.c getopt1.c getopt_int.h

lib_SOURCES += gettext.h

lib_SOURCES += hard-locale.h hard-locale.c

lib_SOURCES += hash.h hash.c


lib_SOURCES += mbswidth.h mbswidth.c


lib_SOURCES += quote.h quote.c

lib_SOURCES += quotearg.h quotearg.c


BUILT_SOURCES += $(STDBOOL_H)
EXTRA_DIST += stdbool_.h

# We need the following in order to create an <stdbool.h> when the system
# doesn't have one that works.
all-local $(lib_OBJECTS): $(STDBOOL_H)
stdbool.h: stdbool_.h
	sed -e 's/@''HAVE__BOOL''@/$(HAVE__BOOL)/g' < $(srcdir)/stdbool_.h > $@-t
	mv $@-t $@
MOSTLYCLEANFILES += stdbool.h stdbool.h-t

lib_SOURCES += stpcpy.h

lib_SOURCES += strndup.h


lib_SOURCES += unlocked-io.h

lib_SOURCES += xalloc.h xmalloc.c xstrdup.c

lib_SOURCES += xstrndup.h xstrndup.c

