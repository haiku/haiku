# Makefile for PDFlib's Python binding
# $Id: Makefile 14574 2005-10-29 16:27:43Z bonefish $

top_builddir	= ../../..

include $(top_builddir)/config/mkcommon.inc

INCLUDES	= $(PDFLIBINC) $(PYINCLUDE)
# special CFLAGS, as -ansi is not working here.
CFLAGS          = $(DEFS) $(DEFINES) $(INCLUDES)

LANG_LIBDIR 	= $(PYTHONLIBDIR)

LIBNAME		= pdflib_py$(LA)
OBJ		= pdflib_py$(LO)
SRC		= pdflib_py.c


include $(top_builddir)/config/mkbind.inc

test:: all
	@-(. ./pdflib_py$(LA); cd .libs; cp $$library_names ..)
	-$(LIBTOOL_EXE) python hello.py
	-$(LIBTOOL_EXE) python image.py
	-$(LIBTOOL_EXE) python pdfclock.py
	-$(LIBTOOL_EXE) python chartab.py
	-$(LIBTOOL_EXE) python invoice.py
	-$(LIBTOOL_EXE) python businesscard.py
	-$(LIBTOOL_EXE) python quickreference.py

smoke:: test

install:: $(LIBNAME)
	@-if test -f .libs/libpdflib_py.dylib; then		\
	    $(INSTALL_DATA) .libs/pdflib_py.so $(LANG_LIBDIR);	\
	else							\
	    $(LIBTOOL) --mode=install $(INSTALL_DATA) 		\
		$(LIBNAME) $(LANG_LIBDIR);			\
	fi

uninstall::
	@-if test -f .libs/libpdflib_py.dylib; then			 \
	    $(LIBTOOL) --mode=uninstall $(RM) $(LANG_LIBDIR)/pdflib_py.so\
	else 								 \
	    $(LIBTOOL) --mode=uninstall $(RM) $(LANG_LIBDIR)/$(LIBNAME) \
	fi

clean::
	@-if test -f pdflib_py.la; then			\
	    (. ./pdflib_py.la; $(RM) $$library_names)	\
	fi
	$(RM) smoke_c_?.pdf
	$(RM) hello.pdf image.pdf pdfclock.pdf chartab.pdf invoice.pdf
	$(RM) businesscard.pdf quickreference.pdf

# Automatically generated dependencies
