# Makefile for PDFlib's Perl binding
# $Id: Makefile 14574 2005-10-29 16:27:43Z bonefish $

top_builddir	= ../../..

include $(top_builddir)/config/mkcommon.inc

INCLUDES	= $(PDFLIBINC) $(PERLBOOLDEF) $(PERLINCLUDE)
# special CFLAGS, as -ansi is not working here.
CFLAGS          = $(DEFS) $(DEFINES) $(INCLUDES)

PERLBIN		= $(PERL) -I. -I.libs
LANG_LIBDIR 	= $(PERLLIBDIR)
BINDEXTERNALLIBS= $(PERLLINK)

LIBNAME		= pdflib_pl$(LA)
OBJ		= pdflib_pl$(LO)
SRC		= pdflib_pl.c

include $(top_builddir)/config/mkbind.inc

test:: all
	-$(PERLBIN) hello.pl
	-$(PERLBIN) image.pl
	-$(PERLBIN) pdfclock.pl
	-$(PERLBIN) chartab.pl
	-$(PERLBIN) invoice.pl
	-$(PERLBIN) businesscard.pl
	-$(PERLBIN) quickreference.pl

smoke:: test
	-$(PERLBIN) smoketest.pl

install:: $(SWIG_LIB)
	$(LIBTOOL) --mode=install \
		$(INSTALL_DATA) $(SWIG_LIB) $(LANG_LIBDIR)
	$(INSTALL_DATA) pdflib_pl.pm $(LANG_LIBDIR)

uninstall::
	$(LIBTOOL) --mode=uninstall \
		$(RM) $(LANG_LIBDIR)/$(SWIG_LIB) $(LANG_LIBDIR)/pdflib_pl.pm 

clean::
	$(RM) smoke_c_?.pdf
	$(RM) hello.pdf image.pdf pdfclock.pdf chartab.pdf invoice.pdf
	$(RM) businesscard.pdf quickreference.pdf

# Automatically generated dependencies
