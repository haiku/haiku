# Makefile for libpdcore
# This generates a libtool convenience library
# $Id: Makefile 14574 2005-10-29 16:27:43Z bonefish $

top_builddir	= ../..

include ../../config/mkcommon.inc

LIBNAME 	= $(PDCORELIBLINK)
INCLUDES	= $(PDCORELIBINC) $(PDFLIBINC) $(PDILIBINC) $(FLATELIBINC)

SRC	= \
	$(srcdir)/pc_core.c	\
	$(srcdir)/pc_corefont.c	\
	$(srcdir)/pc_crypt.c	\
	$(srcdir)/pc_ebcdic.c	\
	$(srcdir)/pc_encoding.c	\
	$(srcdir)/pc_file.c	\
	$(srcdir)/pc_font.c	\
	$(srcdir)/pc_geom.c	\
	$(srcdir)/pc_md5.c	\
	$(srcdir)/pc_optparse.c	\
	$(srcdir)/pc_output.c	\
	$(srcdir)/pc_arc4.c	\
	$(srcdir)/pc_sbuf.c	\
	$(srcdir)/pc_scope.c	\
	$(srcdir)/pc_unicode.c	\
	$(srcdir)/pc_util.c

OBJS	= \
	$(srcdir)/pc_core$(LO)	\
	$(srcdir)/pc_corefont$(LO)\
	$(srcdir)/pc_crypt$(LO)	\
	$(srcdir)/pc_ebcdic$(LO)	\
	$(srcdir)/pc_encoding$(LO)\
	$(srcdir)/pc_file$(LO)	\
	$(srcdir)/pc_font$(LO)	\
	$(srcdir)/pc_geom$(LO)	\
	$(srcdir)/pc_md5$(LO)	\
	$(srcdir)/pc_optparse$(LO)\
	$(srcdir)/pc_output$(LO)	\
	$(srcdir)/pc_arc4$(LO)	\
	$(srcdir)/pc_sbuf$(LO)	\
	$(srcdir)/pc_scope$(LO)	\
	$(srcdir)/pc_unicode$(LO)	\
	$(srcdir)/pc_util$(LO)


include ../../config/mklibs.inc

# Automatically generated dependencies
