# Makefile for libxml2 python library
AUTOMAKE_OPTIONS = 1.4 foreign

SUBDIRS= . tests

INCLUDES = \
	-I$(PYTHON_INCLUDES) \
	 $(LIBXML_CFLAGS) \
	-I$(top_srcdir)/libxslt \
	-I$(top_srcdir) \
	-I../libexslt

DOCS_DIR = $(datadir)/doc/libxslt-python-$(LIBXSLT_VERSION)
# libxsltclass.txt is generated
DOCS = TODO

EXTRA_DIST = 			\
	libxslt.c		\
	types.c			\
	generator.py		\
	libxml_wrap.h		\
	libxslt_wrap.h		\
	libxsl.py		\
	libxslt-python-api.xml	\
	$(DOCS)

libxsltmod_la_LDFLAGS = -module -avoid-version -L$(top_builddir)/libxslt/.libs \
                        -L$(top_builddir)/libexslt/.libs

if WITH_PYTHON
mylibs = \
	$(top_builddir)/libxslt/libxslt.la \
	$(top_builddir)/libexslt/libexslt.la

all-local: libxslt.py

python_LTLIBRARIES = libxsltmod.la

libxsltmod_la_SOURCES = libxslt.c types.c libxslt-py.c
libxsltmod_la_LIBADD = $(mylibs) 

libxslt.py: $(srcdir)/libxsl.py $(srcdir)/libxsltclass.py
	cat $(srcdir)/libxsl.py $(srcdir)/libxsltclass.py > libxslt.py

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(pythondir)
	@INSTALL@ -m 0644 libxslt.py $(DESTDIR)$(pythondir)
	$(mkinstalldirs) $(DESTDIR)$(DOCS_DIR)
	@(for doc in $(DOCS) ; \
	   do @INSTALL@ -m 0644 $(srcdir)/$$doc $(DESTDIR)$(DOCS_DIR) ; done)

GENERATE = generator.py
API_DESC = $(top_srcdir)/doc/libxslt-api.xml $(srcdir)/libxslt-python-api.xml
GENERATED= $(srcdir)/libxsltclass.py \
           $(srcdir)/libxslt-export.c \
	   $(srcdir)/libxslt-py.c \
	   $(srcdir)/libxslt-py.h \
	   $(srcdir)/libxsltclass.txt

$(GENERATED): gen_prog

gen_prog:  $(srcdir)/$(GENERATE) $(API_DESC)
	cd $(srcdir) && $(PYTHON) $(GENERATE)
	touch gen_prog

$(libxsltmod_la_OBJECTS): $(GENERATED)

else
all: 
endif
tests test: all
	cd tests && $(MAKE) tests

CLEANFILES= $(GENERATED) *.o libxslt.so *.pyc libxslt.py gen_prog
