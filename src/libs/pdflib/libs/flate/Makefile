# Makefile for zlib
# This generates a libtool convenience library
# $Id: Makefile,v 1.1 2004/10/06 17:46:48 laplace Exp $

top_builddir	= ../..

INCLUDES	= $(PDCORELIBINC)

LIBNAME = $(FLATELIBLINK)

include ../../config/mkcommon.inc

SRC	= \
	$(srcdir)/adler32.c 	\
	$(srcdir)/compress.c	\
	$(srcdir)/crc32.c	\
	$(srcdir)/deflate.c	\
	$(srcdir)/infblock.c	\
	$(srcdir)/infcodes.c	\
	$(srcdir)/inffast.c	\
	$(srcdir)/inflate.c	\
	$(srcdir)/inftrees.c 	\
	$(srcdir)/infutil.c	\
	$(srcdir)/trees.c	\
	$(srcdir)/uncompr.c	\
	$(srcdir)/zutil.c

OBJS	= \
	$(srcdir)/adler32$(LO) 	\
	$(srcdir)/compress$(LO)	\
	$(srcdir)/crc32$(LO)	\
	$(srcdir)/deflate$(LO)	\
	$(srcdir)/infblock$(LO)	\
	$(srcdir)/infcodes$(LO)	\
	$(srcdir)/inffast$(LO)	\
	$(srcdir)/inflate$(LO)	\
	$(srcdir)/inftrees$(LO) 	\
	$(srcdir)/infutil$(LO)	\
	$(srcdir)/trees$(LO)	\
	$(srcdir)/uncompr$(LO)	\
	$(srcdir)/zutil$(LO)

include ../../config/mklibs.inc

# Automatically generated dependencies
