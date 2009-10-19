# Makefile for PDFlib
# $Id: Makefile 14574 2005-10-29 16:27:43Z bonefish $

top_builddir	= ../..

include ../../config/mkcommon.inc

INCLUDES	= $(PDFLIBINC) $(PDCORELIBINC) $(TIFFLIBINC) $(FLATELIBINC)\
		  $(PNGLIBINC) $(PDILIBINC)

LIBNAME		= $(PDFLIB_LINK)

SRC	= \
	$(srcdir)/p_afm.c	\
	$(srcdir)/p_annots.c	\
        $(srcdir)/p_block.c     \
	$(srcdir)/p_bmp.c	\
	$(srcdir)/p_ccitt.c	\
	$(srcdir)/p_cid.c	\
	$(srcdir)/p_color.c	\
	$(srcdir)/p_draw.c	\
	$(srcdir)/p_encoding.c	\
	$(srcdir)/p_filter.c	\
	$(srcdir)/p_font.c	\
	$(srcdir)/p_gif.c	\
	$(srcdir)/p_gstate.c	\
	$(srcdir)/p_hostfont.c	\
	$(srcdir)/p_hyper.c	\
	$(srcdir)/p_icc.c	\
	$(srcdir)/p_icclib.c	\
	$(srcdir)/p_image.c	\
	$(srcdir)/p_jpeg.c	\
	$(srcdir)/p_kerning.c	\
	$(srcdir)/p_params.c	\
	$(srcdir)/p_pattern.c	\
	$(srcdir)/p_pdi.c	\
	$(srcdir)/p_pfm.c	\
	$(srcdir)/p_png.c	\
	$(srcdir)/p_resource.c	\
	$(srcdir)/p_shading.c	\
	$(srcdir)/p_subsett.c	\
	$(srcdir)/p_template.c	\
	$(srcdir)/p_text.c	\
	$(srcdir)/p_tiff.c	\
	$(srcdir)/p_truetype.c	\
	$(srcdir)/p_type1.c	\
	$(srcdir)/p_type3.c	\
	$(srcdir)/p_xgstate.c

OBJS	= \
	$(srcdir)/p_afm$(LO)	 \
	$(srcdir)/p_annots$(LO)	 \
	$(srcdir)/p_block$(LO)   \
	$(srcdir)/p_bmp$(LO)	 \
	$(srcdir)/p_ccitt$(LO)	 \
	$(srcdir)/p_cid$(LO)	 \
	$(srcdir)/p_color$(LO)	 \
	$(srcdir)/p_draw$(LO)	 \
	$(srcdir)/p_encoding$(LO)\
	$(srcdir)/p_filter$(LO)	 \
	$(srcdir)/p_font$(LO)	 \
	$(srcdir)/p_gif$(LO)	 \
	$(srcdir)/p_gstate$(LO)	 \
	$(srcdir)/p_hostfont$(LO)\
	$(srcdir)/p_hyper$(LO)	 \
	$(srcdir)/p_icc$(LO)	 \
	$(srcdir)/p_icclib$(LO)	 \
	$(srcdir)/p_image$(LO)	 \
	$(srcdir)/p_jpeg$(LO)	 \
	$(srcdir)/p_kerning$(LO) \
	$(srcdir)/p_params$(LO)	 \
	$(srcdir)/p_pattern$(LO) \
	$(srcdir)/p_pdi$(LO)	 \
	$(srcdir)/p_pfm$(LO)	 \
	$(srcdir)/p_png$(LO)	 \
	$(srcdir)/p_resource$(LO)\
	$(srcdir)/p_shading$(LO) \
	$(srcdir)/p_subsett$(LO) \
	$(srcdir)/p_template$(LO)\
	$(srcdir)/p_text$(LO)	 \
	$(srcdir)/p_tiff$(LO)	 \
	$(srcdir)/p_truetype$(LO)\
	$(srcdir)/p_type1$(LO)	 \
	$(srcdir)/p_type3$(LO)	 \
	$(srcdir)/p_xgstate$(LO)

include ../../config/mklibs.inc

MAINLIBNAME	= lib$(PDFLIBNAME)$(LA)

MAINSRC		= $(srcdir)/p_basic.c
MAINOBJ		= $(srcdir)/p_basic$(LO)
MAININC		= pdflib.h

INTERNALLIBS	= $(PDCORELIBLINK) $(PNGLIBLINK) $(FLATELIBLINK) $(TIFFLIBLINK)\
		  $(PDILIBLINK) $(JPEGLIBLINK) $(PDFLIB_LINK)

include ../../config/mkmainlib.inc

# Automatically generated dependencies
