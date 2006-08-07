# $Id: Makefile.in,v 1.84 2005/09/24 23:05:14 tom Exp $
##############################################################################
# Copyright (c) 1998-2003,2005 Free Software Foundation, Inc.                #
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
# Author: Thomas E. Dickey 1996-2002
#
# Makefile for ncurses tests.

# turn off _all_ suffix rules; we'll generate our own
.SUFFIXES:

SHELL		= /bin/sh



x		= 
o		= .o

MODEL		= ../objects
srcdir		= .
prefix		= /usr/local
exec_prefix	= ${prefix}
libdir		= ${exec_prefix}/lib
includedir	= $(prefix)/include/ncurses

LIBTOOL		= 
LIBTOOL_CLEAN	= 
LIBTOOL_COMPILE	= 
LIBTOOL_LINK	= 

CC		= gcc
CPP		= gcc -E

CFLAGS		= -O2
CPPFLAGS	=  -I../test -I$(srcdir) -DHAVE_CONFIG_H -I. -I../include  -D_XOPEN_SOURCE=500 -D_POSIX_C_SOURCE=199506L -DNDEBUG -I$(includedir)

CCFLAGS		= $(CPPFLAGS) $(CFLAGS)

CFLAGS_LIBTOOL	= $(CCFLAGS)
CFLAGS_NORMAL	= $(CCFLAGS)
CFLAGS_DEBUG	= $(CCFLAGS) -g -DTRACE
CFLAGS_PROFILE	= $(CCFLAGS) -pg
CFLAGS_SHARED	= $(CCFLAGS) -fPIC

CFLAGS_DEFAULT	= $(CFLAGS_NORMAL)

REL_VERSION	= 5.5
ABI_VERSION	= 5
LOCAL_LIBS	= ../lib/libform.a ../lib/libmenu.a ../lib/libpanel.a ../lib/libncurses.a 
MATH_LIB	= 

LD		= ld
LINK		=  $(LIBTOOL_LINK) $(CC) $(CFLAGS)

LDFLAGS		=   

LDFLAGS_LIBTOOL	= $(LDFLAGS) $(CFLAGS_LIBTOOL)
LDFLAGS_NORMAL	= $(LDFLAGS) $(CFLAGS_NORMAL)
LDFLAGS_DEBUG	= $(LDFLAGS) $(CFLAGS_DEBUG)
LDFLAGS_PROFILE	= $(LDFLAGS) $(CFLAGS_PROFILE)
LDFLAGS_SHARED	= $(LDFLAGS) $(CFLAGS_SHARED) 

# use these for linking with all of the libraries
LIBS_DEFAULT	= -L../lib -lform -lmenu -lpanel -lncurses  
LDFLAGS_DEFAULT	= $(LDFLAGS_NORMAL) $(LIBS_DEFAULT)

# use these for linking with the (n)curses library
LIBS_CURSES	= `echo "-L../lib -lform -lmenu -lpanel -lncurses  " | sed -e 's/-lform.*-lpanel[^ ]*//'`
LDFLAGS_CURSES	= $(LDFLAGS_NORMAL) $(LIBS_CURSES)

# use these for linking with the tinfo library if we have it, or curses library if not
LIBS_TINFO	= -L../lib -lncurses 
LDFLAGS_TINFO	= $(LDFLAGS_NORMAL) $(LIBS_TINFO)

LINT		= 
LINT_OPTS	= 
LINT_LIBS	= -lform -lmenu -lpanel -lncurses 

HEADER_DEPS	= \
	../include/curses.h \
	../include/term.h

TESTS = \
	background$x \
	blue$x \
	bs$x \
	cardfile$x \
	color_set$x \
	demo_defkey$x \
	demo_forms$x \
	demo_keyok$x \
	demo_menus$x \
	demo_panels$x \
	demo_termcap$x \
	ditto$x \
	dots$x \
	filter$x \
	firework$x \
	firstlast$x \
	gdc$x \
	hanoi$x \
	hashtest$x \
	ins_wide$x \
	inserts$x \
	keynames$x \
	knight$x \
	lrtest$x \
	ncurses$x \
	newdemo$x \
	railroad$x \
	rain$x \
	tclock$x \
	testaddch$x \
	testcurs$x \
	testscanw$x \
	view$x \
	worm$x \
	xmas$x

all: $(TESTS)

sources:

tags:
	ctags *.[ch]

background$x: $(MODEL)/background$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/background$o $(LDFLAGS_CURSES)

blue$x: $(MODEL)/blue$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/blue$o $(LDFLAGS_CURSES)

bs$x: $(MODEL)/bs$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/bs$o $(LDFLAGS_CURSES)

cardfile$x: $(MODEL)/cardfile$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/cardfile$o $(LDFLAGS_DEFAULT)

color_set$x: $(MODEL)/color_set$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/color_set$o $(LDFLAGS_CURSES)

demo_defkey$x: $(MODEL)/demo_defkey$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/demo_defkey$o $(LDFLAGS_CURSES)

demo_forms$x: $(MODEL)/demo_forms$o $(MODEL)/edit_field$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/demo_forms$o $(MODEL)/edit_field$o $(LDFLAGS_DEFAULT)

demo_keyok$x: $(MODEL)/demo_keyok$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/demo_keyok$o $(LDFLAGS_CURSES)

demo_menus$x: $(MODEL)/demo_menus$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/demo_menus$o $(LDFLAGS_DEFAULT)

demo_panels$x: $(MODEL)/demo_panels$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/demo_panels$o $(LDFLAGS_DEFAULT)

demo_termcap$x: $(MODEL)/demo_termcap$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/demo_termcap$o $(LDFLAGS_TINFO)

ditto$x: $(MODEL)/ditto$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/ditto$o $(LDFLAGS_CURSES)

dots$x: $(MODEL)/dots$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/dots$o $(LDFLAGS_TINFO)

filter$x: $(MODEL)/filter$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/filter$o $(LDFLAGS_CURSES)

firework$x: $(MODEL)/firework$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/firework$o $(LDFLAGS_CURSES)

firstlast$x: $(MODEL)/firstlast$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/firstlast$o $(LDFLAGS_DEFAULT)

gdc$x: $(MODEL)/gdc$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/gdc$o $(LDFLAGS_CURSES)

hanoi$x: $(MODEL)/hanoi$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/hanoi$o $(LDFLAGS_CURSES)

hashtest$x: $(MODEL)/hashtest$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/hashtest$o $(LDFLAGS_CURSES)

inserts$x: $(MODEL)/inserts$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/inserts$o $(LDFLAGS_CURSES)

ins_wide$x: $(MODEL)/ins_wide$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/ins_wide$o $(LDFLAGS_CURSES)

keynames$x: $(MODEL)/keynames$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/keynames$o $(LDFLAGS_CURSES)

knight$x: $(MODEL)/knight$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/knight$o $(LDFLAGS_CURSES)

lrtest$x: $(MODEL)/lrtest$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/lrtest$o $(LDFLAGS_CURSES)

ncurses$x: $(MODEL)/ncurses$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/ncurses$o $(LDFLAGS_DEFAULT)

newdemo$x: $(MODEL)/newdemo$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/newdemo$o $(LDFLAGS_CURSES)

rain$x: $(MODEL)/rain$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/rain$o $(LDFLAGS_CURSES)

railroad$x: $(MODEL)/railroad$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/railroad$o $(LDFLAGS_TINFO)

tclock$x: $(MODEL)/tclock$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/tclock$o $(LDFLAGS_CURSES) $(MATH_LIB)

testaddch$x: $(MODEL)/testaddch$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/testaddch$o $(LDFLAGS_CURSES)

testcurs$x: $(MODEL)/testcurs$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/testcurs$o $(LDFLAGS_CURSES)

testscanw$x: $(MODEL)/testscanw$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/testscanw$o $(LDFLAGS_CURSES)

view$x: $(MODEL)/view$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/view$o $(LDFLAGS_CURSES)

worm$x: $(MODEL)/worm$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/worm$o $(LDFLAGS_CURSES)

xmas$x: $(MODEL)/xmas$o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/xmas$o $(LDFLAGS_CURSES)

libs \
install \
install.libs \
install.test:

uninstall:
uninstall.libs:
uninstall.test:

mostlyclean ::
	-rm -f core tags TAGS *~ *.bak *.i *.ln *.atac trace

clean :: mostlyclean
	-sh -c "if test -n '$x' ; then $(MAKE) clean x=''; fi"
	-rm -rf *$o screendump *.lis $(TESTS) .libs

distclean :: clean
	-rm -f Makefile ncurses_cfg.h config.*

realclean :: distclean

lint:
	sh -c 'for N in $(TESTS); do echo LINT:$$N; $(LINT) $(LINT_OPTS) $(CPPFLAGS) $(srcdir)/$$N.c $(LINT_LIBS); done'

# generated by mk-0th.awk
#   libname:    test
#   subsets:    termlib+ext_tinfo+base+ext_funcs

.SUFFIXES: .c .cc .h .i .ii
.c.i :
	$(CPP) $(CPPFLAGS) $< >$@
.cc.ii :
	$(CPP) $(CPPFLAGS) $< >$@
.h.i :
	$(CPP) $(CPPFLAGS) $< >$@

C_SRC = \
	$(srcdir)/background.c \
	$(srcdir)/blue.c \
	$(srcdir)/bs.c \
	$(srcdir)/cardfile.c \
	$(srcdir)/color_set.c \
	$(srcdir)/demo_defkey.c \
	$(srcdir)/demo_keyok.c \
	$(srcdir)/demo_forms.c \
	$(srcdir)/demo_menus.c \
	$(srcdir)/demo_panels.c \
	$(srcdir)/demo_termcap.c \
	$(srcdir)/ditto.c \
	$(srcdir)/dots.c \
	$(srcdir)/edit_field.c \
	$(srcdir)/filter.c \
	$(srcdir)/firework.c \
	$(srcdir)/firstlast.c \
	$(srcdir)/gdc.c \
	$(srcdir)/hanoi.c \
	$(srcdir)/hashtest.c \
	$(srcdir)/ins_wide.c \
	$(srcdir)/inserts.c \
	$(srcdir)/keynames.c \
	$(srcdir)/knight.c \
	$(srcdir)/lrtest.c \
	$(srcdir)/ncurses.c \
	$(srcdir)/newdemo.c \
	$(srcdir)/railroad.c \
	$(srcdir)/rain.c \
	$(srcdir)/tclock.c \
	$(srcdir)/testaddch.c \
	$(srcdir)/testcurs.c \
	$(srcdir)/testscanw.c \
	$(srcdir)/view.c \
	$(srcdir)/worm.c \
	$(srcdir)/xmas.c

lintlib :
	@echo no action needed

# generated by mk-1st.awk (subset=termlib+ext_tinfo+base+ext_funcs)
#  name:          test
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
#  depend:        ../include/ncurses_cfg.h ./test.priv.h
#  host:          i586-pc-beos

NORMAL_OBJS = \
	../objects/background$o \
	../objects/blue$o \
	../objects/bs$o \
	../objects/cardfile$o \
	../objects/color_set$o \
	../objects/demo_defkey$o \
	../objects/demo_keyok$o \
	../objects/demo_forms$o \
	../objects/demo_menus$o \
	../objects/demo_panels$o \
	../objects/demo_termcap$o \
	../objects/ditto$o \
	../objects/dots$o \
	../objects/edit_field$o \
	../objects/filter$o \
	../objects/firework$o \
	../objects/firstlast$o \
	../objects/gdc$o \
	../objects/hanoi$o \
	../objects/hashtest$o \
	../objects/ins_wide$o \
	../objects/inserts$o \
	../objects/keynames$o \
	../objects/knight$o \
	../objects/lrtest$o \
	../objects/ncurses$o \
	../objects/newdemo$o \
	../objects/railroad$o \
	../objects/rain$o \
	../objects/tclock$o \
	../objects/testaddch$o \
	../objects/testcurs$o \
	../objects/testscanw$o \
	../objects/view$o \
	../objects/worm$o \
	../objects/xmas$o

$(NORMAL_OBJS) : ../include/ncurses_cfg.h ./test.priv.h

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

../objects/background$o :	$(srcdir)/background.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/background.c -o ../objects/background$o

../objects/blue$o :	$(srcdir)/blue.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/blue.c -o ../objects/blue$o

../objects/bs$o :	$(srcdir)/bs.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/bs.c -o ../objects/bs$o

../objects/cardfile$o :	$(srcdir)/cardfile.c \
			$(HEADER_DEPS) \
			../include/panel.h \
			../include/form.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/cardfile.c -o ../objects/cardfile$o

../objects/color_set$o :	$(srcdir)/color_set.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/color_set.c -o ../objects/color_set$o

../objects/demo_defkey$o :	$(srcdir)/demo_defkey.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/demo_defkey.c -o ../objects/demo_defkey$o

../objects/demo_keyok$o :	$(srcdir)/demo_keyok.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/demo_keyok.c -o ../objects/demo_keyok$o

../objects/demo_forms$o :	$(srcdir)/demo_forms.c \
			$(HEADER_DEPS) \
			../include/form.h \
			$(srcdir)/edit_field.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/demo_forms.c -o ../objects/demo_forms$o

../objects/demo_menus$o :	$(srcdir)/demo_menus.c \
			$(HEADER_DEPS) \
			../include/menu.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/demo_menus.c -o ../objects/demo_menus$o

../objects/demo_panels$o :	$(srcdir)/demo_panels.c \
			$(HEADER_DEPS) \
			../include/panel.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/demo_panels.c -o ../objects/demo_panels$o

../objects/demo_termcap$o :	$(srcdir)/demo_termcap.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/demo_termcap.c -o ../objects/demo_termcap$o

../objects/ditto$o :	$(srcdir)/ditto.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/ditto.c -o ../objects/ditto$o

../objects/dots$o :	$(srcdir)/dots.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/dots.c -o ../objects/dots$o

../objects/edit_field$o :	$(srcdir)/edit_field.c \
			$(HEADER_DEPS) \
			$(srcdir)/edit_field.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/edit_field.c -o ../objects/edit_field$o

../objects/filter$o :	$(srcdir)/filter.c \
			$(HEADER_DEPS) \
			../include/form.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/filter.c -o ../objects/filter$o

../objects/firework$o :	$(srcdir)/firework.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/firework.c -o ../objects/firework$o

../objects/firstlast$o :	$(srcdir)/firstlast.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/firstlast.c -o ../objects/firstlast$o

../objects/gdc$o :	$(srcdir)/gdc.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/gdc.c -o ../objects/gdc$o

../objects/hanoi$o :	$(srcdir)/hanoi.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/hanoi.c -o ../objects/hanoi$o

../objects/hashtest$o :	$(srcdir)/hashtest.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/hashtest.c -o ../objects/hashtest$o

../objects/ins_wide$o :	$(srcdir)/ins_wide.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/ins_wide.c -o ../objects/ins_wide$o

../objects/inserts$o :	$(srcdir)/inserts.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/inserts.c -o ../objects/inserts$o

../objects/keynames$o :	$(srcdir)/keynames.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/keynames.c -o ../objects/keynames$o

../objects/knight$o :	$(srcdir)/knight.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/knight.c -o ../objects/knight$o

../objects/lrtest$o :	$(srcdir)/lrtest.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/lrtest.c -o ../objects/lrtest$o

../objects/ncurses$o :	$(srcdir)/ncurses.c \
			$(HEADER_DEPS) \
			../include/panel.h \
			../include/menu.h \
			../include/form.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/ncurses.c -o ../objects/ncurses$o

../objects/newdemo$o :	$(srcdir)/newdemo.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/newdemo.c -o ../objects/newdemo$o

../objects/railroad$o :	$(srcdir)/railroad.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/railroad.c -o ../objects/railroad$o

../objects/rain$o :	$(srcdir)/rain.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/rain.c -o ../objects/rain$o

../objects/tclock$o :	$(srcdir)/tclock.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/tclock.c -o ../objects/tclock$o

../objects/testaddch$o :	$(srcdir)/testaddch.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/testaddch.c -o ../objects/testaddch$o

../objects/testcurs$o :	$(srcdir)/testcurs.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/testcurs.c -o ../objects/testcurs$o

../objects/testscanw$o :	$(srcdir)/testscanw.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/testscanw.c -o ../objects/testscanw$o

../objects/view$o :	$(srcdir)/view.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/view.c -o ../objects/view$o

../objects/worm$o :	$(srcdir)/worm.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/worm.c -o ../objects/worm$o

../objects/xmas$o :	$(srcdir)/xmas.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../test/xmas.c -o ../objects/xmas$o


# generated by mk-1st.awk (subset=termlib+ext_tinfo+base+ext_funcs)
#  name:          test
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
#  depend:        ../include/ncurses_cfg.h ./test.priv.h
#  host:          i586-pc-beos

DEBUG_OBJS = \
	../obj_g/background$o \
	../obj_g/blue$o \
	../obj_g/bs$o \
	../obj_g/cardfile$o \
	../obj_g/color_set$o \
	../obj_g/demo_defkey$o \
	../obj_g/demo_keyok$o \
	../obj_g/demo_forms$o \
	../obj_g/demo_menus$o \
	../obj_g/demo_panels$o \
	../obj_g/demo_termcap$o \
	../obj_g/ditto$o \
	../obj_g/dots$o \
	../obj_g/edit_field$o \
	../obj_g/filter$o \
	../obj_g/firework$o \
	../obj_g/firstlast$o \
	../obj_g/gdc$o \
	../obj_g/hanoi$o \
	../obj_g/hashtest$o \
	../obj_g/ins_wide$o \
	../obj_g/inserts$o \
	../obj_g/keynames$o \
	../obj_g/knight$o \
	../obj_g/lrtest$o \
	../obj_g/ncurses$o \
	../obj_g/newdemo$o \
	../obj_g/railroad$o \
	../obj_g/rain$o \
	../obj_g/tclock$o \
	../obj_g/testaddch$o \
	../obj_g/testcurs$o \
	../obj_g/testscanw$o \
	../obj_g/view$o \
	../obj_g/worm$o \
	../obj_g/xmas$o

$(DEBUG_OBJS) : ../include/ncurses_cfg.h ./test.priv.h

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

../obj_g/background$o :	$(srcdir)/background.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/background.c -o ../obj_g/background$o

../obj_g/blue$o :	$(srcdir)/blue.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/blue.c -o ../obj_g/blue$o

../obj_g/bs$o :	$(srcdir)/bs.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/bs.c -o ../obj_g/bs$o

../obj_g/cardfile$o :	$(srcdir)/cardfile.c \
			$(HEADER_DEPS) \
			../include/panel.h \
			../include/form.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/cardfile.c -o ../obj_g/cardfile$o

../obj_g/color_set$o :	$(srcdir)/color_set.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/color_set.c -o ../obj_g/color_set$o

../obj_g/demo_defkey$o :	$(srcdir)/demo_defkey.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/demo_defkey.c -o ../obj_g/demo_defkey$o

../obj_g/demo_keyok$o :	$(srcdir)/demo_keyok.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/demo_keyok.c -o ../obj_g/demo_keyok$o

../obj_g/demo_forms$o :	$(srcdir)/demo_forms.c \
			$(HEADER_DEPS) \
			../include/form.h \
			$(srcdir)/edit_field.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/demo_forms.c -o ../obj_g/demo_forms$o

../obj_g/demo_menus$o :	$(srcdir)/demo_menus.c \
			$(HEADER_DEPS) \
			../include/menu.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/demo_menus.c -o ../obj_g/demo_menus$o

../obj_g/demo_panels$o :	$(srcdir)/demo_panels.c \
			$(HEADER_DEPS) \
			../include/panel.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/demo_panels.c -o ../obj_g/demo_panels$o

../obj_g/demo_termcap$o :	$(srcdir)/demo_termcap.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/demo_termcap.c -o ../obj_g/demo_termcap$o

../obj_g/ditto$o :	$(srcdir)/ditto.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/ditto.c -o ../obj_g/ditto$o

../obj_g/dots$o :	$(srcdir)/dots.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/dots.c -o ../obj_g/dots$o

../obj_g/edit_field$o :	$(srcdir)/edit_field.c \
			$(HEADER_DEPS) \
			$(srcdir)/edit_field.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/edit_field.c -o ../obj_g/edit_field$o

../obj_g/filter$o :	$(srcdir)/filter.c \
			$(HEADER_DEPS) \
			../include/form.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/filter.c -o ../obj_g/filter$o

../obj_g/firework$o :	$(srcdir)/firework.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/firework.c -o ../obj_g/firework$o

../obj_g/firstlast$o :	$(srcdir)/firstlast.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/firstlast.c -o ../obj_g/firstlast$o

../obj_g/gdc$o :	$(srcdir)/gdc.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/gdc.c -o ../obj_g/gdc$o

../obj_g/hanoi$o :	$(srcdir)/hanoi.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/hanoi.c -o ../obj_g/hanoi$o

../obj_g/hashtest$o :	$(srcdir)/hashtest.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/hashtest.c -o ../obj_g/hashtest$o

../obj_g/ins_wide$o :	$(srcdir)/ins_wide.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/ins_wide.c -o ../obj_g/ins_wide$o

../obj_g/inserts$o :	$(srcdir)/inserts.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/inserts.c -o ../obj_g/inserts$o

../obj_g/keynames$o :	$(srcdir)/keynames.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/keynames.c -o ../obj_g/keynames$o

../obj_g/knight$o :	$(srcdir)/knight.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/knight.c -o ../obj_g/knight$o

../obj_g/lrtest$o :	$(srcdir)/lrtest.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/lrtest.c -o ../obj_g/lrtest$o

../obj_g/ncurses$o :	$(srcdir)/ncurses.c \
			$(HEADER_DEPS) \
			../include/panel.h \
			../include/menu.h \
			../include/form.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/ncurses.c -o ../obj_g/ncurses$o

../obj_g/newdemo$o :	$(srcdir)/newdemo.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/newdemo.c -o ../obj_g/newdemo$o

../obj_g/railroad$o :	$(srcdir)/railroad.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/railroad.c -o ../obj_g/railroad$o

../obj_g/rain$o :	$(srcdir)/rain.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/rain.c -o ../obj_g/rain$o

../obj_g/tclock$o :	$(srcdir)/tclock.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/tclock.c -o ../obj_g/tclock$o

../obj_g/testaddch$o :	$(srcdir)/testaddch.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/testaddch.c -o ../obj_g/testaddch$o

../obj_g/testcurs$o :	$(srcdir)/testcurs.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/testcurs.c -o ../obj_g/testcurs$o

../obj_g/testscanw$o :	$(srcdir)/testscanw.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/testscanw.c -o ../obj_g/testscanw$o

../obj_g/view$o :	$(srcdir)/view.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/view.c -o ../obj_g/view$o

../obj_g/worm$o :	$(srcdir)/worm.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/worm.c -o ../obj_g/worm$o

../obj_g/xmas$o :	$(srcdir)/xmas.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../test/xmas.c -o ../obj_g/xmas$o

depend : $(AUTO_SRC)
	makedepend -- $(CPPFLAGS) -- $(C_SRC)

# DO NOT DELETE THIS LINE -- make depend depends on it.
