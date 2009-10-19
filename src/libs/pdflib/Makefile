# Main PDFlib Makefile
# $Id: Makefile 14574 2005-10-29 16:27:43Z bonefish $

top_builddir=.

include config/mkcommon.inc

SUB_DIRS = libs bind progs $(PTFDIR)

include config/mksubdirs.inc

# --------------------------- install ---------------------------------
install:: .dummy
	$(INSTALL) pdflib-config $(bindir)

# ----------------------- distclean ---------------------------------
distclean:: .dummy
	-$(RM) libtool pdflib-config config.cache config.log config.status \
	config/mkprogs.inc config/mklibs.inc config/mkcommon.inc \
	config/mkbind.inc config/mksubdirs.inc
