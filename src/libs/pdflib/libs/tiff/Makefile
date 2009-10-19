# Makefile for tifflib
# This generates a libtool convenience library
# $Id: Makefile 14574 2005-10-29 16:27:43Z bonefish $

top_builddir	= ../..

include ../../config/mkcommon.inc

LIBNAME		= $(TIFFLIBLINK)
INCLUDES	= $(PDCORELIBINC) $(FLATELIBINC)

SRC	= \
	$(srcdir)/tif_auxx.c		\
	$(srcdir)/tif_close.c		\
	$(srcdir)/tif_codec.c		\
	$(srcdir)/tif_compress.c	\
	$(srcdir)/tif_dir.c		\
	$(srcdir)/tif_dirinfo.c		\
	$(srcdir)/tif_dirread.c		\
	$(srcdir)/tif_dumpmode.c	\
	$(srcdir)/tif_error.c		\
	$(srcdir)/tif_fax3.c		\
	$(srcdir)/tif_fax3sm.c		\
	$(srcdir)/tif_getimage.c	\
	$(srcdir)/tif_luv.c		\
	$(srcdir)/tif_lzw.c		\
	$(srcdir)/tif_next.c		\
	$(srcdir)/tif_open.c		\
	$(srcdir)/tif_packbits.c	\
	$(srcdir)/tif_predict.c		\
	$(srcdir)/tif_read.c		\
	$(srcdir)/tif_strip.c		\
	$(srcdir)/tif_swab.c		\
	$(srcdir)/tif_tile.c		\
	$(srcdir)/tif_unix.c		\
	$(srcdir)/tif_version.c		\
	$(srcdir)/tif_warning.c		\
	$(srcdir)/tif_zip.c

# unused modules
#	$(srcdir)/tif_dirwrite.c	\
#	$(srcdir)/tif_flush.c		\
#	$(srcdir)/tif_jpeg.c		\
#	$(srcdir)/tif_ojeg.c		\
#	$(srcdir)/tif_pixarlog.c	\
#	$(srcdir)/tif_print.c		\
#	$(srcdir)/tif_thunder.c		\
#	$(srcdir)/tif_write.c


OBJS	= \
	$(srcdir)/tif_auxx$(LO)		\
	$(srcdir)/tif_close$(LO)		\
	$(srcdir)/tif_codec$(LO)		\
	$(srcdir)/tif_compress$(LO)	\
	$(srcdir)/tif_dir$(LO)		\
	$(srcdir)/tif_dirinfo$(LO)	\
	$(srcdir)/tif_dirread$(LO)	\
	$(srcdir)/tif_dumpmode$(LO)	\
	$(srcdir)/tif_error$(LO)		\
	$(srcdir)/tif_fax3$(LO)		\
	$(srcdir)/tif_fax3sm$(LO)		\
	$(srcdir)/tif_getimage$(LO)	\
	$(srcdir)/tif_luv$(LO)		\
	$(srcdir)/tif_lzw$(LO)		\
	$(srcdir)/tif_next$(LO)		\
	$(srcdir)/tif_open$(LO)		\
	$(srcdir)/tif_packbits$(LO)	\
	$(srcdir)/tif_predict$(LO)	\
	$(srcdir)/tif_read$(LO)		\
	$(srcdir)/tif_strip$(LO)		\
	$(srcdir)/tif_swab$(LO)		\
	$(srcdir)/tif_tile$(LO)		\
	$(srcdir)/tif_unix$(LO)		\
	$(srcdir)/tif_version$(LO)	\
	$(srcdir)/tif_warning$(LO)	\
	$(srcdir)/tif_zip$(LO)

# unused modules
#	$(srcdir)/tif_dirwrite$(LO)	\
#	$(srcdir)/tif_flush$(LO)		\
#	$(srcdir)/tif_jpeg.c		\
#	$(srcdir)/tif_ojeg.c		\
#	$(srcdir)/tif_print$(LO)		\
#	$(srcdir)/tif_pixarlog$(LO)	\
#	$(srcdir)/tif_thunder$(LO)	\
#	$(srcdir)/tif_write$(LO)

include ../../config/mklibs.inc

# Automatically generated dependencies
