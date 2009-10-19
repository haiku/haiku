# Makefile for PDFlib's Tcl binding
# $Id: Makefile 14574 2005-10-29 16:27:43Z bonefish $

top_builddir	= ../../..

include $(top_builddir)/config/mkcommon.inc

INCLUDES	= $(PDFLIBINC) $(TCLINCLUDE)

LANG_LIBDIR 	= $(TCLPACKAGEDIR)

LIBNAME		= pdflib_tcl$(LA)
OBJ		= pdflib_tcl$(LO)
SRC		= pdflib_tcl.c


include $(top_builddir)/config/mkbind.inc

test:: all
	@cp pkgIndex.tcl .libs
	-$(LIBTOOL_EXE) $(TCLBIN) hello.tcl
	-$(LIBTOOL_EXE) $(TCLBIN) image.tcl
	-$(LIBTOOL_EXE) $(TCLBIN) pdfclock.tcl
	-$(LIBTOOL_EXE) $(TCLBIN) chartab.tcl
	-$(LIBTOOL_EXE) $(TCLBIN) invoice.tcl
	-$(LIBTOOL_EXE) $(TCLBIN) businesscard.tcl
	-$(LIBTOOL_EXE) $(TCLBIN) quickreference.tcl

smoke:: test

install:: $(LIBNAME)
	if test ! -d $(LANG_LIBDIR) ; then 	\
	    mkdir -p $(LANG_LIBDIR);		\
	    chmod 755 $(LANG_LIBDIR);		\
	fi
	$(INSTALL_DATA) pkgIndex.tcl $(LANG_LIBDIR)
	@-if test -f .libs/libpdflib_tcl.dylib; then			\
	    $(INSTALL_DATA) .libs/pdflib_tcl.dylib $(LANG_LIBDIR);	\
	else								\
	    $(LIBTOOL) --mode=install $(INSTALL_DATA)			\
		$(LIBNAME) $(LANG_LIBDIR);				\
	fi

uninstall::
	@-if test -f .libs/libpdflib_tcl.dylib; then			\
	    $(LIBTOOL) --mode=uninstall $(RM)				\
		$(LANG_LIBDIR)/pdflib_tcl.dylib				\
		$(LANG_LIBDIR)/pkgIndex.tcl;				\
	else								\
	    $(LIBTOOL) --mode=uninstall $(RM)				\
		$(LANG_LIBDIR)/$(LIBNAME)				\
		$(LANG_LIBDIR)/pkgIndex.tcl;				\
	fi
	$(RM) $(LANG_LIBDIR)

clean::
	$(RM) hello.pdf image.pdf pdfclock.pdf chartab.pdf invoice.pdf
	$(RM) businesscard.pdf quickreference.pdf


# Automatically generated dependencies
