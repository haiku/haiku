# Makefile for PDFlib clients
# $Id: Makefile,v 1.1 2004/10/06 17:46:52 laplace Exp $

top_builddir	= ../..

include $(top_builddir)/config/mkcommon.inc

DEPLIBS		= $(PDFLIBLINK)
LIBS		= $(DEPLIBS) $(EXTERNALLIBS)
INCLUDES	= $(PDFLIBINC)
CFLAGS          = $(DEFS) $(DEFINES) $(INCLUDES)
FLAGS  		= $(LDFLAGS) $(CPPFLAGS) $(CFLAGS)
EXTRA_SOURCES	= getopt.c

# ------------------------------

SRC	= \
	$(srcdir)/pdfimpose.c	\
	$(srcdir)/pdfimage.c	\
	$(srcdir)/text2pdf.c

PROGS	= \
	pdfimpose$(EXE)	\
	pdfimage$(EXE)	\
	text2pdf$(EXE)

include $(top_builddir)/config/mkprogs.inc

pdfimpose$(EXE): pdfimpose.c $(DEPLIBS) $(EXTRA_SOURCES)
	$(PROGS_BUILD)

pdfimage$(EXE): pdfimage.c $(DEPLIBS) $(EXTRA_SOURCES)
	$(PROGS_BUILD)

text2pdf$(EXE): text2pdf.c $(DEPLIBS) $(EXTRA_SOURCES)
	$(PROGS_BUILD)

