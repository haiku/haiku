# Makefile for PDFlib's Java binding
# $Id: Makefile 14574 2005-10-29 16:27:43Z bonefish $

top_builddir	= ../../..

include $(top_builddir)/config/mkcommon.inc
PDFPEDANTIC	=

LIBS		= $(PDFLIBLINK)
INCLUDES	= $(PDFLIBINC) $(JAVAINCLUDE)

LANG_LIBDIR 	= $(libdir)

LIBNAME		= libpdf_java$(LA)
OBJ		= pdflib_java$(LO)
SRC		= pdflib_java.c

TESTPROGS	= \
	hello.class \
	image.class \
	pdfclock.class \
	chartab.class \
	invoice.class \
	businesscard.class \
	quickreference.class

include $(top_builddir)/config/mkbind.inc

.SUFFIXES:	.java .class

.java.class:
	-CLASSPATH=$$CLASSPATH:pdflib.jar:. javac $<


all::	pdflib.jar

pdflib.jar:: pdflib.java PDFlibException.java
	$(RM) com
	mkdir -p com/pdflib
	javac PDFlibException.java pdflib.java && mv pdflib.class com/pdflib\
	    && mv PDFlibException.class com/pdflib/PDFlibException.class
	jar cvf pdflib.jar com/pdflib/pdflib.class \
	    com/pdflib/PDFlibException.class
	$(RM) com

javadoc::
	$(RM) javadoc
	mkdir javadoc
	-javadoc -notree -author -version -d javadoc -public \
	    pdflib.java PDFlibException.java

test:: $(LIBNAME) pdflib.jar $(TESTPROGS)
	-CLASSPATH=$$CLASSPATH:pdflib.jar:. $(LIBTOOL_EXE) java hello
	-CLASSPATH=$$CLASSPATH:pdflib.jar:. $(LIBTOOL_EXE) java image
	-CLASSPATH=$$CLASSPATH:pdflib.jar:. $(LIBTOOL_EXE) java pdfclock
	-CLASSPATH=$$CLASSPATH:pdflib.jar:. $(LIBTOOL_EXE) java chartab
	-CLASSPATH=$$CLASSPATH:pdflib.jar:. $(LIBTOOL_EXE) java invoice
	-CLASSPATH=$$CLASSPATH:pdflib.jar:. $(LIBTOOL_EXE) java businesscard
	-CLASSPATH=$$CLASSPATH:pdflib.jar:. $(LIBTOOL_EXE) java quickreference

smoke:: test smoketest.class
	-CLASSPATH=$$CLASSPATH:pdflib.jar:. $(LIBTOOL_EXE) java smoketest

install:: $(LIBNAME) pdflib.jar
	@-if test -f .libs/libpdf_java.jnilib; then		\
	    $(LIBTOOL) --mode=install $(INSTALL_DATA)		\
		.libs/libpdf_java.jnilib $(LANG_LIBDIR);	\
	else							\
	    $(LIBTOOL) --mode=install $(INSTALL_DATA)		\
		$(LIBNAME) $(LANG_LIBDIR);			\
	fi
	@echo Copy pdflib.jar and javadoc to an appropriate directory,
	@echo and include pdflib.jar in your CLASSPATH.

uninstall::
	@-if test -f .libs/libpdf_java.jnilib; then		\
	    $(LIBTOOL) --mode=uninstall $(RM)			\
		$(LANG_LIBDIR)/libpdf_java.jnilib;		\
	else							\
	    $(LIBTOOL) --mode=uninstall $(RM)			\
		$(LANG_LIBDIR)/$(LIBNAME);			\
	fi

clean::
	$(RM) *.class com pdflib.jar javadoc smoke_c_?.pdf
	$(RM) hello.pdf image.pdf pdfclock.pdf chartab.pdf invoice.pdf
	$(RM) businesscard.pdf quickreference.pdf

# Automatically generated dependencies
