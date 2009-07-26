# Copyright (C) 2004-2009  Internet Systems Consortium, Inc. ("ISC")
# Copyright (C) 1998-2003  Internet Software Consortium.
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
# OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.

# $Id: Makefile.in,v 1.96.50.3 2009/02/16 01:02:58 marka Exp $

srcdir =	.

top_srcdir =	../..

VERSION=9.6.1

LIBINTERFACE = 51
LIBREVISION = 1
LIBAGE = 1

CINCLUDES =	-I${srcdir}/unix/include \
		-I${srcdir}/nothreads/include \
		-I${srcdir}/x86_32/include \
		-I./include \
		-I${srcdir}/include
CDEFINES =
CWARNINGS =

# Alphabetically
UNIXOBJS =	 \
		unix/app.o unix/dir.o unix/entropy.o \
		unix/errno2result.o unix/file.o unix/fsaccess.o \
		unix/interfaceiter.o unix/keyboard.o unix/net.o \
		unix/os.o unix/resource.o unix/socket.o unix/stdio.o \
		unix/stdtime.o unix/strerror.o unix/syslog.o unix/time.o


NLSOBJS =	nls/msgcat.o

THREADOBJS =	nothreads/condition.o nothreads/mutex.o \
		nothreads/thread.o

WIN32OBJS = 	win32/condition.o win32/dir.o win32/file.o \
		win32/fsaccess.o win32/once.o win32/stdtime.o \
		win32/thread.o win32/time.o

# Alphabetically
OBJS =		  \
		assertions.o base32.o base64.o bitstring.o buffer.o \
		bufferlist.o commandline.o error.o event.o \
		hash.o heap.o hex.o hmacmd5.o hmacsha.o \
		httpd.o inet_aton.o iterated_hash.o \
		lex.o lfsr.o lib.o log.o \
		md5.o mem.o mutexblock.o \
		netaddr.o netscope.o ondestroy.o \
		parseint.o portset.o quota.o radix.o random.o \
		ratelimiter.o refcount.o region.o result.o rwlock.o \
		serial.o sha1.o sha2.o sockaddr.o stats.o \
		string.o strtoul.o symtab.o task.o taskpool.o \
		timer.o version.o ${UNIXOBJS} ${NLSOBJS} ${THREADOBJS}

# Alphabetically
SRCS =		  \
		assertions.c base32.c base64.c bitstring.c buffer.c \
		bufferlist.c commandline.c error.c event.c \
		heap.c hex.c hmacmd5.c hmacsha.c \
		httpd.c inet_aton.c iterated_hash.c \
		lex.c lfsr.c lib.c log.c \
		md5.c mem.c mutexblock.c \
		netaddr.c netscope.c ondestroy.c \
		parseint.c portset.c quota.c radix.c random.c \
		ratelimiter.c refcount.c region.c result.c rwlock.c \
		serial.c sha1.c sha2.c sockaddr.c stats.c string.c strtoul.c \
		symtab.c task.c taskpool.c timer.c version.c

LIBS =		 -L/boot/common/lib -lxml2 -lz -liconv -lnetwork

SUBDIRS =	include unix nls nothreads x86_32
TARGETS =	timestamp

# Copyright (C) 2004-2009  Internet Systems Consortium, Inc. ("ISC")
# Copyright (C) 1998-2003  Internet Software Consortium.
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
# OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.

# $Id: rules.in,v 1.64.130.2 2009/01/10 23:46:57 tbox Exp $

###
### Common Makefile rules for BIND 9.
###

###
### Paths
###
### Note: paths that vary by Makefile MUST NOT be listed
### here, or they won't get expanded correctly.

prefix =	/usr/local
exec_prefix =	${prefix}
bindir =	${exec_prefix}/bin
sbindir =	${exec_prefix}/sbin
includedir =	${prefix}/include
libdir =	${exec_prefix}/lib
sysconfdir =	/etc
localstatedir =	/var
mandir =	${datarootdir}/man
datarootdir =   ${prefix}/share

DESTDIR =



top_builddir =	/boot/home/develop/haiku/vendor/bind/9.6.1

###
### All
###
### Makefile may define:
###	TARGETS

all: subdirs ${TARGETS}

###
### Subdirectories
###
### Makefile may define:
###	SUBDIRS

ALL_SUBDIRS = ${SUBDIRS} nulldir

#
# We use a single-colon rule so that additional dependencies of
# subdirectories can be specified after the inclusion of this file.
# The "depend" target is treated the same way.
#
subdirs:
	@for i in ${ALL_SUBDIRS}; do \
		if [ "$$i" != "nulldir" -a -d $$i ]; then \
			echo "making all in `pwd`/$$i"; \
			(cd $$i; ${MAKE} ${MAKEDEFS} DESTDIR="${DESTDIR}" all) || exit 1; \
		fi; \
	done

install:: all

install clean distclean maintainer-clean doc docclean man manclean::
	@for i in ${ALL_SUBDIRS}; do \
		if [ "$$i" != "nulldir" -a -d $$i ]; then \
			echo "making $@ in `pwd`/$$i"; \
			(cd $$i; ${MAKE} ${MAKEDEFS} DESTDIR="${DESTDIR}" $@) || exit 1; \
		fi; \
	done

###
### C Programs
###
### Makefile must define
###	CC
### Makefile may define
###	CFLAGS
###	LDFLAGS
###	CINCLUDES
###	CDEFINES
###	CWARNINGS
### User may define externally
###     EXT_CFLAGS

CC = 		gcc
CFLAGS =	-g -O2 -I/boot/common/include/libxml2
LDFLAGS =	
STD_CINCLUDES =	
STD_CDEFINES =	
STD_CWARNINGS =	 -W -Wall -Wmissing-prototypes -Wcast-qual -Wwrite-strings -Wformat -Wpointer-arith -fno-strict-aliasing

BUILD_CC = gcc
BUILD_CFLAGS = -g -O2 -I/boot/common/include/libxml2
BUILD_CPPFLAGS =  
BUILD_LDFLAGS = 
BUILD_LIBS =  -L/boot/common/lib -lxml2 -lz -liconv -lnetwork

.SUFFIXES:
.SUFFIXES: .c .o

ALWAYS_INCLUDES = -I${top_builddir}
ALWAYS_DEFINES = 
ALWAYS_WARNINGS =

ALL_CPPFLAGS = \
	${ALWAYS_INCLUDES} ${CINCLUDES} ${STD_CINCLUDES} \
	${ALWAYS_DEFINES} ${CDEFINES} ${STD_CDEFINES}

ALL_CFLAGS = ${EXT_CFLAGS} ${ALL_CPPFLAGS} ${CFLAGS} \
	${ALWAYS_WARNINGS} ${STD_CWARNINGS} ${CWARNINGS}

.c.o:
	${LIBTOOL_MODE_COMPILE} ${CC} ${ALL_CFLAGS} -c $<

SHELL = /bin/sh
LIBTOOL = 
LIBTOOL_MODE_COMPILE = ${LIBTOOL} 
LIBTOOL_MODE_INSTALL = ${LIBTOOL} 
LIBTOOL_MODE_LINK = ${LIBTOOL} 
PURIFY = 

MKDEP = ${SHELL} ${top_builddir}/make/mkdep

cleandir: distclean
superclean: maintainer-clean

clean distclean maintainer-clean::
	rm -f *.o *.o *.lo *.la core *.core .depend
	rm -rf .libs

distclean maintainer-clean::
	rm -f Makefile

depend:
	@for i in ${ALL_SUBDIRS}; do \
		if [ "$$i" != "nulldir" -a -d $$i ]; then \
			echo "making depend in `pwd`/$$i"; \
			(cd $$i; ${MAKE} ${MAKEDEFS} DESTDIR="${DESTDIR}" $@) || exit 1; \
		fi; \
	done
	@if [ X"${VPATH}" != X ] ; then \
		if [ X"${SRCS}" != X -a X"${PSRCS}" != X ] ; then \
			echo ${MKDEP} -vpath ${VPATH} ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${SRCS}; \
			${MKDEP} -vpath ${VPATH} ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${SRCS}; \
			echo ${MKDEP} -vpath ${VPATH} -ap ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${PSRCS}; \
			${MKDEP} -vpath ${VPATH} -ap ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${PSRCS}; \
			${DEPENDEXTRA} \
		elif [ X"${SRCS}" != X ] ; then \
			echo ${MKDEP} -vpath ${VPATH} ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${SRCS}; \
			${MKDEP} -vpath ${VPATH} ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${SRCS}; \
			${DEPENDEXTRA} \
		elif [ X"${PSRCS}" != X ] ; then \
			echo ${MKDEP} -vpath ${VPATH} ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${PSRCS}; \
			${MKDEP} -vpath ${VPATH} -p ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${PSRCS}; \
			${DEPENDEXTRA} \
		fi \
	else \
		if [ X"${SRCS}" != X -a X"${PSRCS}" != X ] ; then \
			echo ${MKDEP} ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${SRCS}; \
			${MKDEP} ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${SRCS}; \
			echo ${MKDEP} -ap ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${PSRCS}; \
			${MKDEP} -ap ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${PSRCS}; \
			${DEPENDEXTRA} \
		elif [ X"${SRCS}" != X ] ; then \
			echo ${MKDEP} ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${SRCS}; \
			${MKDEP} ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${SRCS}; \
			${DEPENDEXTRA} \
		elif [ X"${PSRCS}" != X ] ; then \
			echo ${MKDEP} ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${PSRCS}; \
			${MKDEP} -p ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${PSRCS}; \
			${DEPENDEXTRA} \
		fi \
	fi

FORCE:

###
### Libraries
###

AR =		/boot/develop/tools/gnupro/bin/ar
ARFLAGS =	cruv
RANLIB =	ranlib

###
### Installation
###

INSTALL =		/bin/install -c
INSTALL_PROGRAM =	${INSTALL}
LINK_PROGRAM =		ln -s
INSTALL_SCRIPT =	${INSTALL}
INSTALL_DATA =		${INSTALL} -m 644

###
### Programs used when generating documentation.  It's ok for these
### not to exist when not generating documentation.
###

XSLTPROC =		xsltproc --novalid --xinclude --nonet
PERL =			/boot/common/bin/perl
LATEX =			latex
PDFLATEX =		pdflatex
W3M =			w3m

###
### DocBook -> HTML
### DocBook -> man page
###

.SUFFIXES: .docbook .html .1 .2 .3 .4 .5 .6 .7 .8

.docbook.html:
	${XSLTPROC} -o $@ ${top_srcdir}/doc/xsl/isc-docbook-html.xsl $<

.docbook.1:
	${XSLTPROC} -o $@ ${top_srcdir}/doc/xsl/isc-manpage.xsl $<

.docbook.2:
	${XSLTPROC} -o $@ ${top_srcdir}/doc/xsl/isc-manpage.xsl $<

.docbook.3:
	${XSLTPROC} -o $@ ${top_srcdir}/doc/xsl/isc-manpage.xsl $<

.docbook.4:
	${XSLTPROC} -o $@ ${top_srcdir}/doc/xsl/isc-manpage.xsl $<

.docbook.5:
	${XSLTPROC} -o $@ ${top_srcdir}/doc/xsl/isc-manpage.xsl $<

.docbook.6:
	${XSLTPROC} -o $@ ${top_srcdir}/doc/xsl/isc-manpage.xsl $<

.docbook.7:
	${XSLTPROC} -o $@ ${top_srcdir}/doc/xsl/isc-manpage.xsl $<

.docbook.8:
	${XSLTPROC} -o $@ ${top_srcdir}/doc/xsl/isc-manpage.xsl $<

version.o: version.c
	${LIBTOOL_MODE_COMPILE} ${CC} ${ALL_CFLAGS} \
		-DVERSION=\"${VERSION}\" \
		-DLIBINTERFACE=${LIBINTERFACE} \
		-DLIBREVISION=${LIBREVISION} \
		-DLIBAGE=${LIBAGE} \
		-c ${srcdir}/version.c

libisc.a: ${OBJS}
	${AR} ${ARFLAGS} $@ ${OBJS}
	${RANLIB} $@

libisc.la: ${OBJS}
	${LIBTOOL_MODE_LINK} \
		${CC} ${ALL_CFLAGS} ${LDFLAGS} -o libisc.la -rpath ${libdir} \
		-version-info ${LIBINTERFACE}:${LIBREVISION}:${LIBAGE} \
		${OBJS} ${LIBS}

timestamp: libisc.a
	touch timestamp

installdirs:
	$(SHELL) ${top_srcdir}/mkinstalldirs ${DESTDIR}${libdir}

install:: timestamp installdirs
	${LIBTOOL_MODE_INSTALL} ${INSTALL_DATA} libisc.a ${DESTDIR}${libdir}

clean distclean::
	rm -f libisc.a libisc.la timestamp
