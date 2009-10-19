# Makefile for libpng 1.2.5
# This generates a libtool convenience library
# $Id: Makefile 14574 2005-10-29 16:27:43Z bonefish $

top_builddir	= ../..

include ../../config/mkcommon.inc

LIBNAME		= $(PNGLIBLINK)
INCLUDES	= $(ZLIBINC) $(PDCORELIBINC)

SRC	= \
	$(srcdir)/png.c	\
	$(srcdir)/pngset.c	\
	$(srcdir)/pngget.c	\
	$(srcdir)/pngrutil.c	\
	$(srcdir)/pngtrans.c	\
	$(srcdir)/pngread.c	\
	$(srcdir)/pngrio.c	\
	$(srcdir)/pngrtran.c	\
	$(srcdir)/pngmem.c	\
	$(srcdir)/pngerror.c

OBJS	= \
	$(srcdir)/png$(LO)	\
	$(srcdir)/pngset$(LO)	\
	$(srcdir)/pngget$(LO)	\
	$(srcdir)/pngrutil$(LO)	\
	$(srcdir)/pngtrans$(LO)	\
	$(srcdir)/pngread$(LO)	\
	$(srcdir)/pngrio$(LO)	\
	$(srcdir)/pngrtran$(LO)	\
	$(srcdir)/pngmem$(LO)	\
	$(srcdir)/pngerror$(LO)

include ../../config/mklibs.inc

# Automatically generated dependencies
