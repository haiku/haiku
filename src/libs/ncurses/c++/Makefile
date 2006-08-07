# $Id: Makefile.in,v 1.77 2005/06/12 22:52:56 tom Exp $
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
# Author: Thomas E. Dickey
#
#  Simple makefile for c++ window class demo

# turn off _all_ suffix rules; we'll generate our own
.SUFFIXES:

SHELL		= /bin/sh

CF_MFLAGS 	= 

x		= 
o		= .o

MODEL		= ../objects
DESTDIR		= 
srcdir		= .
prefix		= /usr/local
exec_prefix	= ${prefix}
libdir		= ${exec_prefix}/lib
includedir	= $(prefix)/include/ncurses

LIBTOOL		= 
LIBTOOL_CLEAN	= 
LIBTOOL_COMPILE	= 
LIBTOOL_LINK	= 
LIBTOOL_INSTALL	= 
LIBTOOL_UNINSTALL = 

INSTALL		= /bin/install -c
INSTALL_DATA	= ${INSTALL} -m 644

AR		= ar
AR_OPTS		= rv

CXX_AR		= $(AR)
CXX_AR_OPTS	= $(AR_OPTS)
RANLIB		= ranlib

CXX		= /boot/develop/bin/g++
CPP		= /boot/develop/bin/g++ -E
CXXFLAGS	= -O2 
CXXLIBS		=  -lstdc++

INCDIR		= ../include
CPPFLAGS	= -I../c++ -I$(INCDIR) -I$(srcdir) -DHAVE_CONFIG_H -I. -I../include  -D_XOPEN_SOURCE=500 -D_POSIX_C_SOURCE=199506L -DNDEBUG -I$(includedir)

CCFLAGS		= $(CPPFLAGS) $(CXXFLAGS)

CFLAGS_LIBTOOL	= $(CCFLAGS)
CFLAGS_NORMAL	= $(CCFLAGS)
CFLAGS_DEBUG	= $(CCFLAGS) -g -DTRACE
CFLAGS_PROFILE	= $(CCFLAGS) -pg
CFLAGS_SHARED	= $(CCFLAGS) -fPIC

CFLAGS_DEFAULT	= $(CFLAGS_NORMAL)

NCURSES_MAJOR	= 5
NCURSES_MINOR	= 5
REL_VERSION     = 5.5
ABI_VERSION	= 5

LINK		=  $(LIBTOOL_LINK) $(CXX) 
SHLIB_DIRS	= -L../lib
SHLIB_LIST	= $(SHLIB_DIRS) -lncurses 

LIBROOT		= ncurses++

LIBNAME_LIBTOOL	= lib$(LIBROOT).la
LIBNAME_NORMAL	= lib$(LIBROOT).a
LIBNAME		= lib$(LIBROOT).a

LINK_FLAGS	=  -L../lib -l$(LIBROOT)

LINK_LIBTOOL	=  -L../lib ../lib/$(LIBNAME)
LINK_NORMAL	= $(LINK_FLAGS)
LINK_DEBUG	= $(LINK_FLAGS)
LINK_PROFILE	= $(LINK_FLAGS)
LINK_SHARED	= $(LINK_FLAGS)

LDFLAGS		= -L../lib -lform -lmenu -lpanel -lncurses   \
	   $(CXXLIBS)

LDFLAGS_LIBTOOL	= $(LDFLAGS) $(CFLAGS_LIBTOOL)
LDFLAGS_NORMAL	= $(LDFLAGS) $(CFLAGS_NORMAL)
LDFLAGS_DEBUG	= $(LDFLAGS) $(CFLAGS_DEBUG)
LDFLAGS_PROFILE	= $(LDFLAGS) $(CFLAGS_PROFILE)
LDFLAGS_SHARED	= $(LDFLAGS) $(CFLAGS_SHARED) 

LDFLAGS_DEFAULT	= $(LINK_NORMAL) $(LDFLAGS_NORMAL)

AUTO_SRC	= \
		etip.h

all \
libs ::		$(AUTO_SRC) ../lib/$(LIBNAME)

all ::		demo$x

sources :	$(AUTO_SRC)

depend :

tags:
	ctags *.[h] *.cc

# Build a conventional library for installing, since a shared library would
# pull in all of the ncurses libraries (panel, menu, form, ncurses) as direct
# dependencies.
LIB_OBJS = \
	$(MODEL)/cursesf$o \
	$(MODEL)/cursesm$o \
	$(MODEL)/cursesw$o \
	$(MODEL)/cursespad$o \
	$(MODEL)/cursesp$o \
	$(MODEL)/cursslk$o \
	$(MODEL)/cursesapp$o \
	$(MODEL)/cursesmain$o

../lib/$(LIBNAME_NORMAL) : $(LIB_OBJS)
	$(CXX_AR) $(CXX_AR_OPTS) $@ $?
	$(RANLIB) $@

../lib/$(LIBNAME_LIBTOOL) : $(LIB_OBJS)
	cd ../lib && $(LIBTOOL_LINK) $(CXX) $(CXXFLAGS) \
		-o $(LIBNAME) $(LIB_OBJS:$o=.lo) \
		-rpath $(INSTALL_PREFIX)$(libdir) \
		-version-info $(NCURSES_MAJOR):$(NCURSES_MINOR) $(SHLIB_LIST)

OBJS_DEMO = $(MODEL)/demo$o

$(MODEL)/demo$o : $(srcdir)/demo.cc \
	$(cursesf_h) $(cursesm_h) $(cursesapp_h)

demo$x:	$(OBJS_DEMO) \
	../lib/$(LIBNAME)  \
	../lib/libform.a ../lib/libmenu.a ../lib/libpanel.a ../lib/libncurses.a 
	 $(LINK) -o $@ $(OBJS_DEMO) $(LDFLAGS_DEFAULT)

etip.h:	$(srcdir)/etip.h.in $(srcdir)/edit_cfg.sh
	cp $(srcdir)/etip.h.in $@
	sh $(srcdir)/edit_cfg.sh ../include/ncurses_cfg.h $@

$(DESTDIR)$(libdir) :
	sh $(srcdir)/../mkinstalldirs $@

install \
install.libs:: ../lib/$(LIBNAME) $(DESTDIR)$(libdir)
	$(LIBTOOL_INSTALL) $(INSTALL) ../lib/$(LIBNAME) $(DESTDIR)$(libdir)/$(LIBNAME)

uninstall \
uninstall.libs::
	-$(LIBTOOL_UNINSTALL) rm -f $(DESTDIR)$(libdir)/$(LIBNAME)

mostlyclean ::
	-rm -f core tags TAGS *~ *.bak *.i *.ii *.ln *.atac trace

clean :: mostlyclean
	-sh -c "if test -n '$x' ; then $(MAKE) clean x=''; fi"
	-rm -rf $(MODEL)/SunWS_cache
	-$(LIBTOOL_CLEAN) rm -f demo$x $(AUTO_SRC) ../lib/$(LIBNAME) $(LIB_OBJS) $(OBJS_DEMO)
	-rm -rf .libs

distclean :: clean
	-rm -f Makefile

realclean :: distclean

###############################################################################

cursesw_h	= $(srcdir)/cursesw.h \
		  etip.h \
		  $(INCDIR)/curses.h

cursesp_h	= $(srcdir)/cursesp.h \
		  $(cursesw_h) \
		  $(INCDIR)/panel.h

cursesf_h	= $(srcdir)/cursesf.h \
		  $(cursesp_h) \
		  $(INCDIR)/form.h

cursesm_h	= $(srcdir)/cursesm.h \
		  $(cursesp_h) \
		  $(INCDIR)/menu.h

cursslk_h	= $(srcdir)/cursslk.h \
		  $(cursesw_h)

cursesapp_h	= $(srcdir)/cursesapp.h \
		  $(cursslk_h)

$(INCDIR)/form.h :
	cd ../form && $(MAKE) $@

$(INCDIR)/menu.h :
	cd ../menu && $(MAKE) $@

$(INCDIR)/panel.h :
	cd ../panel && $(MAKE) $@

###############################################################################
# The remainder of this file is automatically generated during configuration
###############################################################################

# generated by mk-0th.awk
#   libname:    c++
#   subsets:    termlib+ext_tinfo+base+ext_funcs

.SUFFIXES: .c .cc .h .i .ii
.c.i :
	$(CPP) $(CPPFLAGS) $< >$@
.cc.ii :
	$(CPP) $(CPPFLAGS) $< >$@
.h.i :
	$(CPP) $(CPPFLAGS) $< >$@

C_SRC = \
	$(srcdir)/cursesf.cc \
	$(srcdir)/cursesm.cc \
	$(srcdir)/cursesp.cc \
	$(srcdir)/cursesw.cc \
	$(srcdir)/cursespad.cc \
	$(srcdir)/cursslk.cc \
	$(srcdir)/cursesapp.cc \
	$(srcdir)/cursesmain.cc \
	$(srcdir)/demo.cc

lintlib :
	@echo no action needed

# generated by mk-1st.awk (subset=termlib+ext_tinfo+base+ext_funcs)
#  name:          c++
#  traces:        DEBUG
#  MODEL:         NORMAL
#  model:         objects
#  prefix:        lib
#  suffix:        .a
#  subset:        termlib+ext_tinfo+base+ext_funcs
#  ShlibVer:      auto
#  ShlibVerInfix: no
#  DoLinks:       no
#  rmSoLocs:      no
#  ldconfig:      
#  overwrite:     no
#  depend:        ../include/ncurses_cfg.h
#  host:          i586-pc-beos

NORMAL_OBJS = \
	../objects/cursesf$o \
	../objects/cursesm$o \
	../objects/cursesp$o \
	../objects/cursesw$o \
	../objects/cursespad$o \
	../objects/cursslk$o \
	../objects/cursesapp$o \
	../objects/cursesmain$o \
	../objects/demo$o

$(NORMAL_OBJS) : ../include/ncurses_cfg.h

mostlyclean::
	-rm -f $(NORMAL_OBJS)

clean ::
	-rm -f $(NORMAL_OBJS)

# generated by mk-2nd.awk
#   model:      objects
#   MODEL:      NORMAL
#   echo:       yes
#   subset:     termlib+ext_tinfo+base+ext_funcs
#   crenames:   yes
#   cxxrenames: yes
#   traces:     DEBUG
#   srcdir:     .

../objects/cursesf$o :	$(srcdir)/cursesf.cc \
			$(cursesf_h) \
			$(cursesapp_h)
	$(LIBTOOL_COMPILE) $(CXX) $(CFLAGS_NORMAL) -c ../c++/cursesf.cc -o ../objects/cursesf$o

../objects/cursesm$o :	$(srcdir)/cursesm.cc \
			$(cursesm_h) \
			$(cursesapp_h)
	$(LIBTOOL_COMPILE) $(CXX) $(CFLAGS_NORMAL) -c ../c++/cursesm.cc -o ../objects/cursesm$o

../objects/cursesp$o :	$(srcdir)/cursesp.cc \
			$(cursesp_h)
	$(LIBTOOL_COMPILE) $(CXX) $(CFLAGS_NORMAL) -c ../c++/cursesp.cc -o ../objects/cursesp$o

../objects/cursesw$o :	$(srcdir)/cursesw.cc \
			$(cursesw_h)
	$(LIBTOOL_COMPILE) $(CXX) $(CFLAGS_NORMAL) -c ../c++/cursesw.cc -o ../objects/cursesw$o

../objects/cursespad$o :	$(srcdir)/cursespad.cc \
			$(cursesw_h)
	$(LIBTOOL_COMPILE) $(CXX) $(CFLAGS_NORMAL) -c ../c++/cursespad.cc -o ../objects/cursespad$o

../objects/cursslk$o :	$(srcdir)/cursslk.cc \
			$(cursslk_h) \
			$(cursesapp_h)
	$(LIBTOOL_COMPILE) $(CXX) $(CFLAGS_NORMAL) -c ../c++/cursslk.cc -o ../objects/cursslk$o

../objects/cursesapp$o :	$(srcdir)/cursesapp.cc \
			$(cursesapp_h)
	$(LIBTOOL_COMPILE) $(CXX) $(CFLAGS_NORMAL) -c ../c++/cursesapp.cc -o ../objects/cursesapp$o

../objects/cursesmain$o :	$(srcdir)/cursesmain.cc \
			$(cursesapp_h)
	$(LIBTOOL_COMPILE) $(CXX) $(CFLAGS_NORMAL) -c ../c++/cursesmain.cc -o ../objects/cursesmain$o

../objects/demo$o :	$(srcdir)/demo.cc \
			$(cursesf_h) \
			$(cursesm_h) \
			$(cursesapp_h)
	$(LIBTOOL_COMPILE) $(CXX) $(CFLAGS_NORMAL) -c ../c++/demo.cc -o ../objects/demo$o


# generated by mk-1st.awk (subset=termlib+ext_tinfo+base+ext_funcs)
#  name:          c++
#  traces:        DEBUG
#  MODEL:         DEBUG
#  model:         obj_g
#  prefix:        lib
#  suffix:        _g.a
#  subset:        termlib+ext_tinfo+base+ext_funcs
#  ShlibVer:      auto
#  ShlibVerInfix: no
#  DoLinks:       no
#  rmSoLocs:      no
#  ldconfig:      
#  overwrite:     no
#  depend:        ../include/ncurses_cfg.h
#  host:          i586-pc-beos

DEBUG_OBJS = \
	../obj_g/cursesf$o \
	../obj_g/cursesm$o \
	../obj_g/cursesp$o \
	../obj_g/cursesw$o \
	../obj_g/cursespad$o \
	../obj_g/cursslk$o \
	../obj_g/cursesapp$o \
	../obj_g/cursesmain$o \
	../obj_g/demo$o

$(DEBUG_OBJS) : ../include/ncurses_cfg.h

mostlyclean::
	-rm -f $(DEBUG_OBJS)

clean ::
	-rm -f $(DEBUG_OBJS)

# generated by mk-2nd.awk
#   model:      obj_g
#   MODEL:      DEBUG
#   echo:       yes
#   subset:     termlib+ext_tinfo+base+ext_funcs
#   crenames:   yes
#   cxxrenames: yes
#   traces:     DEBUG
#   srcdir:     .

../obj_g/cursesf$o :	$(srcdir)/cursesf.cc \
			$(cursesf_h) \
			$(cursesapp_h)
	$(LIBTOOL_COMPILE) $(CXX) $(CFLAGS_DEBUG) -c ../c++/cursesf.cc -o ../obj_g/cursesf$o

../obj_g/cursesm$o :	$(srcdir)/cursesm.cc \
			$(cursesm_h) \
			$(cursesapp_h)
	$(LIBTOOL_COMPILE) $(CXX) $(CFLAGS_DEBUG) -c ../c++/cursesm.cc -o ../obj_g/cursesm$o

../obj_g/cursesp$o :	$(srcdir)/cursesp.cc \
			$(cursesp_h)
	$(LIBTOOL_COMPILE) $(CXX) $(CFLAGS_DEBUG) -c ../c++/cursesp.cc -o ../obj_g/cursesp$o

../obj_g/cursesw$o :	$(srcdir)/cursesw.cc \
			$(cursesw_h)
	$(LIBTOOL_COMPILE) $(CXX) $(CFLAGS_DEBUG) -c ../c++/cursesw.cc -o ../obj_g/cursesw$o

../obj_g/cursespad$o :	$(srcdir)/cursespad.cc \
			$(cursesw_h)
	$(LIBTOOL_COMPILE) $(CXX) $(CFLAGS_DEBUG) -c ../c++/cursespad.cc -o ../obj_g/cursespad$o

../obj_g/cursslk$o :	$(srcdir)/cursslk.cc \
			$(cursslk_h) \
			$(cursesapp_h)
	$(LIBTOOL_COMPILE) $(CXX) $(CFLAGS_DEBUG) -c ../c++/cursslk.cc -o ../obj_g/cursslk$o

../obj_g/cursesapp$o :	$(srcdir)/cursesapp.cc \
			$(cursesapp_h)
	$(LIBTOOL_COMPILE) $(CXX) $(CFLAGS_DEBUG) -c ../c++/cursesapp.cc -o ../obj_g/cursesapp$o

../obj_g/cursesmain$o :	$(srcdir)/cursesmain.cc \
			$(cursesapp_h)
	$(LIBTOOL_COMPILE) $(CXX) $(CFLAGS_DEBUG) -c ../c++/cursesmain.cc -o ../obj_g/cursesmain$o

../obj_g/demo$o :	$(srcdir)/demo.cc \
			$(cursesf_h) \
			$(cursesm_h) \
			$(cursesapp_h)
	$(LIBTOOL_COMPILE) $(CXX) $(CFLAGS_DEBUG) -c ../c++/demo.cc -o ../obj_g/demo$o

$(DESTDIR)$(includedir) :
	sh $(srcdir)/../mkinstalldirs $@

install \
install.libs \
install.includes :: $(AUTO_SRC) $(DESTDIR)$(includedir) \
		$(srcdir)/cursesapp.h \
		$(srcdir)/cursesf.h \
		$(srcdir)/cursesm.h \
		$(srcdir)/cursesp.h \
		$(srcdir)/cursesw.h \
		$(srcdir)/cursslk.h \
		etip.h
	@ (cd $(DESTDIR)$(includedir) && rm -f cursesapp.h) ; ../headers.sh $(INSTALL_DATA) $(DESTDIR)$(includedir) $(srcdir) $(srcdir)/cursesapp.h
	@ (cd $(DESTDIR)$(includedir) && rm -f cursesf.h) ; ../headers.sh $(INSTALL_DATA) $(DESTDIR)$(includedir) $(srcdir) $(srcdir)/cursesf.h
	@ (cd $(DESTDIR)$(includedir) && rm -f cursesm.h) ; ../headers.sh $(INSTALL_DATA) $(DESTDIR)$(includedir) $(srcdir) $(srcdir)/cursesm.h
	@ (cd $(DESTDIR)$(includedir) && rm -f cursesp.h) ; ../headers.sh $(INSTALL_DATA) $(DESTDIR)$(includedir) $(srcdir) $(srcdir)/cursesp.h
	@ (cd $(DESTDIR)$(includedir) && rm -f cursesw.h) ; ../headers.sh $(INSTALL_DATA) $(DESTDIR)$(includedir) $(srcdir) $(srcdir)/cursesw.h
	@ (cd $(DESTDIR)$(includedir) && rm -f cursslk.h) ; ../headers.sh $(INSTALL_DATA) $(DESTDIR)$(includedir) $(srcdir) $(srcdir)/cursslk.h
	@ (cd $(DESTDIR)$(includedir) && rm -f etip.h) ; ../headers.sh $(INSTALL_DATA) $(DESTDIR)$(includedir) $(srcdir) etip.h

uninstall \
uninstall.libs \
uninstall.includes ::
	-@ (cd $(DESTDIR)$(includedir) && rm -f cursesapp.h)
	-@ (cd $(DESTDIR)$(includedir) && rm -f cursesf.h)
	-@ (cd $(DESTDIR)$(includedir) && rm -f cursesm.h)
	-@ (cd $(DESTDIR)$(includedir) && rm -f cursesp.h)
	-@ (cd $(DESTDIR)$(includedir) && rm -f cursesw.h)
	-@ (cd $(DESTDIR)$(includedir) && rm -f cursslk.h)
	-@ (cd $(DESTDIR)$(includedir) && rm -f etip.h)
