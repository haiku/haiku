# Makefile for `wget' utility
# Copyright (C) 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003,
# 2004, 2005, 2006, 2007, 2008, 2009 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with Wget.  If not, see <http://www.gnu.org/licenses/>.

# Additional permission under GNU GPL version 3 section 7

# If you modify this program, or any covered work, by linking or
# combining it with the OpenSSL project's OpenSSL library (or a
# modified version of that library), containing parts covered by the
# terms of the OpenSSL or SSLeay licenses, the Free Software Foundation
# grants you additional permission to convey the resulting work.
# Corresponding Source for a non-source form of such a combination
# shall include the source code for the parts of OpenSSL used as well
# as that of the covered work.

#
# Version: @VERSION@
#

PERL     = perl
PERLRUN = $(PERL) -I$(srcdir)

.PHONY: test run-unit-tests run-px-tests

check-local: test

test: ../src/wget$(EXEEXT) run-unit-tests run-px-tests

../src/wget$(EXEEXT):
	cd ../src && $(MAKE) $(AM_MAKEFLAGS)

# Make libunittest "PHONY" so we're always sure we're up-to-date.
.PHONY: ../src/libunittest.a
../src/libunittest.a:
	cd ../src && $(MAKE) $(AM_MAKEFLAGS) libunittest.a

../lib/libgnu.a:
	cd ../lib && $(MAKE) $(AM_MAKEFLAGS)

../md5/libmd5.a:
	cd ../lib && $(MAKE) $(AM_MAKEFLAGS)

run-unit-tests: unit-tests$(EXEEXT) ../src/libunittest.a
	./unit-tests$(EXEEXT)

run-px-tests: WgetTest.pm ../src/wget$(EXEEXT)
	$(srcdir)/run-px $(top_srcdir)

EXTRA_DIST = FTPServer.pm FTPTest.pm HTTPServer.pm HTTPTest.pm \
             WgetFeature.pm WgetFeature.cfg \
             Test-auth-basic.px \
             Test-auth-no-challenge.px \
             Test-auth-no-challenge-url.px \
             Test-auth-with-content-disposition.px \
             Test-c-full.px \
             Test-c-partial.px \
             Test-c.px \
             Test-c-shorter.px \
             Test-cookies.px \
             Test-cookies-401.px \
             Test-E-k-K.px \
             Test-E-k.px \
             Test-ftp.px \
             Test-ftp-pasv-fail.px \
             Test-ftp-bad-list.px \
             Test-ftp-recursive.px \
             Test-ftp-iri.px \
             Test-ftp-iri-fallback.px \
             Test-ftp-iri-recursive.px \
             Test-ftp-iri-disabled.px \
             Test-HTTP-Content-Disposition-1.px \
             Test-HTTP-Content-Disposition-2.px \
             Test-HTTP-Content-Disposition.px \
             Test-idn-headers.px \
             Test-idn-meta.px \
             Test-idn-cmd.px \
             Test-idn-robots.px \
             Test-iri.px \
             Test-iri-percent.px \
             Test-iri-disabled.px \
             Test-iri-forced-remote.px \
             Test-iri-list.px \
             Test-k.px \
             Test-meta-robots.px \
             Test-N-current.px \
             Test-N-HTTP-Content-Disposition.px \
             Test-N--no-content-disposition.px \
             Test-N--no-content-disposition-trivial.px \
             Test-N-no-info.px \
             Test--no-content-disposition.px \
             Test--no-content-disposition-trivial.px \
             Test-N-old.px \
             Test-nonexisting-quiet.px \
             Test-noop.px \
             Test-np.px \
             Test-N.px \
             Test-N-smaller.px \
             Test-O-HTTP-Content-Disposition.px \
             Test-O-nc.px \
             Test-O--no-content-disposition.px \
             Test-O--no-content-disposition-trivial.px \
             Test-O-nonexisting.px \
             Test-O.px \
             Test-proxied-https-auth.px \
             Test-proxy-auth-basic.px \
             Test-restrict-ascii.px \
             Test-Restrict-Lowercase.px \
             Test-Restrict-Uppercase.px \
             Test--spider-fail.px \
             Test--spider.px \
             Test--spider-r-HTTP-Content-Disposition.px \
             Test--spider-r--no-content-disposition.px \
             Test--spider-r--no-content-disposition-trivial.px \
             Test--spider-r.px \
             run-px certs

check_PROGRAMS = unit-tests
unit_tests_SOURCES =
LDADD = ../src/libunittest.a ../lib/libgnu.a @LIBSSL@ @LIBGNUTLS@ \
    @MD5_LDADD@

CLEANFILES = *~ *.bak core core.[0-9]*
