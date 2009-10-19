# Makefile for PDFlib C samples, C language binding
# $Id: Makefile 14574 2005-10-29 16:27:43Z bonefish $

top_builddir	= ../../..

include $(top_builddir)/config/mkcommon.inc

DEPLIBS		= $(PDFLIBLINK)
LIBS		= $(DEPLIBS) $(EXTERNALLIBS)
INCLUDES	= $(PDFLIBINC) $(PDCORELIBINC)
FLAGS   	= $(LDFLAGS) $(CPPFLAGS) $(CFLAGS)

# the following is platform-specific
DLLIB		= -ldl

# ------------------------------

SRC	= \
	$(srcdir)/chartab.c		\
	$(srcdir)/hello.c		\
	$(srcdir)/hellodl.c		\
	$(srcdir)/image.c		\
	$(srcdir)/pdfclock.c		\
	$(srcdir)/invoice.c		\
	$(srcdir)/businesscard.c	\
	$(srcdir)/quickreference.c 	\
	$(srcdir)/smoketest.c		\
	$(srcdir)/smoketestdl.c

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

smoke:: test smoketest$(EXE)
	-./smoketest

clean::
	$(RM) chartab.pdf hello.pdf image.pdf pdfclock.pdf invoice.pdf
	$(RM) smoketest$(EXE) smoketestdl$(EXE) smoke_c_?.pdf smoke_cdl_?.pdf
	$(RM) hellodl hellodl.pdf businesscard.pdf quickreference.pdf

hello$(EXE): hello.c $(DEPLIBS)
	$(PROGS_BUILD)

image$(EXE): image.c $(DEPLIBS)
	$(PROGS_BUILD)

pdfclock$(EXE): pdfclock.c $(DEPLIBS)
	$(PROGS_BUILD)

chartab$(EXE): chartab.c $(DEPLIBS)
	$(PROGS_BUILD)

invoice$(EXE): invoice.c $(DEPLIBS)
	$(PROGS_BUILD)

businesscard$(EXE): businesscard.c $(DEPLIBS)
	$(PROGS_BUILD)

quickreference$(EXE): quickreference.c $(DEPLIBS)
	$(PROGS_BUILD)

smoketest$(EXE): smoketest.c $(DEPLIBS)
	$(PROGS_BUILD)

smoketestdl$(EXE): smoketestdl.c pdflibdl.c $(DEPLIBS) $(DLLIB) pdflibdl.h
	$(PROGS_BUILD) pdflibdl.c $(DLLIB)

hellodl$(EXE): hellodl.c pdflibdl.c $(DEPLIBS) $(DLLIB) pdflibdl.h
	$(PROGS_BUILD) pdflibdl.c $(DLLIB)
