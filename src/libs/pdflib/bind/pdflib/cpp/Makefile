# Makefile for PDFlib, C++ language binding
# $Id: Makefile 14574 2005-10-29 16:27:43Z bonefish $

top_builddir	= ../../..

include $(top_builddir)/config/mkcommon.inc

DEPLIBS		= $(PDFLIBLINK) pdflib$(LO)
LIBS		= $(DEPLIBS) $(STDCPP) $(EXTERNALLIBS)
INCLUDES	= $(PDFLIBINC)
CXXFLAGS	= $(CFLAGS)
FLAGS   	= $(LDFLAGS) $(CPPFLAGS) $(CFLAGS)

# --------------------------------------------------------------------------

SRC	= \
	$(srcdir)/pdflib.cpp		\
	$(srcdir)/hello.cpp		\
	$(srcdir)/image.cpp		\
	$(srcdir)/pdfclock.cpp		\
	$(srcdir)/chartab.cpp		\
	$(srcdir)/invoice.cpp		\
	$(srcdir)/businesscard.cpp	\
	$(srcdir)/quickreference.cpp	\
	$(srcdir)/smoketest.cpp

PROGS = \
	hello$(EXE)		\
	image$(EXE)		\
	pdfclock$(EXE)		\
	chartab$(EXE)		\
	invoice$(EXE)		\
	businesscard$(EXE)	\
	quickreference$(EXE)

include $(top_builddir)/config/mkprogs.inc


test:: $(PROGS)
	-./hello
	-./image
	-./pdfclock
	-./chartab
	-./invoice
	-./businesscard
	-./quickreference

smoke::	test smoketest$(EXE)
	-./smoketest

clean::
	$(RM) smoketest$(EXE) smoke_cpp_?.pdf
	$(RM) hello.pdf image.pdf pdfclock.pdf chartab.pdf invoice.pdf
	$(RM) businesscard.pdf quickreference.pdf

hello$(EXE): hello.cpp $(DEPLIBS)
	$(CXXPROGS_BUILD)

image$(EXE): image.cpp $(DEPLIBS)
	$(CXXPROGS_BUILD)

pdfclock$(EXE): pdfclock.cpp $(DEPLIBS)
	$(CXXPROGS_BUILD)

chartab$(EXE): chartab.cpp $(DEPLIBS)
	$(CXXPROGS_BUILD)

invoice$(EXE): invoice.cpp $(DEPLIBS)
	$(CXXPROGS_BUILD)

businesscard$(EXE): businesscard.cpp $(DEPLIBS)
	$(CXXPROGS_BUILD)

quickreference$(EXE): quickreference.cpp $(DEPLIBS)
	$(CXXPROGS_BUILD)

smoketest$(EXE): smoketest.cpp $(DEPLIBS)
	$(CXXPROGS_BUILD)

pdflib$(LO): ./pdflib.cpp pdflib.hpp ../../../libs/pdflib/pdflib.h
	$(LIBTOOL_CC) $(CXX) -c $(CXXFLAGS) pdflib.cpp
