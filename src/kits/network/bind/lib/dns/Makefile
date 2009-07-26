# Copyright (C) 2004-2008  Internet Systems Consortium, Inc. ("ISC")
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

# $Id: Makefile.in,v 1.163 2008/09/24 02:46:22 marka Exp $

srcdir =	.

top_srcdir =	../..

# Attempt to disable parallel processing.
.NOTPARALLEL:
.NO_PARALLEL:

VERSION=9.6.1

LIBINTERFACE = 52
LIBREVISION = 0
LIBAGE = 2

# Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
# Copyright (C) 1999-2001  Internet Software Consortium.
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

# $Id: includes.in,v 1.21 2007/06/19 23:47:24 tbox Exp $

# Search for machine-generated header files in the build tree,
# and for normal headers in the source tree (${top_srcdir}).
# We only need to look in OS-specific subdirectories for the
# latter case, because there are no machine-generated OS-specific
# headers.

ISC_INCLUDES = -I/boot/home/develop/haiku/vendor/bind/9.6.1/lib/isc/include \
	-I${top_srcdir}/lib/isc \
	-I${top_srcdir}/lib/isc/include \
	-I${top_srcdir}/lib/isc/unix/include \
	-I${top_srcdir}/lib/isc/nothreads/include \
	-I${top_srcdir}/lib/isc/x86_32/include

ISCCC_INCLUDES = -I/boot/home/develop/haiku/vendor/bind/9.6.1/lib/isccc/include \
       -I${top_srcdir}/lib/isccc/include

ISCCFG_INCLUDES = -I/boot/home/develop/haiku/vendor/bind/9.6.1/lib/isccfg/include \
       -I${top_srcdir}/lib/isccfg/include

DNS_INCLUDES = -I/boot/home/develop/haiku/vendor/bind/9.6.1/lib/dns/include \
	-I${top_srcdir}/lib/dns/include

LWRES_INCLUDES = -I/boot/home/develop/haiku/vendor/bind/9.6.1/lib/lwres/include \
	-I${top_srcdir}/lib/lwres/unix/include \
	-I${top_srcdir}/lib/lwres/include

BIND9_INCLUDES = -I/boot/home/develop/haiku/vendor/bind/9.6.1/lib/bind9/include \
	-I${top_srcdir}/lib/bind9/include

TEST_INCLUDES = \
	-I${top_srcdir}/lib/tests/include

USE_ISC_SPNEGO = 

CINCLUDES =	-I. -Iinclude ${DNS_INCLUDES} \
		${ISC_INCLUDES}  

CDEFINES =	-DUSE_MD5    \
		${USE_ISC_SPNEGO}

CWARNINGS =

ISCLIBS =	../../lib/isc/libisc.a

ISCDEPLIBS =	../../lib/isc/libisc.a

LIBS =		 -L/boot/common/lib -lxml2 -lz -liconv -lnetwork

# Alphabetically

DSTOBJS =	 \
		dst_api.o dst_lib.o dst_parse.o dst_result.o \
		gssapi_link.o gssapictx.o hmac_link.o key.o \
		openssl_link.o openssldh_link.o openssldsa_link.o \
		opensslrsa_link.o

# Alphabetically
DNSOBJS =	acache.o acl.o adb.o byaddr.o \
		cache.o callbacks.o compress.o \
		db.o dbiterator.o dbtable.o diff.o dispatch.o \
		dlz.o dnssec.o ds.o forward.o iptable.o journal.o \
		keytable.o lib.o log.o lookup.o \
		master.o masterdump.o message.o \
		name.o ncache.o nsec.o nsec3.o order.o peer.o portlist.o \
		rbt.o rbtdb.o rbtdb64.o rcode.o rdata.o \
		rdatalist.o \
		rdataset.o rdatasetiter.o rdataslab.o request.o \
		resolver.o result.o rootns.o sdb.o sdlz.o \
		soa.o ssu.o \
		stats.o tcpmsg.o time.o timer.o tkey.o \
		tsig.o ttl.o validator.o \
		version.o view.o xfrin.o zone.o zonekey.o zt.o

OBJS=		${DNSOBJS} ${OTHEROBJS} ${DSTOBJS}

# Alphabetically
DSTSRCS =	 \
		dst_api.c dst_lib.c dst_parse.c \
		dst_result.c gssapi_link.c gssapictx.c \
		hmac_link.c key.c \
		openssl_link.c openssldh_link.c \
		openssldsa_link.c opensslrsa_link.c

DNSSRCS =	acache.c acl.c adb.c byaddr.c \
		cache.c callbacks.c compress.c \
		db.c dbiterator.c dbtable.c diff.c dispatch.c \
		dlz.c dnssec.c ds.c forward.c iptable.c journal.c \
		keytable.c lib.c log.c lookup.c \
		master.c masterdump.c message.c \
		name.c ncache.c nsec.c nsec3.c order.c peer.c portlist.c \
		rbt.c rbtdb.c rbtdb64.c rcode.c rdata.c \
		rdatalist.c \
		rdataset.c rdatasetiter.c rdataslab.c request.c \
		resolver.c result.c rootns.c sdb.c sdlz.c \
		soa.c ssu.c \
		stats.c tcpmsg.c time.c timer.c tkey.c \
		tsig.c ttl.c validator.c \
		version.c view.c xfrin.c zone.c zonekey.c zt.c ${OTHERSRCS}
SRCS = ${DSTSRCS} ${DNSSRCS}

SUBDIRS =	include 
TARGETS =	include/dns/enumtype.h include/dns/enumclass.h \
		include/dns/rdatastruct.h timestamp

DEPENDEXTRA =	./gen -F include/dns/rdatastruct.h \
		-s ${srcdir} -d >> Makefile ;

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

libdns.a: ${OBJS}
	${AR} ${ARFLAGS} $@ ${OBJS}
	${RANLIB} $@

libdns.la: ${OBJS}
	${LIBTOOL_MODE_LINK} \
		${CC} ${ALL_CFLAGS} ${LDFLAGS} -o libdns.la -rpath ${libdir} \
		-version-info ${LIBINTERFACE}:${LIBREVISION}:${LIBAGE} \
		${OBJS} ${ISCLIBS}  ${LIBS}

timestamp: libdns.a
	touch timestamp

installdirs:
	$(SHELL) ${top_srcdir}/mkinstalldirs ${DESTDIR}${libdir}

install:: timestamp installdirs
	${LIBTOOL_MODE_INSTALL} ${INSTALL_DATA} libdns.a ${DESTDIR}${libdir}

clean distclean::
	rm -f libdns.a timestamp
	rm -f gen code.h include/dns/enumtype.h include/dns/enumclass.h
	rm -f include/dns/rdatastruct.h

newrr::
	rm -f code.h include/dns/enumtype.h include/dns/enumclass.h
	rm -f include/dns/rdatastruct.h

include: include/dns/enumtype.h include/dns/enumclass.h \
	include/dns/rdatastruct.h

rdata.o: code.h

include/dns/enumtype.h: gen
	./gen -s ${srcdir} -t > $@

include/dns/enumclass.h: gen
	./gen -s ${srcdir} -c > $@

include/dns/rdatastruct.h: gen \
		${srcdir}/rdata/rdatastructpre.h \
		${srcdir}/rdata/rdatastructsuf.h
	./gen -s ${srcdir} -i \
		-P ${srcdir}/rdata/rdatastructpre.h \
		-S ${srcdir}/rdata/rdatastructsuf.h > $@

code.h:	gen
	./gen -s ${srcdir} > code.h

gen: gen.c
	${BUILD_CC} ${BUILD_CFLAGS} -I${top_srcdir}/lib/isc/include \
	${BUILD_CPPFLAGS} ${BUILD_LDFLAGS} -o $@ ${srcdir}/gen.c ${BUILD_LIBS}

rbtdb64.o: rbtdb.c

depend: include/dns/enumtype.h include/dns/enumclass.h \
	include/dns/rdatastruct.h code.h
subdirs: include/dns/enumtype.h include/dns/enumclass.h \
	include/dns/rdatastruct.h code.h
${OBJS}: include/dns/enumtype.h include/dns/enumclass.h \
	include/dns/rdatastruct.h

spnego.o: spnego_asn1.c spnego.h
