EXAMPLE_DIR = $(datadir)/doc/libxslt-python-$(LIBXSLT_VERSION)/examples

TESTSPY=	\
    basic.py	\
    exslt.py	\
    extelem.py	\
    extfunc.py

XMLS=		\
    test.xml	\
    test.xsl

EXTRAS=		\
    pyxsltproc.py

EXTRA_DIST = $(TESTSPY) $(XMLS) $(EXTRAS)

if WITH_PYTHON
tests: $(TESTSPY)
	-@(PYTHONPATH="`pwd`/..:`pwd`/../.libs:$(srcdir)/.."; \
	  if test "x$(LIBXML_SRC)" != "x" ; then PYTHONPATH="$$PYTHONPATH:$(LIBXML_SRC)/python:$(LIBXML_SRC)/python/.libs"; fi; \
	  export PYTHONPATH; \
	   for test in $(TESTSPY) ; do echo "-- $$test" ; (cd $(srcdir) && $(PYTHON) $$test ); done)
else
tests:
endif

clean:
	rm -f *.pyc core

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(EXAMPLE_DIR)
	-(for test in $(TESTSPY) $(XMLS) $(EXTRAS); \
	  do @INSTALL@ -m 0644 $(srcdir)/$$test $(DESTDIR)$(EXAMPLE_DIR) ; done)

