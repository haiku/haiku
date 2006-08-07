# $Id: Makefile.in,v 1.24 2005/01/29 19:30:06 tom Exp $
##############################################################################
# Copyright (c) 1998-2004,2005 Free Software Foundation, Inc.                #
#                                                                            #
# Permission is hereby granted, free of charge, to any person obtaining a    #
# copy of this software and associated documentation files (the "Software"), #
# to deal in the Software without restriction, including without limitation  #
# the rights to use, copy, modify, merge, publish, distribute, distribute    #
# with modifications, sublicense, and/or sell copies of the Software, and to #
# permit persons to whom the Software is furnished to do so, subject to the  #
# following conditions:                                                      #
#                                                                            #
# The above copyright notice and this permission notice shall be included in #
# all copies or substantial portions of the Software.                        #
#                                                                            #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR #
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   #
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    #
# THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      #
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    #
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        #
# DEALINGS IN THE SOFTWARE.                                                  #
#                                                                            #
# Except as contained in this notice, the name(s) of the above copyright     #
# holders shall not be used in advertising or otherwise to promote the sale, #
# use or other dealings in this Software without prior written               #
# authorization.                                                             #
##############################################################################
#
# Author: Thomas E. Dickey <dickey@clark.net> 1996,1997
#
# Master Makefile for ncurses library.

SHELL = /bin/sh

DESTDIR=
CF_MFLAGS =  DESTDIR="$(DESTDIR)"



NCURSES_MAJOR	= 5
NCURSES_MINOR	= 5
NCURSES_PATCH	= 20051010

prefix		= /usr/local
exec_prefix	= ${prefix}

bindir		= ${exec_prefix}/bin
ticdir		= /usr/local/share/terminfo
includedir	= $(prefix)/include/ncurses
libdir		= ${exec_prefix}/lib
mandir		= ${prefix}/man

INSTALL		= /bin/install -c
INSTALL_DATA	= ${INSTALL} -m 644

DIRS_TO_MAKE	= lib objects obj_g

all ::	$(DIRS_TO_MAKE)

$(DIRS_TO_MAKE) :
	mkdir $@

preinstall :
	@ echo ''
	@ echo '** Configuration summary for NCURSES $(NCURSES_MAJOR).$(NCURSES_MINOR) $(NCURSES_PATCH):'
	@ echo ''
	@ echo '     extended funcs: '`test 1 != 0 && echo yes || echo no`
	@ echo '     xterm terminfo: 'xterm-new
	@ echo ''
	@ echo '      bin directory: '$(bindir)
	@ echo '      lib directory: '$(libdir)
	@ echo '  include directory: '$(includedir)
	@ echo '      man directory: '$(mandir)
	@ echo ' terminfo directory: '$(ticdir)
	@ echo ''
	@ test "$(includedir)" = "$(prefix)/include" || \
		echo '** Include-directory is not in a standard location'
	@ test ! -f $(includedir)/termcap.h || \
		fgrep NCURSES_VERSION $(includedir)/termcap.h >/dev/null || \
		echo '** Will overwrite non-ncurses termcap.h'
	@ test ! -f $(includedir)/curses.h || \
		fgrep NCURSES_VERSION $(includedir)/curses.h >/dev/null || \
		echo '** Will overwrite non-ncurses curses.h'

# Put the common rules here so that we can easily construct the list of
# directories to visit.
all \
clean \
distclean \
mostlyclean \
realclean \
depend \
sources \
tags \
uninstall \
install ::
	cd man && $(MAKE) $(CF_MFLAGS) $@
	cd include && $(MAKE) $(CF_MFLAGS) $@
	cd ncurses && $(MAKE) $(CF_MFLAGS) $@
	cd progs && $(MAKE) $(CF_MFLAGS) $@
	cd tack && $(MAKE) $(CF_MFLAGS) $@
	cd panel && $(MAKE) $(CF_MFLAGS) $@
	cd menu && $(MAKE) $(CF_MFLAGS) $@
	cd form && $(MAKE) $(CF_MFLAGS) $@
	cd test && $(MAKE) $(CF_MFLAGS) $@
	cd misc && $(MAKE) $(CF_MFLAGS) $@
	cd c++ && $(MAKE) $(CF_MFLAGS) $@

libs \
install.libs \
uninstall.libs \
install.includes \
uninstall.includes ::
	cd include && $(MAKE) $(CF_MFLAGS) $@

lint \
libs \
lintlib \
install.libs \
uninstall.libs \
install.ncurses \
uninstall.ncurses ::
	cd ncurses && $(MAKE) $(CF_MFLAGS) $@

lint \
libs \
lintlib \
install.libs \
uninstall.libs \
install.progs \
uninstall.progs ::
	cd progs && $(MAKE) $(CF_MFLAGS) $@

lint \
libs \
lintlib \
install.libs \
uninstall.libs \
install.tack \
uninstall.tack ::
	cd tack && $(MAKE) $(CF_MFLAGS) $@

install.includes \
uninstall.includes \
lint \
libs \
lintlib \
install.libs \
uninstall.libs \
install.panel \
uninstall.panel ::
	cd panel && $(MAKE) $(CF_MFLAGS) $@

install.includes \
uninstall.includes \
lint \
libs \
lintlib \
install.libs \
uninstall.libs \
install.menu \
uninstall.menu ::
	cd menu && $(MAKE) $(CF_MFLAGS) $@

install.includes \
uninstall.includes \
lint \
libs \
lintlib \
install.libs \
uninstall.libs \
install.form \
uninstall.form ::
	cd form && $(MAKE) $(CF_MFLAGS) $@

lint \
libs \
lintlib \
install.libs \
uninstall.libs \
install.test \
uninstall.test ::
	cd test && $(MAKE) $(CF_MFLAGS) $@

install.includes \
uninstall.includes \
libs \
lintlib \
install.libs \
uninstall.libs \
install.c++ \
uninstall.c++ ::
	cd c++ && $(MAKE) $(CF_MFLAGS) $@

install.data \
uninstall.data ::
	cd misc && $(MAKE) $(CF_MFLAGS) $@

install.man \
uninstall.man ::
	cd man && $(MAKE) $(CF_MFLAGS) $@

distclean ::
	rm -f config.cache config.log config.status Makefile include/ncurses_cfg.h
	rm -f headers.sh headers.sed
	rm -rf $(DIRS_TO_MAKE)
