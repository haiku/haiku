# $Id: Makefile.in,v 1.42 2004/12/11 23:33:16 tom Exp $
##############################################################################
# Copyright (c) 1998-2002,2003 Free Software Foundation, Inc.                #
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
# Makefile for menu source code.
#
# This makes the following:
#	libraries (normal/debug/profile/shared)
#
# The variable 'srcdir' refers to the source-distribution, and can be set with
# the configure script by "--srcdir=DIR".
#
# The rules are organized to produce the libraries for the configured models,

# turn off _all_ suffix rules; we'll generate our own
.SUFFIXES:

SHELL		= /bin/sh
THIS		= Makefile

x		= 
o		= .o

MODEL		= normal
DESTDIR		= 
srcdir		= .
prefix		= /usr/local
exec_prefix	= ${prefix}
bindir		= ${exec_prefix}/bin
libdir		= ${exec_prefix}/lib
includedir	= $(prefix)/include/ncurses

LIBTOOL		= 
LIBTOOL_CLEAN	= 
LIBTOOL_COMPILE	= 
LIBTOOL_LINK	= 
LIBTOOL_INSTALL	= 
LIBTOOL_UNINSTALL = 

INSTALL		= /bin/install -c
INSTALL_LIB	= /bin/install -c -m 644
INSTALL_PROG	= @INSTALL_PROG@
INSTALL_DATA	= ${INSTALL} -m 644

AR		= ar
AR_OPTS		= rv
AWK		= gawk
LD		= ld
LN_S		= ln -s

CC		= gcc
CPP		= gcc -E
CFLAGS		= -O2

CPPFLAGS	= -I../ncurses -DHAVE_CONFIG_H -I. -I../include  -D_XOPEN_SOURCE=500 -D_POSIX_C_SOURCE=199506L -DNDEBUG -I$(includedir)

CCFLAGS		= $(CPPFLAGS) $(CFLAGS)

CFLAGS_LIBTOOL	= $(CCFLAGS)
CFLAGS_NORMAL	= $(CCFLAGS)
CFLAGS_DEBUG	= $(CCFLAGS) -g -DTRACE
CFLAGS_PROFILE	= $(CCFLAGS) -pg
CFLAGS_SHARED	= $(CCFLAGS) -fPIC

CFLAGS_DEFAULT	= $(CFLAGS_NORMAL)

LINK		= $(LIBTOOL_LINK) $(CC)
LDFLAGS		=   

SHLIB_DIRS	= -L../lib
SHLIB_LIST	= $(SHLIB_DIRS) -lncurses 

MK_SHARED_LIB	= $(CC) -o $@ -Xlinker -soname=`basename $@` -nostart -e 0

NCURSES_MAJOR	= 5
NCURSES_MINOR	= 5
REL_VERSION	= 5.5
ABI_VERSION	= 5

RANLIB		= ranlib

IMPORT_LIB	= 
SHARED_LIB	= 
LIBRARIES	=  ../lib/libmenu.a ../lib/libmenu_g.a

LINT		= 
LINT_OPTS	= 
LINT_LIBS	= -lmenu -lncurses 

AUTO_SRC	= \
		../include/menu.h \
		../include/eti.h \
		../include/mf_common.h

################################################################################
all \
libs \
install ::	$(AUTO_SRC) $(LIBRARIES)

sources :	$(AUTO_SRC)

$(DESTDIR)$(bindir) \
$(DESTDIR)$(libdir) :
	sh $(srcdir)/../mkinstalldirs $@

# make copies to simplify include-paths while still keeping menu's include
# file in this directory.
../include/menu.h : $(srcdir)/menu.h
	-rm -f $@
	cp $(srcdir)/menu.h $@
../include/eti.h : $(srcdir)/eti.h
	-rm -f $@
	cp $(srcdir)/eti.h $@
../include/mf_common.h : $(srcdir)/mf_common.h
	-rm -f $@
	cp $(srcdir)/mf_common.h $@

MENU_PRIV_H = \
	$(srcdir)/menu.priv.h \
	$(AUTO_SRC) \
	../include/curses.h

tags:
	ctags *.[ch]

TAGS:
	etags *.[ch]

mostlyclean ::
	-rm -f core tags TAGS *~ *.bak *.i *.ln *.atac trace

clean :: mostlyclean
	-rm -f $(AUTO_SRC)

distclean :: clean
	-rm -f Makefile

realclean :: distclean

###############################################################################
# The remainder of this file is automatically generated during configuration
###############################################################################

# generated by mk-0th.awk
#   libname:    menu
#   subsets:    termlib+ext_tinfo+base+ext_funcs

.SUFFIXES: .c .cc .h .i .ii
.c.i :
	$(CPP) $(CPPFLAGS) $< >$@
.cc.ii :
	$(CPP) $(CPPFLAGS) $< >$@
.h.i :
	$(CPP) $(CPPFLAGS) $< >$@

C_SRC = \
	$(srcdir)/m_attribs.c \
	$(srcdir)/m_cursor.c \
	$(srcdir)/m_driver.c \
	$(srcdir)/m_format.c \
	$(srcdir)/m_global.c \
	$(srcdir)/m_hook.c \
	$(srcdir)/m_item_cur.c \
	$(srcdir)/m_item_nam.c \
	$(srcdir)/m_item_new.c \
	$(srcdir)/m_item_opt.c \
	$(srcdir)/m_item_top.c \
	$(srcdir)/m_item_use.c \
	$(srcdir)/m_item_val.c \
	$(srcdir)/m_item_vis.c \
	$(srcdir)/m_items.c \
	$(srcdir)/m_new.c \
	$(srcdir)/m_opts.c \
	$(srcdir)/m_pad.c \
	$(srcdir)/m_pattern.c \
	$(srcdir)/m_post.c \
	$(srcdir)/m_req_name.c \
	$(srcdir)/m_scale.c \
	$(srcdir)/m_spacing.c \
	$(srcdir)/m_sub.c \
	$(srcdir)/m_trace.c \
	$(srcdir)/m_userptr.c \
	$(srcdir)/m_win.c

# Producing llib-lmenu is time-consuming, so there's no direct-dependency for
# it in the lintlib rule.  We'll only remove in the cleanest setup.
clean ::
	rm -f llib-lmenu.*

realclean ::
	rm -f llib-lmenu

llib-lmenu : $(C_SRC)
	cproto -a -l -DNCURSES_ENABLE_STDBOOL_H=0 -DLINT $(CPPFLAGS) $(C_SRC) >$@

lintlib :
	sh $(srcdir)/../misc/makellib menu $(CPPFLAGS)
lint :
	$(LINT) $(LINT_OPTS) $(CPPFLAGS) $(C_SRC) $(LINT_LIBS)

# generated by mk-1st.awk (subset=termlib+ext_tinfo+base+ext_funcs)
#  name:          menu
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
#  depend:        ../include/ncurses_cfg.h ./menu.priv.h
#  host:          i586-pc-beos

NORMAL_OBJS = \
	../objects/m_attribs$o \
	../objects/m_cursor$o \
	../objects/m_driver$o \
	../objects/m_format$o \
	../objects/m_global$o \
	../objects/m_hook$o \
	../objects/m_item_cur$o \
	../objects/m_item_nam$o \
	../objects/m_item_new$o \
	../objects/m_item_opt$o \
	../objects/m_item_top$o \
	../objects/m_item_use$o \
	../objects/m_item_val$o \
	../objects/m_item_vis$o \
	../objects/m_items$o \
	../objects/m_new$o \
	../objects/m_opts$o \
	../objects/m_pad$o \
	../objects/m_pattern$o \
	../objects/m_post$o \
	../objects/m_req_name$o \
	../objects/m_scale$o \
	../objects/m_spacing$o \
	../objects/m_sub$o \
	../objects/m_userptr$o \
	../objects/m_win$o

$(NORMAL_OBJS) : ../include/ncurses_cfg.h ./menu.priv.h

../lib/libmenu.a : $(NORMAL_OBJS)
	$(AR) $(AR_OPTS) $@ $?
	$(RANLIB) $@

install \
install.libs \
install.menu :: $(DESTDIR)$(libdir) ../lib/libmenu.a
	@echo installing ../lib/libmenu.a as $(DESTDIR)$(libdir)/libmenu.a
	$(INSTALL_DATA) ../lib/libmenu.a $(DESTDIR)$(libdir)/libmenu.a
	$(RANLIB) $(DESTDIR)$(libdir)/libmenu.a

uninstall \
uninstall.libs \
uninstall.menu ::
	@echo uninstalling $(DESTDIR)$(libdir)/libmenu.a
	-@rm -f $(DESTDIR)$(libdir)/libmenu.a

clean ::
	-rm -f ../lib/libmenu.a

mostlyclean::
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

../objects/m_attribs$o :	$(srcdir)/m_attribs.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../menu/m_attribs.c -o ../objects/m_attribs$o

../objects/m_cursor$o :	$(srcdir)/m_cursor.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../menu/m_cursor.c -o ../objects/m_cursor$o

../objects/m_driver$o :	$(srcdir)/m_driver.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../menu/m_driver.c -o ../objects/m_driver$o

../objects/m_format$o :	$(srcdir)/m_format.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../menu/m_format.c -o ../objects/m_format$o

../objects/m_global$o :	$(srcdir)/m_global.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../menu/m_global.c -o ../objects/m_global$o

../objects/m_hook$o :	$(srcdir)/m_hook.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../menu/m_hook.c -o ../objects/m_hook$o

../objects/m_item_cur$o :	$(srcdir)/m_item_cur.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../menu/m_item_cur.c -o ../objects/m_item_cur$o

../objects/m_item_nam$o :	$(srcdir)/m_item_nam.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../menu/m_item_nam.c -o ../objects/m_item_nam$o

../objects/m_item_new$o :	$(srcdir)/m_item_new.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../menu/m_item_new.c -o ../objects/m_item_new$o

../objects/m_item_opt$o :	$(srcdir)/m_item_opt.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../menu/m_item_opt.c -o ../objects/m_item_opt$o

../objects/m_item_top$o :	$(srcdir)/m_item_top.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../menu/m_item_top.c -o ../objects/m_item_top$o

../objects/m_item_use$o :	$(srcdir)/m_item_use.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../menu/m_item_use.c -o ../objects/m_item_use$o

../objects/m_item_val$o :	$(srcdir)/m_item_val.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../menu/m_item_val.c -o ../objects/m_item_val$o

../objects/m_item_vis$o :	$(srcdir)/m_item_vis.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../menu/m_item_vis.c -o ../objects/m_item_vis$o

../objects/m_items$o :	$(srcdir)/m_items.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../menu/m_items.c -o ../objects/m_items$o

../objects/m_new$o :	$(srcdir)/m_new.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../menu/m_new.c -o ../objects/m_new$o

../objects/m_opts$o :	$(srcdir)/m_opts.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../menu/m_opts.c -o ../objects/m_opts$o

../objects/m_pad$o :	$(srcdir)/m_pad.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../menu/m_pad.c -o ../objects/m_pad$o

../objects/m_pattern$o :	$(srcdir)/m_pattern.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../menu/m_pattern.c -o ../objects/m_pattern$o

../objects/m_post$o :	$(srcdir)/m_post.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../menu/m_post.c -o ../objects/m_post$o

../objects/m_req_name$o :	$(srcdir)/m_req_name.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../menu/m_req_name.c -o ../objects/m_req_name$o

../objects/m_scale$o :	$(srcdir)/m_scale.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../menu/m_scale.c -o ../objects/m_scale$o

../objects/m_spacing$o :	$(srcdir)/m_spacing.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../menu/m_spacing.c -o ../objects/m_spacing$o

../objects/m_sub$o :	$(srcdir)/m_sub.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../menu/m_sub.c -o ../objects/m_sub$o

../objects/m_userptr$o :	$(srcdir)/m_userptr.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../menu/m_userptr.c -o ../objects/m_userptr$o

../objects/m_win$o :	$(srcdir)/m_win.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../menu/m_win.c -o ../objects/m_win$o


# generated by mk-1st.awk (subset=termlib+ext_tinfo+base+ext_funcs)
#  name:          menu
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
#  depend:        ../include/ncurses_cfg.h ./menu.priv.h
#  host:          i586-pc-beos

DEBUG_OBJS = \
	../obj_g/m_attribs$o \
	../obj_g/m_cursor$o \
	../obj_g/m_driver$o \
	../obj_g/m_format$o \
	../obj_g/m_global$o \
	../obj_g/m_hook$o \
	../obj_g/m_item_cur$o \
	../obj_g/m_item_nam$o \
	../obj_g/m_item_new$o \
	../obj_g/m_item_opt$o \
	../obj_g/m_item_top$o \
	../obj_g/m_item_use$o \
	../obj_g/m_item_val$o \
	../obj_g/m_item_vis$o \
	../obj_g/m_items$o \
	../obj_g/m_new$o \
	../obj_g/m_opts$o \
	../obj_g/m_pad$o \
	../obj_g/m_pattern$o \
	../obj_g/m_post$o \
	../obj_g/m_req_name$o \
	../obj_g/m_scale$o \
	../obj_g/m_spacing$o \
	../obj_g/m_sub$o \
	../obj_g/m_trace$o \
	../obj_g/m_userptr$o \
	../obj_g/m_win$o

$(DEBUG_OBJS) : ../include/ncurses_cfg.h ./menu.priv.h

../lib/libmenu_g.a : $(DEBUG_OBJS)
	$(AR) $(AR_OPTS) $@ $?
	$(RANLIB) $@

install \
install.libs \
install.menu :: $(DESTDIR)$(libdir) ../lib/libmenu_g.a
	@echo installing ../lib/libmenu_g.a as $(DESTDIR)$(libdir)/libmenu_g.a
	$(INSTALL_DATA) ../lib/libmenu_g.a $(DESTDIR)$(libdir)/libmenu_g.a
	$(RANLIB) $(DESTDIR)$(libdir)/libmenu_g.a

uninstall \
uninstall.libs \
uninstall.menu ::
	@echo uninstalling $(DESTDIR)$(libdir)/libmenu_g.a
	-@rm -f $(DESTDIR)$(libdir)/libmenu_g.a

clean ::
	-rm -f ../lib/libmenu_g.a

mostlyclean::
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

../obj_g/m_attribs$o :	$(srcdir)/m_attribs.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_attribs.c -o ../obj_g/m_attribs$o

../obj_g/m_cursor$o :	$(srcdir)/m_cursor.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_cursor.c -o ../obj_g/m_cursor$o

../obj_g/m_driver$o :	$(srcdir)/m_driver.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_driver.c -o ../obj_g/m_driver$o

../obj_g/m_format$o :	$(srcdir)/m_format.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_format.c -o ../obj_g/m_format$o

../obj_g/m_global$o :	$(srcdir)/m_global.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_global.c -o ../obj_g/m_global$o

../obj_g/m_hook$o :	$(srcdir)/m_hook.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_hook.c -o ../obj_g/m_hook$o

../obj_g/m_item_cur$o :	$(srcdir)/m_item_cur.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_item_cur.c -o ../obj_g/m_item_cur$o

../obj_g/m_item_nam$o :	$(srcdir)/m_item_nam.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_item_nam.c -o ../obj_g/m_item_nam$o

../obj_g/m_item_new$o :	$(srcdir)/m_item_new.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_item_new.c -o ../obj_g/m_item_new$o

../obj_g/m_item_opt$o :	$(srcdir)/m_item_opt.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_item_opt.c -o ../obj_g/m_item_opt$o

../obj_g/m_item_top$o :	$(srcdir)/m_item_top.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_item_top.c -o ../obj_g/m_item_top$o

../obj_g/m_item_use$o :	$(srcdir)/m_item_use.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_item_use.c -o ../obj_g/m_item_use$o

../obj_g/m_item_val$o :	$(srcdir)/m_item_val.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_item_val.c -o ../obj_g/m_item_val$o

../obj_g/m_item_vis$o :	$(srcdir)/m_item_vis.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_item_vis.c -o ../obj_g/m_item_vis$o

../obj_g/m_items$o :	$(srcdir)/m_items.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_items.c -o ../obj_g/m_items$o

../obj_g/m_new$o :	$(srcdir)/m_new.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_new.c -o ../obj_g/m_new$o

../obj_g/m_opts$o :	$(srcdir)/m_opts.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_opts.c -o ../obj_g/m_opts$o

../obj_g/m_pad$o :	$(srcdir)/m_pad.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_pad.c -o ../obj_g/m_pad$o

../obj_g/m_pattern$o :	$(srcdir)/m_pattern.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_pattern.c -o ../obj_g/m_pattern$o

../obj_g/m_post$o :	$(srcdir)/m_post.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_post.c -o ../obj_g/m_post$o

../obj_g/m_req_name$o :	$(srcdir)/m_req_name.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_req_name.c -o ../obj_g/m_req_name$o

../obj_g/m_scale$o :	$(srcdir)/m_scale.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_scale.c -o ../obj_g/m_scale$o

../obj_g/m_spacing$o :	$(srcdir)/m_spacing.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_spacing.c -o ../obj_g/m_spacing$o

../obj_g/m_sub$o :	$(srcdir)/m_sub.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_sub.c -o ../obj_g/m_sub$o

../obj_g/m_trace$o :	$(srcdir)/m_trace.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_trace.c -o ../obj_g/m_trace$o

../obj_g/m_userptr$o :	$(srcdir)/m_userptr.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_userptr.c -o ../obj_g/m_userptr$o

../obj_g/m_win$o :	$(srcdir)/m_win.c \
			$(MENU_PRIV_H)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../menu/m_win.c -o ../obj_g/m_win$o

$(DESTDIR)$(includedir) :
	sh $(srcdir)/../mkinstalldirs $@

install \
install.libs \
install.includes :: $(AUTO_SRC) $(DESTDIR)$(includedir) \
		$(srcdir)/eti.h \
		$(srcdir)/menu.h
	@ (cd $(DESTDIR)$(includedir) && rm -f eti.h) ; ../headers.sh $(INSTALL_DATA) $(DESTDIR)$(includedir) $(srcdir) $(srcdir)/eti.h
	@ (cd $(DESTDIR)$(includedir) && rm -f menu.h) ; ../headers.sh $(INSTALL_DATA) $(DESTDIR)$(includedir) $(srcdir) $(srcdir)/menu.h

uninstall \
uninstall.libs \
uninstall.includes ::
	-@ (cd $(DESTDIR)$(includedir) && rm -f eti.h)
	-@ (cd $(DESTDIR)$(includedir) && rm -f menu.h)
depend : $(AUTO_SRC)
	makedepend -- $(CPPFLAGS) -- $(C_SRC)

# DO NOT DELETE THIS LINE -- make depend depends on it.
