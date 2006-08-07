# $Id: Makefile.in,v 1.96 2005/05/07 18:57:05 tom Exp $
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
# Author: Thomas E. Dickey 1996-2002
#
# Makefile for ncurses source code.
#
# This makes the following:
#	programs
#	includes
#	libraries (normal/debug/profile/shared)
#
# The variable 'srcdir' refers to the source-distribution, and can be set with
# the configure script by "--srcdir=DIR".
#
# The rules are organized to produce the libraries for the configured models,
# and the programs with the configured default model.

# turn off _all_ suffix rules; we'll generate our own
.SUFFIXES:

SHELL		= /bin/sh
THIS		= Makefile

CF_MFLAGS 	= 


x		= 
o		= .o

MODEL		= normal
DESTDIR		= 
top_srcdir	= ..
srcdir		= .
prefix		= /usr/local
exec_prefix	= ${prefix}
bindir		= ${exec_prefix}/bin
libdir		= ${exec_prefix}/lib
includedir	= $(prefix)/include/ncurses
datadir		= ${prefix}/share

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

INCDIR		= $(srcdir)/../include
CPPFLAGS	= -DHAVE_CONFIG_H -I../ncurses -I$(srcdir) -I. -I../include  -D_XOPEN_SOURCE=500 -D_POSIX_C_SOURCE=199506L -DNDEBUG -I$(includedir)

CCFLAGS		= $(CPPFLAGS) $(CFLAGS)

BUILD_CPPFLAGS	= -I../include $(CPPFLAGS)
BUILD_CC	= $(CC)
BUILD_CCFLAGS	= -DHAVE_CONFIG_H -I../ncurses -I$(srcdir) -I$(INCDIR) $(BUILD_CPPFLAGS) $(CFLAGS)
BUILD_LDFLAGS	= $(LDFLAGS)
BUILD_LIBS	= $(LIBS)

# The executables built in this directory are used for generating source that
# is compiled into the build, or are test-programs that are not installed.

BUILD_EXEEXT	= $x
x		= 

CFLAGS_LIBTOOL	= $(CCFLAGS)
CFLAGS_NORMAL	= $(CCFLAGS)
CFLAGS_DEBUG	= $(CCFLAGS) -g -DTRACE
CFLAGS_PROFILE	= $(CCFLAGS) -pg
CFLAGS_SHARED	= $(CCFLAGS) -fPIC

CFLAGS_DEFAULT	= $(CFLAGS_NORMAL)

LINK		= $(LIBTOOL) $(CC)
LDFLAGS		=   

SHLIB_DIRS	= -L../lib
SHLIB_LIST	= $(SHLIB_DIRS) 
TINFO_LIST	= $(SHLIB_DIRS) 

MK_SHARED_LIB	= $(CC) -o $@ -Xlinker -soname=`basename $@` -nostart -e 0

NCURSES_MAJOR	= 5
NCURSES_MINOR	= 5
REL_VERSION	= 5.5
ABI_VERSION	= 5

RANLIB		= ranlib

IMPORT_LIB	= 
SHARED_LIB	= 
LIBRARIES	=  ../lib/libncurses.a ../lib/libncurses_g.a

LINT		= 
LINT_OPTS	= 
LINT_LIBS	= -lncurses 

FALLBACK_LIST	= 

TERMINFO_CAPS	= $(top_srcdir)/include/Caps

AUTO_SRC = \
	../include/nomacros.h \
	codes.c \
	comp_captab.c \
	expanded.c \
	fallback.c \
	init_keytry.h \
	keys.list \
	lib_gen.c \
	lib_keyname.c \
	link_test.c \
	names.c \
	unctrl.c

TEST_DEPS	= ../lib/libncurses.a
TEST_ARGS	= -L../lib -lncurses
TEST_LDFLAGS	=  $(TEST_ARGS)   

TEST_PROGS = \
	captoinfo$x \
	hardscroll$x \
	link_test$x \
	hashmap$x \
	lib_mvcur$x

base	= $(srcdir)/base
serial	= $(srcdir)/tty
tinfo	= $(srcdir)/tinfo
trace	= $(srcdir)/trace
wide	= $(srcdir)/widechar

################################################################################
all \
libs ::		$(AUTO_SRC) ../lib $(LIBRARIES)

sources:	$(AUTO_SRC)

$(DESTDIR)$(bindir) \
$(DESTDIR)$(libdir) :
	sh $(srcdir)/../mkinstalldirs $@

../lib : ; mkdir $@

fallback.c : $(tinfo)/MKfallback.sh
	sh $(tinfo)/MKfallback.sh /usr/local/share/terminfo ${top_srcdir}/misc/terminfo.src $(FALLBACK_LIST) >$@

lib_gen.c : $(base)/MKlib_gen.sh ../include/curses.h
	sh $(base)/MKlib_gen.sh "$(CPP) $(CPPFLAGS)" "$(AWK)" generated <../include/curses.h >$@

../include/nomacros.h : $(base)/MKlib_gen.sh ../include/curses.h
	sh $(base)/MKlib_gen.sh "$(CPP) $(CPPFLAGS)" "$(AWK)" generated <../include/curses.h | \
		fgrep undef >$@

init_keytry.h: make_keys$(BUILD_EXEEXT) keys.list
	./make_keys$(BUILD_EXEEXT) keys.list > $@

keys.list :	$(tinfo)/MKkeys_list.sh
	AWK=$(AWK) sh $(tinfo)/MKkeys_list.sh $(TERMINFO_CAPS) | sort >$@

make_keys$(BUILD_EXEEXT) : \
		$(tinfo)/make_keys.c \
		names.c
	$(BUILD_CC) -o $@ $(BUILD_CCFLAGS) $(tinfo)/make_keys.c $(BUILD_LDFLAGS) $(BUILD_LIBS)

make_hash$(BUILD_EXEEXT) : \
		$(tinfo)/comp_hash.c \
		../include/hashsize.h
	$(BUILD_CC) -o $@ $(BUILD_CCFLAGS) -DMAIN_PROGRAM $(tinfo)/comp_hash.c $(BUILD_LDFLAGS) $(BUILD_LIBS)

expanded.c : $(serial)/MKexpanded.sh
	sh $(serial)/MKexpanded.sh "$(CPP)" $(CPPFLAGS) > $@

comp_captab.c: \
		make_hash$(BUILD_EXEEXT) \
		../include/hashsize.h \
		$(tinfo)/MKcaptab.awk
	sh $(tinfo)/MKcaptab.awk $(AWK) $(srcdir)/../include/Caps > $@

lib_keyname.c: keys.list $(base)/MKkeyname.awk
	$(AWK) -f $(base)/MKkeyname.awk keys.list > $@

names.c codes.c: $(tinfo)/MKnames.awk
	$(AWK) -f $(tinfo)/MKnames.awk $(srcdir)/../include/Caps
	cat namehdr boolnames boolfnames numnames numfnames strnames strfnames nameftr >names.c
	cat namehdr boolcodes numcodes strcodes codeftr >codes.c
	-rm -f namehdr nameftr codeftr boolnames boolfnames boolcodes numnames numfnames numcodes strnames strfnames strcodes

unctrl.c: $(base)/MKunctrl.awk
	echo | $(AWK) -f $(base)/MKunctrl.awk >$@

tags:
	ctags *.[ch] */*.[ch]

TAGS:
	etags *.[ch] */*.[ch]

mostlyclean ::
	-rm -f core tags TAGS *~ *.bak *.i *.ln *.atac trace
	-rm -f $(TEST_PROGS)

clean :: mostlyclean
	-rm -f $(AUTO_SRC)
	-rm -f make_keys$(BUILD_EXEEXT)
	-rm -f make_hash$(BUILD_EXEEXT)
	-rm -rf .libs

distclean :: clean
	-rm -f Makefile

realclean :: distclean

# These rules are used to allow "make -n" to work on a clean directory-tree
../include/hashsize.h \
../include/parametrized.h \
../include/term.h :
	cd ../include; $(MAKE) $(CF_MFLAGS)

# These rules build test-programs for the modules that have test-drivers
test_progs : $(TEST_PROGS)

link_test.c : $(base)/MKlib_gen.sh ../include/curses.h
	sh $(base)/MKlib_gen.sh "$(CPP) $(CPPFLAGS)" "$(AWK)" implemented <../include/curses.h >$@

captoinfo$x : $(tinfo)/captoinfo.c $(TEST_DEPS)
	 $(CC) -o $@ $(CFLAGS_DEFAULT) -DMAIN $(tinfo)/captoinfo.c $(TEST_LDFLAGS)

hardscroll$x : $(serial)/hardscroll.c $(TEST_DEPS)
	 $(CC) -o $@ $(CFLAGS_DEFAULT) -DSCROLLDEBUG $(serial)/hardscroll.c $(TEST_LDFLAGS)

hashmap$x : $(serial)/hashmap.c $(TEST_DEPS)
	 $(CC) -o $@ $(CFLAGS_DEFAULT) -DHASHDEBUG $(serial)/hashmap.c $(TEST_LDFLAGS)

lib_mvcur$x : $(serial)/lib_mvcur.c $(TEST_DEPS) \
		../objects/dump_entry$o
	 $(CC) -o $@ $(CFLAGS_DEFAULT) -DNCURSES_TEST -I$(serial)/../../progs $(serial)/lib_mvcur.c ../objects/dump_entry$o $(TEST_LDFLAGS)

link_test$x : link_test.c $(TEST_DEPS) \
		../objects/link_test$o
	 $(CC) -o $@ $(CFLAGS_DEFAULT) ../objects/link_test$o $(TEST_LDFLAGS)

../objects/dump_entry$o:
	cd ../progs && $(MAKE) ../objects/dump_entry$o

###############################################################################
# The remainder of this file is automatically generated during configuration
###############################################################################

# generated by mk-0th.awk
#   libname:    ncurses
#   subsets:    termlib+ext_tinfo+base+ext_funcs

.SUFFIXES: .c .cc .h .i .ii
.c.i :
	$(CPP) $(CPPFLAGS) $< >$@
.cc.ii :
	$(CPP) $(CPPFLAGS) $< >$@
.h.i :
	$(CPP) $(CPPFLAGS) $< >$@

C_SRC = \
	$(serial)/hardscroll.c \
	$(serial)/hashmap.c \
	$(base)/lib_addch.c \
	$(base)/lib_addstr.c \
	$(base)/lib_beep.c \
	$(base)/lib_bkgd.c \
	$(base)/lib_box.c \
	$(base)/lib_chgat.c \
	$(base)/lib_clear.c \
	$(base)/lib_clearok.c \
	$(base)/lib_clrbot.c \
	$(base)/lib_clreol.c \
	$(base)/lib_color.c \
	$(base)/lib_colorset.c \
	$(base)/lib_delch.c \
	$(base)/lib_delwin.c \
	$(base)/lib_echo.c \
	$(base)/lib_endwin.c \
	$(base)/lib_erase.c \
	$(base)/lib_flash.c \
	./lib_gen.c \
	$(base)/lib_getch.c \
	$(base)/lib_getstr.c \
	$(base)/lib_hline.c \
	$(base)/lib_immedok.c \
	$(base)/lib_inchstr.c \
	$(base)/lib_initscr.c \
	$(base)/lib_insch.c \
	$(base)/lib_insdel.c \
	$(base)/lib_insnstr.c \
	$(base)/lib_instr.c \
	$(base)/lib_isendwin.c \
	$(base)/lib_leaveok.c \
	$(base)/lib_mouse.c \
	$(base)/lib_move.c \
	$(serial)/lib_mvcur.c \
	$(base)/lib_mvwin.c \
	$(base)/lib_newterm.c \
	$(base)/lib_newwin.c \
	$(base)/lib_nl.c \
	$(base)/lib_overlay.c \
	$(base)/lib_pad.c \
	$(base)/lib_printw.c \
	$(base)/lib_redrawln.c \
	$(base)/lib_refresh.c \
	$(base)/lib_restart.c \
	$(base)/lib_scanw.c \
	$(base)/lib_screen.c \
	$(base)/lib_scroll.c \
	$(base)/lib_scrollok.c \
	$(base)/lib_scrreg.c \
	$(base)/lib_set_term.c \
	$(base)/lib_slk.c \
	$(base)/lib_slkatr_set.c \
	$(base)/lib_slkatrof.c \
	$(base)/lib_slkatron.c \
	$(base)/lib_slkatrset.c \
	$(base)/lib_slkattr.c \
	$(base)/lib_slkclear.c \
	$(base)/lib_slkcolor.c \
	$(base)/lib_slkinit.c \
	$(base)/lib_slklab.c \
	$(base)/lib_slkrefr.c \
	$(base)/lib_slkset.c \
	$(base)/lib_slktouch.c \
	$(base)/lib_touch.c \
	$(trace)/lib_tracedmp.c \
	$(trace)/lib_tracemse.c \
	$(serial)/lib_tstp.c \
	$(base)/lib_ungetch.c \
	$(serial)/lib_vidattr.c \
	$(base)/lib_vline.c \
	$(base)/lib_wattroff.c \
	$(base)/lib_wattron.c \
	$(base)/lib_winch.c \
	$(base)/lib_window.c \
	$(base)/nc_panel.c \
	$(base)/safe_sprintf.c \
	$(serial)/tty_update.c \
	$(trace)/varargs.c \
	$(base)/memmove.c \
	$(base)/vsscanf.c \
	$(base)/lib_freeall.c \
	./expanded.c \
	$(base)/lib_dft_fgbg.c \
	$(tinfo)/lib_print.c \
	$(base)/resizeterm.c \
	$(trace)/trace_xnames.c \
	$(base)/wresize.c \
	$(tinfo)/access.c \
	$(tinfo)/add_tries.c \
	$(tinfo)/alloc_entry.c \
	$(tinfo)/alloc_ttype.c \
	$(tinfo)/captoinfo.c \
	./codes.c \
	./comp_captab.c \
	$(tinfo)/comp_error.c \
	$(tinfo)/comp_expand.c \
	$(tinfo)/comp_hash.c \
	$(tinfo)/comp_parse.c \
	$(tinfo)/comp_scan.c \
	$(tinfo)/doalloc.c \
	./fallback.c \
	$(tinfo)/free_ttype.c \
	$(tinfo)/getenv_num.c \
	$(tinfo)/home_terminfo.c \
	$(tinfo)/init_keytry.c \
	$(tinfo)/lib_acs.c \
	$(tinfo)/lib_baudrate.c \
	$(tinfo)/lib_cur_term.c \
	$(tinfo)/lib_data.c \
	$(tinfo)/lib_has_cap.c \
	$(tinfo)/lib_kernel.c \
	./lib_keyname.c \
	$(tinfo)/lib_longname.c \
	$(tinfo)/lib_napms.c \
	$(tinfo)/lib_options.c \
	$(tinfo)/lib_raw.c \
	$(tinfo)/lib_setup.c \
	$(tinfo)/lib_termcap.c \
	$(tinfo)/lib_termname.c \
	$(tinfo)/lib_tgoto.c \
	$(tinfo)/lib_ti.c \
	$(tinfo)/lib_tparm.c \
	$(tinfo)/lib_tputs.c \
	$(trace)/lib_trace.c \
	$(trace)/lib_traceatr.c \
	$(trace)/lib_tracebits.c \
	$(trace)/lib_tracechr.c \
	$(tinfo)/lib_ttyflags.c \
	$(serial)/lib_twait.c \
	$(tinfo)/name_match.c \
	./names.c \
	$(tinfo)/parse_entry.c \
	$(tinfo)/read_entry.c \
	$(tinfo)/read_termcap.c \
	$(tinfo)/setbuf.c \
	$(tinfo)/strings.c \
	$(trace)/trace_buf.c \
	$(trace)/trace_tries.c \
	$(base)/tries.c \
	$(tinfo)/trim_sgr0.c \
	./unctrl.c \
	$(trace)/visbuf.c \
	$(tinfo)/write_entry.c \
	$(base)/define_key.c \
	$(base)/key_defined.c \
	$(base)/keybound.c \
	$(base)/keyok.c \
	$(base)/version.c

# Producing llib-lncurses is time-consuming, so there's no direct-dependency for
# it in the lintlib rule.  We'll only remove in the cleanest setup.
clean ::
	rm -f llib-lncurses.*

realclean ::
	rm -f llib-lncurses

llib-lncurses : $(C_SRC)
	cproto -a -l -DNCURSES_ENABLE_STDBOOL_H=0 -DLINT $(CPPFLAGS) $(C_SRC) >$@

lintlib :
	sh $(srcdir)/../misc/makellib ncurses $(CPPFLAGS)
lint :
	$(LINT) $(LINT_OPTS) $(CPPFLAGS) $(C_SRC) $(LINT_LIBS)

# generated by mk-1st.awk (subset=termlib+ext_tinfo+base+ext_funcs)
#  name:          ncurses
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
#  depend:        ../include/ncurses_cfg.h ./curses.priv.h
#  host:          i586-pc-beos

NORMAL_OBJS = \
	../objects/hardscroll$o \
	../objects/hashmap$o \
	../objects/lib_addch$o \
	../objects/lib_addstr$o \
	../objects/lib_beep$o \
	../objects/lib_bkgd$o \
	../objects/lib_box$o \
	../objects/lib_chgat$o \
	../objects/lib_clear$o \
	../objects/lib_clearok$o \
	../objects/lib_clrbot$o \
	../objects/lib_clreol$o \
	../objects/lib_color$o \
	../objects/lib_colorset$o \
	../objects/lib_delch$o \
	../objects/lib_delwin$o \
	../objects/lib_echo$o \
	../objects/lib_endwin$o \
	../objects/lib_erase$o \
	../objects/lib_flash$o \
	../objects/lib_gen$o \
	../objects/lib_getch$o \
	../objects/lib_getstr$o \
	../objects/lib_hline$o \
	../objects/lib_immedok$o \
	../objects/lib_inchstr$o \
	../objects/lib_initscr$o \
	../objects/lib_insch$o \
	../objects/lib_insdel$o \
	../objects/lib_insnstr$o \
	../objects/lib_instr$o \
	../objects/lib_isendwin$o \
	../objects/lib_leaveok$o \
	../objects/lib_mouse$o \
	../objects/lib_move$o \
	../objects/lib_mvcur$o \
	../objects/lib_mvwin$o \
	../objects/lib_newterm$o \
	../objects/lib_newwin$o \
	../objects/lib_nl$o \
	../objects/lib_overlay$o \
	../objects/lib_pad$o \
	../objects/lib_printw$o \
	../objects/lib_redrawln$o \
	../objects/lib_refresh$o \
	../objects/lib_restart$o \
	../objects/lib_scanw$o \
	../objects/lib_screen$o \
	../objects/lib_scroll$o \
	../objects/lib_scrollok$o \
	../objects/lib_scrreg$o \
	../objects/lib_set_term$o \
	../objects/lib_slk$o \
	../objects/lib_slkatr_set$o \
	../objects/lib_slkatrof$o \
	../objects/lib_slkatron$o \
	../objects/lib_slkatrset$o \
	../objects/lib_slkattr$o \
	../objects/lib_slkclear$o \
	../objects/lib_slkcolor$o \
	../objects/lib_slkinit$o \
	../objects/lib_slklab$o \
	../objects/lib_slkrefr$o \
	../objects/lib_slkset$o \
	../objects/lib_slktouch$o \
	../objects/lib_touch$o \
	../objects/lib_tstp$o \
	../objects/lib_ungetch$o \
	../objects/lib_vidattr$o \
	../objects/lib_vline$o \
	../objects/lib_wattroff$o \
	../objects/lib_wattron$o \
	../objects/lib_winch$o \
	../objects/lib_window$o \
	../objects/nc_panel$o \
	../objects/safe_sprintf$o \
	../objects/tty_update$o \
	../objects/varargs$o \
	../objects/memmove$o \
	../objects/vsscanf$o \
	../objects/lib_freeall$o \
	../objects/expanded$o \
	../objects/lib_dft_fgbg$o \
	../objects/lib_print$o \
	../objects/resizeterm$o \
	../objects/wresize$o \
	../objects/access$o \
	../objects/add_tries$o \
	../objects/alloc_entry$o \
	../objects/alloc_ttype$o \
	../objects/captoinfo$o \
	../objects/codes$o \
	../objects/comp_captab$o \
	../objects/comp_error$o \
	../objects/comp_expand$o \
	../objects/comp_hash$o \
	../objects/comp_parse$o \
	../objects/comp_scan$o \
	../objects/doalloc$o \
	../objects/fallback$o \
	../objects/free_ttype$o \
	../objects/getenv_num$o \
	../objects/home_terminfo$o \
	../objects/init_keytry$o \
	../objects/lib_acs$o \
	../objects/lib_baudrate$o \
	../objects/lib_cur_term$o \
	../objects/lib_data$o \
	../objects/lib_has_cap$o \
	../objects/lib_kernel$o \
	../objects/lib_keyname$o \
	../objects/lib_longname$o \
	../objects/lib_napms$o \
	../objects/lib_options$o \
	../objects/lib_raw$o \
	../objects/lib_setup$o \
	../objects/lib_termcap$o \
	../objects/lib_termname$o \
	../objects/lib_tgoto$o \
	../objects/lib_ti$o \
	../objects/lib_tparm$o \
	../objects/lib_tputs$o \
	../objects/lib_trace$o \
	../objects/lib_ttyflags$o \
	../objects/lib_twait$o \
	../objects/name_match$o \
	../objects/names$o \
	../objects/parse_entry$o \
	../objects/read_entry$o \
	../objects/read_termcap$o \
	../objects/setbuf$o \
	../objects/strings$o \
	../objects/tries$o \
	../objects/trim_sgr0$o \
	../objects/unctrl$o \
	../objects/visbuf$o \
	../objects/write_entry$o \
	../objects/define_key$o \
	../objects/key_defined$o \
	../objects/keybound$o \
	../objects/keyok$o \
	../objects/version$o

$(NORMAL_OBJS) : ../include/ncurses_cfg.h ./curses.priv.h

../lib/libncurses.a : $(NORMAL_OBJS)
	$(AR) $(AR_OPTS) $@ $?
	$(RANLIB) $@

install \
install.libs \
install.ncurses :: $(DESTDIR)$(libdir) ../lib/libncurses.a
	@echo installing ../lib/libncurses.a as $(DESTDIR)$(libdir)/libncurses.a
	$(INSTALL_DATA) ../lib/libncurses.a $(DESTDIR)$(libdir)/libncurses.a
	$(RANLIB) $(DESTDIR)$(libdir)/libncurses.a

uninstall \
uninstall.libs \
uninstall.ncurses ::
	@echo uninstalling $(DESTDIR)$(libdir)/libncurses.a
	-@rm -f $(DESTDIR)$(libdir)/libncurses.a

clean ::
	-rm -f ../lib/libncurses.a

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

../objects/hardscroll$o :	$(serial)/hardscroll.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(serial)/hardscroll.c -o ../objects/hardscroll$o

../objects/hashmap$o :	$(serial)/hashmap.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(serial)/hashmap.c -o ../objects/hashmap$o

../objects/lib_addch$o :	$(base)/lib_addch.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_addch.c -o ../objects/lib_addch$o

../objects/lib_addstr$o :	$(base)/lib_addstr.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_addstr.c -o ../objects/lib_addstr$o

../objects/lib_beep$o :	$(base)/lib_beep.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_beep.c -o ../objects/lib_beep$o

../objects/lib_bkgd$o :	$(base)/lib_bkgd.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_bkgd.c -o ../objects/lib_bkgd$o

../objects/lib_box$o :	$(base)/lib_box.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_box.c -o ../objects/lib_box$o

../objects/lib_chgat$o :	$(base)/lib_chgat.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_chgat.c -o ../objects/lib_chgat$o

../objects/lib_clear$o :	$(base)/lib_clear.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_clear.c -o ../objects/lib_clear$o

../objects/lib_clearok$o :	$(base)/lib_clearok.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_clearok.c -o ../objects/lib_clearok$o

../objects/lib_clrbot$o :	$(base)/lib_clrbot.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_clrbot.c -o ../objects/lib_clrbot$o

../objects/lib_clreol$o :	$(base)/lib_clreol.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_clreol.c -o ../objects/lib_clreol$o

../objects/lib_color$o :	$(base)/lib_color.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_color.c -o ../objects/lib_color$o

../objects/lib_colorset$o :	$(base)/lib_colorset.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_colorset.c -o ../objects/lib_colorset$o

../objects/lib_delch$o :	$(base)/lib_delch.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_delch.c -o ../objects/lib_delch$o

../objects/lib_delwin$o :	$(base)/lib_delwin.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_delwin.c -o ../objects/lib_delwin$o

../objects/lib_echo$o :	$(base)/lib_echo.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_echo.c -o ../objects/lib_echo$o

../objects/lib_endwin$o :	$(base)/lib_endwin.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_endwin.c -o ../objects/lib_endwin$o

../objects/lib_erase$o :	$(base)/lib_erase.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_erase.c -o ../objects/lib_erase$o

../objects/lib_flash$o :	$(base)/lib_flash.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_flash.c -o ../objects/lib_flash$o

../objects/lib_gen$o :	./lib_gen.c \
			../include/curses.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/lib_gen.c -o ../objects/lib_gen$o

../objects/lib_getch$o :	$(base)/lib_getch.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_getch.c -o ../objects/lib_getch$o

../objects/lib_getstr$o :	$(base)/lib_getstr.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_getstr.c -o ../objects/lib_getstr$o

../objects/lib_hline$o :	$(base)/lib_hline.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_hline.c -o ../objects/lib_hline$o

../objects/lib_immedok$o :	$(base)/lib_immedok.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_immedok.c -o ../objects/lib_immedok$o

../objects/lib_inchstr$o :	$(base)/lib_inchstr.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_inchstr.c -o ../objects/lib_inchstr$o

../objects/lib_initscr$o :	$(base)/lib_initscr.c \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_initscr.c -o ../objects/lib_initscr$o

../objects/lib_insch$o :	$(base)/lib_insch.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_insch.c -o ../objects/lib_insch$o

../objects/lib_insdel$o :	$(base)/lib_insdel.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_insdel.c -o ../objects/lib_insdel$o

../objects/lib_insnstr$o :	$(base)/lib_insnstr.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_insnstr.c -o ../objects/lib_insnstr$o

../objects/lib_instr$o :	$(base)/lib_instr.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_instr.c -o ../objects/lib_instr$o

../objects/lib_isendwin$o :	$(base)/lib_isendwin.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_isendwin.c -o ../objects/lib_isendwin$o

../objects/lib_leaveok$o :	$(base)/lib_leaveok.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_leaveok.c -o ../objects/lib_leaveok$o

../objects/lib_mouse$o :	$(base)/lib_mouse.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_mouse.c -o ../objects/lib_mouse$o

../objects/lib_move$o :	$(base)/lib_move.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_move.c -o ../objects/lib_move$o

../objects/lib_mvcur$o :	$(serial)/lib_mvcur.c \
			../include/term.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(serial)/lib_mvcur.c -o ../objects/lib_mvcur$o

../objects/lib_mvwin$o :	$(base)/lib_mvwin.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_mvwin.c -o ../objects/lib_mvwin$o

../objects/lib_newterm$o :	$(base)/lib_newterm.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_newterm.c -o ../objects/lib_newterm$o

../objects/lib_newwin$o :	$(base)/lib_newwin.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_newwin.c -o ../objects/lib_newwin$o

../objects/lib_nl$o :	$(base)/lib_nl.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_nl.c -o ../objects/lib_nl$o

../objects/lib_overlay$o :	$(base)/lib_overlay.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_overlay.c -o ../objects/lib_overlay$o

../objects/lib_pad$o :	$(base)/lib_pad.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_pad.c -o ../objects/lib_pad$o

../objects/lib_printw$o :	$(base)/lib_printw.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_printw.c -o ../objects/lib_printw$o

../objects/lib_redrawln$o :	$(base)/lib_redrawln.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_redrawln.c -o ../objects/lib_redrawln$o

../objects/lib_refresh$o :	$(base)/lib_refresh.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_refresh.c -o ../objects/lib_refresh$o

../objects/lib_restart$o :	$(base)/lib_restart.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_restart.c -o ../objects/lib_restart$o

../objects/lib_scanw$o :	$(base)/lib_scanw.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_scanw.c -o ../objects/lib_scanw$o

../objects/lib_screen$o :	$(base)/lib_screen.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_screen.c -o ../objects/lib_screen$o

../objects/lib_scroll$o :	$(base)/lib_scroll.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_scroll.c -o ../objects/lib_scroll$o

../objects/lib_scrollok$o :	$(base)/lib_scrollok.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_scrollok.c -o ../objects/lib_scrollok$o

../objects/lib_scrreg$o :	$(base)/lib_scrreg.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_scrreg.c -o ../objects/lib_scrreg$o

../objects/lib_set_term$o :	$(base)/lib_set_term.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_set_term.c -o ../objects/lib_set_term$o

../objects/lib_slk$o :	$(base)/lib_slk.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slk.c -o ../objects/lib_slk$o

../objects/lib_slkatr_set$o :	$(base)/lib_slkatr_set.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkatr_set.c -o ../objects/lib_slkatr_set$o

../objects/lib_slkatrof$o :	$(base)/lib_slkatrof.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkatrof.c -o ../objects/lib_slkatrof$o

../objects/lib_slkatron$o :	$(base)/lib_slkatron.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkatron.c -o ../objects/lib_slkatron$o

../objects/lib_slkatrset$o :	$(base)/lib_slkatrset.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkatrset.c -o ../objects/lib_slkatrset$o

../objects/lib_slkattr$o :	$(base)/lib_slkattr.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkattr.c -o ../objects/lib_slkattr$o

../objects/lib_slkclear$o :	$(base)/lib_slkclear.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkclear.c -o ../objects/lib_slkclear$o

../objects/lib_slkcolor$o :	$(base)/lib_slkcolor.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkcolor.c -o ../objects/lib_slkcolor$o

../objects/lib_slkinit$o :	$(base)/lib_slkinit.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkinit.c -o ../objects/lib_slkinit$o

../objects/lib_slklab$o :	$(base)/lib_slklab.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slklab.c -o ../objects/lib_slklab$o

../objects/lib_slkrefr$o :	$(base)/lib_slkrefr.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkrefr.c -o ../objects/lib_slkrefr$o

../objects/lib_slkset$o :	$(base)/lib_slkset.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkset.c -o ../objects/lib_slkset$o

../objects/lib_slktouch$o :	$(base)/lib_slktouch.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slktouch.c -o ../objects/lib_slktouch$o

../objects/lib_touch$o :	$(base)/lib_touch.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_touch.c -o ../objects/lib_touch$o

../objects/lib_tstp$o :	$(serial)/lib_tstp.c \
			$(srcdir)/SigAction.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(serial)/lib_tstp.c -o ../objects/lib_tstp$o

../objects/lib_ungetch$o :	$(base)/lib_ungetch.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_ungetch.c -o ../objects/lib_ungetch$o

../objects/lib_vidattr$o :	$(serial)/lib_vidattr.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(serial)/lib_vidattr.c -o ../objects/lib_vidattr$o

../objects/lib_vline$o :	$(base)/lib_vline.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_vline.c -o ../objects/lib_vline$o

../objects/lib_wattroff$o :	$(base)/lib_wattroff.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_wattroff.c -o ../objects/lib_wattroff$o

../objects/lib_wattron$o :	$(base)/lib_wattron.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_wattron.c -o ../objects/lib_wattron$o

../objects/lib_winch$o :	$(base)/lib_winch.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_winch.c -o ../objects/lib_winch$o

../objects/lib_window$o :	$(base)/lib_window.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_window.c -o ../objects/lib_window$o

../objects/link_test$o :	./link_test.c \
			../include/curses.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/link_test.c -o ../objects/link_test$o

../objects/nc_panel$o :	$(base)/nc_panel.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/nc_panel.c -o ../objects/nc_panel$o

../objects/safe_sprintf$o :	$(base)/safe_sprintf.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/safe_sprintf.c -o ../objects/safe_sprintf$o

../objects/tty_update$o :	$(serial)/tty_update.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(serial)/tty_update.c -o ../objects/tty_update$o

../objects/varargs$o :	$(trace)/varargs.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(trace)/varargs.c -o ../objects/varargs$o

../objects/memmove$o :	$(base)/memmove.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/memmove.c -o ../objects/memmove$o

../objects/vsscanf$o :	$(base)/vsscanf.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/vsscanf.c -o ../objects/vsscanf$o

../objects/lib_freeall$o :	$(base)/lib_freeall.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_freeall.c -o ../objects/lib_freeall$o

../objects/expanded$o :	./expanded.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/expanded.c -o ../objects/expanded$o

../objects/lib_dft_fgbg$o :	$(base)/lib_dft_fgbg.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_dft_fgbg.c -o ../objects/lib_dft_fgbg$o

../objects/lib_print$o :	$(tinfo)/lib_print.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_print.c -o ../objects/lib_print$o

../objects/resizeterm$o :	$(base)/resizeterm.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/resizeterm.c -o ../objects/resizeterm$o

../objects/wresize$o :	$(base)/wresize.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/wresize.c -o ../objects/wresize$o

../objects/access$o :	$(tinfo)/access.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/access.c -o ../objects/access$o

../objects/add_tries$o :	$(tinfo)/add_tries.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/add_tries.c -o ../objects/add_tries$o

../objects/alloc_entry$o :	$(tinfo)/alloc_entry.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/alloc_entry.c -o ../objects/alloc_entry$o

../objects/alloc_ttype$o :	$(tinfo)/alloc_ttype.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/alloc_ttype.c -o ../objects/alloc_ttype$o

../objects/captoinfo$o :	$(tinfo)/captoinfo.c \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/captoinfo.c -o ../objects/captoinfo$o

../objects/codes$o :	./codes.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/codes.c -o ../objects/codes$o

../objects/comp_captab$o :	./comp_captab.c \
			$(INCDIR)/tic.h \
			../include/term.h \
			../include/hashsize.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/comp_captab.c -o ../objects/comp_captab$o

../objects/comp_error$o :	$(tinfo)/comp_error.c \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/comp_error.c -o ../objects/comp_error$o

../objects/comp_expand$o :	$(tinfo)/comp_expand.c \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/comp_expand.c -o ../objects/comp_expand$o

../objects/comp_hash$o :	$(tinfo)/comp_hash.c \
			../include/term.h \
			$(INCDIR)/tic.h \
			../include/hashsize.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/comp_hash.c -o ../objects/comp_hash$o

../objects/comp_parse$o :	$(tinfo)/comp_parse.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/comp_parse.c -o ../objects/comp_parse$o

../objects/comp_scan$o :	$(tinfo)/comp_scan.c \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/comp_scan.c -o ../objects/comp_scan$o

../objects/doalloc$o :	$(tinfo)/doalloc.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/doalloc.c -o ../objects/doalloc$o

../objects/fallback$o :	./fallback.c \
			../include/term.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/fallback.c -o ../objects/fallback$o

../objects/free_ttype$o :	$(tinfo)/free_ttype.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/free_ttype.c -o ../objects/free_ttype$o

../objects/getenv_num$o :	$(tinfo)/getenv_num.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/getenv_num.c -o ../objects/getenv_num$o

../objects/home_terminfo$o :	$(tinfo)/home_terminfo.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/home_terminfo.c -o ../objects/home_terminfo$o

../objects/init_keytry$o :	$(tinfo)/init_keytry.c \
			../include/term.h \
			$(INCDIR)/tic.h \
			init_keytry.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/init_keytry.c -o ../objects/init_keytry$o

../objects/lib_acs$o :	$(tinfo)/lib_acs.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_acs.c -o ../objects/lib_acs$o

../objects/lib_baudrate$o :	$(tinfo)/lib_baudrate.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_baudrate.c -o ../objects/lib_baudrate$o

../objects/lib_cur_term$o :	$(tinfo)/lib_cur_term.c \
			../include/term.h \
			$(INCDIR)/term_entry.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_cur_term.c -o ../objects/lib_cur_term$o

../objects/lib_data$o :	$(tinfo)/lib_data.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_data.c -o ../objects/lib_data$o

../objects/lib_has_cap$o :	$(tinfo)/lib_has_cap.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_has_cap.c -o ../objects/lib_has_cap$o

../objects/lib_kernel$o :	$(tinfo)/lib_kernel.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_kernel.c -o ../objects/lib_kernel$o

../objects/lib_keyname$o :	./lib_keyname.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/lib_keyname.c -o ../objects/lib_keyname$o

../objects/lib_longname$o :	$(tinfo)/lib_longname.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_longname.c -o ../objects/lib_longname$o

../objects/lib_napms$o :	$(tinfo)/lib_napms.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_napms.c -o ../objects/lib_napms$o

../objects/lib_options$o :	$(tinfo)/lib_options.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_options.c -o ../objects/lib_options$o

../objects/lib_raw$o :	$(tinfo)/lib_raw.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_raw.c -o ../objects/lib_raw$o

../objects/lib_setup$o :	$(tinfo)/lib_setup.c \
			../include/term.h \
			$(INCDIR)/term_entry.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_setup.c -o ../objects/lib_setup$o

../objects/lib_termcap$o :	$(tinfo)/lib_termcap.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h \
			$(INCDIR)/capdefaults.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_termcap.c -o ../objects/lib_termcap$o

../objects/lib_termname$o :	$(tinfo)/lib_termname.c \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_termname.c -o ../objects/lib_termname$o

../objects/lib_tgoto$o :	$(tinfo)/lib_tgoto.c \
			../include/term.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_tgoto.c -o ../objects/lib_tgoto$o

../objects/lib_ti$o :	$(tinfo)/lib_ti.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_ti.c -o ../objects/lib_ti$o

../objects/lib_tparm$o :	$(tinfo)/lib_tparm.c \
			../include/term.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_tparm.c -o ../objects/lib_tparm$o

../objects/lib_tputs$o :	$(tinfo)/lib_tputs.c \
			../include/term.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_tputs.c -o ../objects/lib_tputs$o

../objects/lib_trace$o :	$(trace)/lib_trace.c \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(trace)/lib_trace.c -o ../objects/lib_trace$o

../objects/lib_ttyflags$o :	$(tinfo)/lib_ttyflags.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_ttyflags.c -o ../objects/lib_ttyflags$o

../objects/lib_twait$o :	$(serial)/lib_twait.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(serial)/lib_twait.c -o ../objects/lib_twait$o

../objects/name_match$o :	$(tinfo)/name_match.c \
			../include/term.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/name_match.c -o ../objects/name_match$o

../objects/names$o :	./names.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/names.c -o ../objects/names$o

../objects/parse_entry$o :	$(tinfo)/parse_entry.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h \
			../include/parametrized.h \
			$(INCDIR)/capdefaults.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/parse_entry.c -o ../objects/parse_entry$o

../objects/read_entry$o :	$(tinfo)/read_entry.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/read_entry.c -o ../objects/read_entry$o

../objects/read_termcap$o :	$(tinfo)/read_termcap.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/read_termcap.c -o ../objects/read_termcap$o

../objects/setbuf$o :	$(tinfo)/setbuf.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/setbuf.c -o ../objects/setbuf$o

../objects/strings$o :	$(tinfo)/strings.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/strings.c -o ../objects/strings$o

../objects/tries$o :	$(base)/tries.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/tries.c -o ../objects/tries$o

../objects/trim_sgr0$o :	$(tinfo)/trim_sgr0.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/trim_sgr0.c -o ../objects/trim_sgr0$o

../objects/unctrl$o :	./unctrl.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/unctrl.c -o ../objects/unctrl$o

../objects/visbuf$o :	$(trace)/visbuf.c \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(trace)/visbuf.c -o ../objects/visbuf$o

../objects/write_entry$o :	$(tinfo)/write_entry.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/write_entry.c -o ../objects/write_entry$o

../objects/define_key$o :	$(base)/define_key.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/define_key.c -o ../objects/define_key$o

../objects/key_defined$o :	$(base)/key_defined.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/key_defined.c -o ../objects/key_defined$o

../objects/keybound$o :	$(base)/keybound.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/keybound.c -o ../objects/keybound$o

../objects/keyok$o :	$(base)/keyok.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/keyok.c -o ../objects/keyok$o

../objects/version$o :	$(base)/version.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/version.c -o ../objects/version$o


# generated by mk-1st.awk (subset=termlib+ext_tinfo+base+ext_funcs)
#  name:          ncurses
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
#  depend:        ../include/ncurses_cfg.h ./curses.priv.h
#  host:          i586-pc-beos

DEBUG_OBJS = \
	../obj_g/hardscroll$o \
	../obj_g/hashmap$o \
	../obj_g/lib_addch$o \
	../obj_g/lib_addstr$o \
	../obj_g/lib_beep$o \
	../obj_g/lib_bkgd$o \
	../obj_g/lib_box$o \
	../obj_g/lib_chgat$o \
	../obj_g/lib_clear$o \
	../obj_g/lib_clearok$o \
	../obj_g/lib_clrbot$o \
	../obj_g/lib_clreol$o \
	../obj_g/lib_color$o \
	../obj_g/lib_colorset$o \
	../obj_g/lib_delch$o \
	../obj_g/lib_delwin$o \
	../obj_g/lib_echo$o \
	../obj_g/lib_endwin$o \
	../obj_g/lib_erase$o \
	../obj_g/lib_flash$o \
	../obj_g/lib_gen$o \
	../obj_g/lib_getch$o \
	../obj_g/lib_getstr$o \
	../obj_g/lib_hline$o \
	../obj_g/lib_immedok$o \
	../obj_g/lib_inchstr$o \
	../obj_g/lib_initscr$o \
	../obj_g/lib_insch$o \
	../obj_g/lib_insdel$o \
	../obj_g/lib_insnstr$o \
	../obj_g/lib_instr$o \
	../obj_g/lib_isendwin$o \
	../obj_g/lib_leaveok$o \
	../obj_g/lib_mouse$o \
	../obj_g/lib_move$o \
	../obj_g/lib_mvcur$o \
	../obj_g/lib_mvwin$o \
	../obj_g/lib_newterm$o \
	../obj_g/lib_newwin$o \
	../obj_g/lib_nl$o \
	../obj_g/lib_overlay$o \
	../obj_g/lib_pad$o \
	../obj_g/lib_printw$o \
	../obj_g/lib_redrawln$o \
	../obj_g/lib_refresh$o \
	../obj_g/lib_restart$o \
	../obj_g/lib_scanw$o \
	../obj_g/lib_screen$o \
	../obj_g/lib_scroll$o \
	../obj_g/lib_scrollok$o \
	../obj_g/lib_scrreg$o \
	../obj_g/lib_set_term$o \
	../obj_g/lib_slk$o \
	../obj_g/lib_slkatr_set$o \
	../obj_g/lib_slkatrof$o \
	../obj_g/lib_slkatron$o \
	../obj_g/lib_slkatrset$o \
	../obj_g/lib_slkattr$o \
	../obj_g/lib_slkclear$o \
	../obj_g/lib_slkcolor$o \
	../obj_g/lib_slkinit$o \
	../obj_g/lib_slklab$o \
	../obj_g/lib_slkrefr$o \
	../obj_g/lib_slkset$o \
	../obj_g/lib_slktouch$o \
	../obj_g/lib_touch$o \
	../obj_g/lib_tracedmp$o \
	../obj_g/lib_tracemse$o \
	../obj_g/lib_tstp$o \
	../obj_g/lib_ungetch$o \
	../obj_g/lib_vidattr$o \
	../obj_g/lib_vline$o \
	../obj_g/lib_wattroff$o \
	../obj_g/lib_wattron$o \
	../obj_g/lib_winch$o \
	../obj_g/lib_window$o \
	../obj_g/nc_panel$o \
	../obj_g/safe_sprintf$o \
	../obj_g/tty_update$o \
	../obj_g/varargs$o \
	../obj_g/memmove$o \
	../obj_g/vsscanf$o \
	../obj_g/lib_freeall$o \
	../obj_g/expanded$o \
	../obj_g/lib_dft_fgbg$o \
	../obj_g/lib_print$o \
	../obj_g/resizeterm$o \
	../obj_g/trace_xnames$o \
	../obj_g/wresize$o \
	../obj_g/access$o \
	../obj_g/add_tries$o \
	../obj_g/alloc_entry$o \
	../obj_g/alloc_ttype$o \
	../obj_g/captoinfo$o \
	../obj_g/codes$o \
	../obj_g/comp_captab$o \
	../obj_g/comp_error$o \
	../obj_g/comp_expand$o \
	../obj_g/comp_hash$o \
	../obj_g/comp_parse$o \
	../obj_g/comp_scan$o \
	../obj_g/doalloc$o \
	../obj_g/fallback$o \
	../obj_g/free_ttype$o \
	../obj_g/getenv_num$o \
	../obj_g/home_terminfo$o \
	../obj_g/init_keytry$o \
	../obj_g/lib_acs$o \
	../obj_g/lib_baudrate$o \
	../obj_g/lib_cur_term$o \
	../obj_g/lib_data$o \
	../obj_g/lib_has_cap$o \
	../obj_g/lib_kernel$o \
	../obj_g/lib_keyname$o \
	../obj_g/lib_longname$o \
	../obj_g/lib_napms$o \
	../obj_g/lib_options$o \
	../obj_g/lib_raw$o \
	../obj_g/lib_setup$o \
	../obj_g/lib_termcap$o \
	../obj_g/lib_termname$o \
	../obj_g/lib_tgoto$o \
	../obj_g/lib_ti$o \
	../obj_g/lib_tparm$o \
	../obj_g/lib_tputs$o \
	../obj_g/lib_trace$o \
	../obj_g/lib_traceatr$o \
	../obj_g/lib_tracebits$o \
	../obj_g/lib_tracechr$o \
	../obj_g/lib_ttyflags$o \
	../obj_g/lib_twait$o \
	../obj_g/name_match$o \
	../obj_g/names$o \
	../obj_g/parse_entry$o \
	../obj_g/read_entry$o \
	../obj_g/read_termcap$o \
	../obj_g/setbuf$o \
	../obj_g/strings$o \
	../obj_g/trace_buf$o \
	../obj_g/trace_tries$o \
	../obj_g/tries$o \
	../obj_g/trim_sgr0$o \
	../obj_g/unctrl$o \
	../obj_g/visbuf$o \
	../obj_g/write_entry$o \
	../obj_g/define_key$o \
	../obj_g/key_defined$o \
	../obj_g/keybound$o \
	../obj_g/keyok$o \
	../obj_g/version$o

$(DEBUG_OBJS) : ../include/ncurses_cfg.h ./curses.priv.h

../lib/libncurses_g.a : $(DEBUG_OBJS)
	$(AR) $(AR_OPTS) $@ $?
	$(RANLIB) $@

install \
install.libs \
install.ncurses :: $(DESTDIR)$(libdir) ../lib/libncurses_g.a
	@echo installing ../lib/libncurses_g.a as $(DESTDIR)$(libdir)/libncurses_g.a
	$(INSTALL_DATA) ../lib/libncurses_g.a $(DESTDIR)$(libdir)/libncurses_g.a
	$(RANLIB) $(DESTDIR)$(libdir)/libncurses_g.a

uninstall \
uninstall.libs \
uninstall.ncurses ::
	@echo uninstalling $(DESTDIR)$(libdir)/libncurses_g.a
	-@rm -f $(DESTDIR)$(libdir)/libncurses_g.a

clean ::
	-rm -f ../lib/libncurses_g.a

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

../obj_g/hardscroll$o :	$(serial)/hardscroll.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(serial)/hardscroll.c -o ../obj_g/hardscroll$o

../obj_g/hashmap$o :	$(serial)/hashmap.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(serial)/hashmap.c -o ../obj_g/hashmap$o

../obj_g/lib_addch$o :	$(base)/lib_addch.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_addch.c -o ../obj_g/lib_addch$o

../obj_g/lib_addstr$o :	$(base)/lib_addstr.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_addstr.c -o ../obj_g/lib_addstr$o

../obj_g/lib_beep$o :	$(base)/lib_beep.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_beep.c -o ../obj_g/lib_beep$o

../obj_g/lib_bkgd$o :	$(base)/lib_bkgd.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_bkgd.c -o ../obj_g/lib_bkgd$o

../obj_g/lib_box$o :	$(base)/lib_box.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_box.c -o ../obj_g/lib_box$o

../obj_g/lib_chgat$o :	$(base)/lib_chgat.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_chgat.c -o ../obj_g/lib_chgat$o

../obj_g/lib_clear$o :	$(base)/lib_clear.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_clear.c -o ../obj_g/lib_clear$o

../obj_g/lib_clearok$o :	$(base)/lib_clearok.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_clearok.c -o ../obj_g/lib_clearok$o

../obj_g/lib_clrbot$o :	$(base)/lib_clrbot.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_clrbot.c -o ../obj_g/lib_clrbot$o

../obj_g/lib_clreol$o :	$(base)/lib_clreol.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_clreol.c -o ../obj_g/lib_clreol$o

../obj_g/lib_color$o :	$(base)/lib_color.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_color.c -o ../obj_g/lib_color$o

../obj_g/lib_colorset$o :	$(base)/lib_colorset.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_colorset.c -o ../obj_g/lib_colorset$o

../obj_g/lib_delch$o :	$(base)/lib_delch.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_delch.c -o ../obj_g/lib_delch$o

../obj_g/lib_delwin$o :	$(base)/lib_delwin.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_delwin.c -o ../obj_g/lib_delwin$o

../obj_g/lib_echo$o :	$(base)/lib_echo.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_echo.c -o ../obj_g/lib_echo$o

../obj_g/lib_endwin$o :	$(base)/lib_endwin.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_endwin.c -o ../obj_g/lib_endwin$o

../obj_g/lib_erase$o :	$(base)/lib_erase.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_erase.c -o ../obj_g/lib_erase$o

../obj_g/lib_flash$o :	$(base)/lib_flash.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_flash.c -o ../obj_g/lib_flash$o

../obj_g/lib_gen$o :	./lib_gen.c \
			../include/curses.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/lib_gen.c -o ../obj_g/lib_gen$o

../obj_g/lib_getch$o :	$(base)/lib_getch.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_getch.c -o ../obj_g/lib_getch$o

../obj_g/lib_getstr$o :	$(base)/lib_getstr.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_getstr.c -o ../obj_g/lib_getstr$o

../obj_g/lib_hline$o :	$(base)/lib_hline.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_hline.c -o ../obj_g/lib_hline$o

../obj_g/lib_immedok$o :	$(base)/lib_immedok.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_immedok.c -o ../obj_g/lib_immedok$o

../obj_g/lib_inchstr$o :	$(base)/lib_inchstr.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_inchstr.c -o ../obj_g/lib_inchstr$o

../obj_g/lib_initscr$o :	$(base)/lib_initscr.c \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_initscr.c -o ../obj_g/lib_initscr$o

../obj_g/lib_insch$o :	$(base)/lib_insch.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_insch.c -o ../obj_g/lib_insch$o

../obj_g/lib_insdel$o :	$(base)/lib_insdel.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_insdel.c -o ../obj_g/lib_insdel$o

../obj_g/lib_insnstr$o :	$(base)/lib_insnstr.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_insnstr.c -o ../obj_g/lib_insnstr$o

../obj_g/lib_instr$o :	$(base)/lib_instr.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_instr.c -o ../obj_g/lib_instr$o

../obj_g/lib_isendwin$o :	$(base)/lib_isendwin.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_isendwin.c -o ../obj_g/lib_isendwin$o

../obj_g/lib_leaveok$o :	$(base)/lib_leaveok.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_leaveok.c -o ../obj_g/lib_leaveok$o

../obj_g/lib_mouse$o :	$(base)/lib_mouse.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_mouse.c -o ../obj_g/lib_mouse$o

../obj_g/lib_move$o :	$(base)/lib_move.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_move.c -o ../obj_g/lib_move$o

../obj_g/lib_mvcur$o :	$(serial)/lib_mvcur.c \
			../include/term.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(serial)/lib_mvcur.c -o ../obj_g/lib_mvcur$o

../obj_g/lib_mvwin$o :	$(base)/lib_mvwin.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_mvwin.c -o ../obj_g/lib_mvwin$o

../obj_g/lib_newterm$o :	$(base)/lib_newterm.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_newterm.c -o ../obj_g/lib_newterm$o

../obj_g/lib_newwin$o :	$(base)/lib_newwin.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_newwin.c -o ../obj_g/lib_newwin$o

../obj_g/lib_nl$o :	$(base)/lib_nl.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_nl.c -o ../obj_g/lib_nl$o

../obj_g/lib_overlay$o :	$(base)/lib_overlay.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_overlay.c -o ../obj_g/lib_overlay$o

../obj_g/lib_pad$o :	$(base)/lib_pad.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_pad.c -o ../obj_g/lib_pad$o

../obj_g/lib_printw$o :	$(base)/lib_printw.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_printw.c -o ../obj_g/lib_printw$o

../obj_g/lib_redrawln$o :	$(base)/lib_redrawln.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_redrawln.c -o ../obj_g/lib_redrawln$o

../obj_g/lib_refresh$o :	$(base)/lib_refresh.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_refresh.c -o ../obj_g/lib_refresh$o

../obj_g/lib_restart$o :	$(base)/lib_restart.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_restart.c -o ../obj_g/lib_restart$o

../obj_g/lib_scanw$o :	$(base)/lib_scanw.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_scanw.c -o ../obj_g/lib_scanw$o

../obj_g/lib_screen$o :	$(base)/lib_screen.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_screen.c -o ../obj_g/lib_screen$o

../obj_g/lib_scroll$o :	$(base)/lib_scroll.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_scroll.c -o ../obj_g/lib_scroll$o

../obj_g/lib_scrollok$o :	$(base)/lib_scrollok.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_scrollok.c -o ../obj_g/lib_scrollok$o

../obj_g/lib_scrreg$o :	$(base)/lib_scrreg.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_scrreg.c -o ../obj_g/lib_scrreg$o

../obj_g/lib_set_term$o :	$(base)/lib_set_term.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_set_term.c -o ../obj_g/lib_set_term$o

../obj_g/lib_slk$o :	$(base)/lib_slk.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_slk.c -o ../obj_g/lib_slk$o

../obj_g/lib_slkatr_set$o :	$(base)/lib_slkatr_set.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_slkatr_set.c -o ../obj_g/lib_slkatr_set$o

../obj_g/lib_slkatrof$o :	$(base)/lib_slkatrof.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_slkatrof.c -o ../obj_g/lib_slkatrof$o

../obj_g/lib_slkatron$o :	$(base)/lib_slkatron.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_slkatron.c -o ../obj_g/lib_slkatron$o

../obj_g/lib_slkatrset$o :	$(base)/lib_slkatrset.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_slkatrset.c -o ../obj_g/lib_slkatrset$o

../obj_g/lib_slkattr$o :	$(base)/lib_slkattr.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_slkattr.c -o ../obj_g/lib_slkattr$o

../obj_g/lib_slkclear$o :	$(base)/lib_slkclear.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_slkclear.c -o ../obj_g/lib_slkclear$o

../obj_g/lib_slkcolor$o :	$(base)/lib_slkcolor.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_slkcolor.c -o ../obj_g/lib_slkcolor$o

../obj_g/lib_slkinit$o :	$(base)/lib_slkinit.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_slkinit.c -o ../obj_g/lib_slkinit$o

../obj_g/lib_slklab$o :	$(base)/lib_slklab.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_slklab.c -o ../obj_g/lib_slklab$o

../obj_g/lib_slkrefr$o :	$(base)/lib_slkrefr.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_slkrefr.c -o ../obj_g/lib_slkrefr$o

../obj_g/lib_slkset$o :	$(base)/lib_slkset.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_slkset.c -o ../obj_g/lib_slkset$o

../obj_g/lib_slktouch$o :	$(base)/lib_slktouch.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_slktouch.c -o ../obj_g/lib_slktouch$o

../obj_g/lib_touch$o :	$(base)/lib_touch.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_touch.c -o ../obj_g/lib_touch$o

../obj_g/lib_tracedmp$o :	$(trace)/lib_tracedmp.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(trace)/lib_tracedmp.c -o ../obj_g/lib_tracedmp$o

../obj_g/lib_tracemse$o :	$(trace)/lib_tracemse.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(trace)/lib_tracemse.c -o ../obj_g/lib_tracemse$o

../obj_g/lib_tstp$o :	$(serial)/lib_tstp.c \
			$(srcdir)/SigAction.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(serial)/lib_tstp.c -o ../obj_g/lib_tstp$o

../obj_g/lib_ungetch$o :	$(base)/lib_ungetch.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_ungetch.c -o ../obj_g/lib_ungetch$o

../obj_g/lib_vidattr$o :	$(serial)/lib_vidattr.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(serial)/lib_vidattr.c -o ../obj_g/lib_vidattr$o

../obj_g/lib_vline$o :	$(base)/lib_vline.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_vline.c -o ../obj_g/lib_vline$o

../obj_g/lib_wattroff$o :	$(base)/lib_wattroff.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_wattroff.c -o ../obj_g/lib_wattroff$o

../obj_g/lib_wattron$o :	$(base)/lib_wattron.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_wattron.c -o ../obj_g/lib_wattron$o

../obj_g/lib_winch$o :	$(base)/lib_winch.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_winch.c -o ../obj_g/lib_winch$o

../obj_g/lib_window$o :	$(base)/lib_window.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_window.c -o ../obj_g/lib_window$o

../obj_g/link_test$o :	./link_test.c \
			../include/curses.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/link_test.c -o ../obj_g/link_test$o

../obj_g/nc_panel$o :	$(base)/nc_panel.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/nc_panel.c -o ../obj_g/nc_panel$o

../obj_g/safe_sprintf$o :	$(base)/safe_sprintf.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/safe_sprintf.c -o ../obj_g/safe_sprintf$o

../obj_g/tty_update$o :	$(serial)/tty_update.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(serial)/tty_update.c -o ../obj_g/tty_update$o

../obj_g/varargs$o :	$(trace)/varargs.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(trace)/varargs.c -o ../obj_g/varargs$o

../obj_g/memmove$o :	$(base)/memmove.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/memmove.c -o ../obj_g/memmove$o

../obj_g/vsscanf$o :	$(base)/vsscanf.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/vsscanf.c -o ../obj_g/vsscanf$o

../obj_g/lib_freeall$o :	$(base)/lib_freeall.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_freeall.c -o ../obj_g/lib_freeall$o

../obj_g/expanded$o :	./expanded.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/expanded.c -o ../obj_g/expanded$o

../obj_g/lib_dft_fgbg$o :	$(base)/lib_dft_fgbg.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/lib_dft_fgbg.c -o ../obj_g/lib_dft_fgbg$o

../obj_g/lib_print$o :	$(tinfo)/lib_print.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/lib_print.c -o ../obj_g/lib_print$o

../obj_g/resizeterm$o :	$(base)/resizeterm.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/resizeterm.c -o ../obj_g/resizeterm$o

../obj_g/trace_xnames$o :	$(trace)/trace_xnames.c \
			../include/term.h \
			$(INCDIR)/term_entry.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(trace)/trace_xnames.c -o ../obj_g/trace_xnames$o

../obj_g/wresize$o :	$(base)/wresize.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/wresize.c -o ../obj_g/wresize$o

../obj_g/access$o :	$(tinfo)/access.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/access.c -o ../obj_g/access$o

../obj_g/add_tries$o :	$(tinfo)/add_tries.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/add_tries.c -o ../obj_g/add_tries$o

../obj_g/alloc_entry$o :	$(tinfo)/alloc_entry.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/alloc_entry.c -o ../obj_g/alloc_entry$o

../obj_g/alloc_ttype$o :	$(tinfo)/alloc_ttype.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/alloc_ttype.c -o ../obj_g/alloc_ttype$o

../obj_g/captoinfo$o :	$(tinfo)/captoinfo.c \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/captoinfo.c -o ../obj_g/captoinfo$o

../obj_g/codes$o :	./codes.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/codes.c -o ../obj_g/codes$o

../obj_g/comp_captab$o :	./comp_captab.c \
			$(INCDIR)/tic.h \
			../include/term.h \
			../include/hashsize.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/comp_captab.c -o ../obj_g/comp_captab$o

../obj_g/comp_error$o :	$(tinfo)/comp_error.c \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/comp_error.c -o ../obj_g/comp_error$o

../obj_g/comp_expand$o :	$(tinfo)/comp_expand.c \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/comp_expand.c -o ../obj_g/comp_expand$o

../obj_g/comp_hash$o :	$(tinfo)/comp_hash.c \
			../include/term.h \
			$(INCDIR)/tic.h \
			../include/hashsize.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/comp_hash.c -o ../obj_g/comp_hash$o

../obj_g/comp_parse$o :	$(tinfo)/comp_parse.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/comp_parse.c -o ../obj_g/comp_parse$o

../obj_g/comp_scan$o :	$(tinfo)/comp_scan.c \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/comp_scan.c -o ../obj_g/comp_scan$o

../obj_g/doalloc$o :	$(tinfo)/doalloc.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/doalloc.c -o ../obj_g/doalloc$o

../obj_g/fallback$o :	./fallback.c \
			../include/term.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/fallback.c -o ../obj_g/fallback$o

../obj_g/free_ttype$o :	$(tinfo)/free_ttype.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/free_ttype.c -o ../obj_g/free_ttype$o

../obj_g/getenv_num$o :	$(tinfo)/getenv_num.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/getenv_num.c -o ../obj_g/getenv_num$o

../obj_g/home_terminfo$o :	$(tinfo)/home_terminfo.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/home_terminfo.c -o ../obj_g/home_terminfo$o

../obj_g/init_keytry$o :	$(tinfo)/init_keytry.c \
			../include/term.h \
			$(INCDIR)/tic.h \
			init_keytry.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/init_keytry.c -o ../obj_g/init_keytry$o

../obj_g/lib_acs$o :	$(tinfo)/lib_acs.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/lib_acs.c -o ../obj_g/lib_acs$o

../obj_g/lib_baudrate$o :	$(tinfo)/lib_baudrate.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/lib_baudrate.c -o ../obj_g/lib_baudrate$o

../obj_g/lib_cur_term$o :	$(tinfo)/lib_cur_term.c \
			../include/term.h \
			$(INCDIR)/term_entry.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/lib_cur_term.c -o ../obj_g/lib_cur_term$o

../obj_g/lib_data$o :	$(tinfo)/lib_data.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/lib_data.c -o ../obj_g/lib_data$o

../obj_g/lib_has_cap$o :	$(tinfo)/lib_has_cap.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/lib_has_cap.c -o ../obj_g/lib_has_cap$o

../obj_g/lib_kernel$o :	$(tinfo)/lib_kernel.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/lib_kernel.c -o ../obj_g/lib_kernel$o

../obj_g/lib_keyname$o :	./lib_keyname.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/lib_keyname.c -o ../obj_g/lib_keyname$o

../obj_g/lib_longname$o :	$(tinfo)/lib_longname.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/lib_longname.c -o ../obj_g/lib_longname$o

../obj_g/lib_napms$o :	$(tinfo)/lib_napms.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/lib_napms.c -o ../obj_g/lib_napms$o

../obj_g/lib_options$o :	$(tinfo)/lib_options.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/lib_options.c -o ../obj_g/lib_options$o

../obj_g/lib_raw$o :	$(tinfo)/lib_raw.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/lib_raw.c -o ../obj_g/lib_raw$o

../obj_g/lib_setup$o :	$(tinfo)/lib_setup.c \
			../include/term.h \
			$(INCDIR)/term_entry.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/lib_setup.c -o ../obj_g/lib_setup$o

../obj_g/lib_termcap$o :	$(tinfo)/lib_termcap.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h \
			$(INCDIR)/capdefaults.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/lib_termcap.c -o ../obj_g/lib_termcap$o

../obj_g/lib_termname$o :	$(tinfo)/lib_termname.c \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/lib_termname.c -o ../obj_g/lib_termname$o

../obj_g/lib_tgoto$o :	$(tinfo)/lib_tgoto.c \
			../include/term.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/lib_tgoto.c -o ../obj_g/lib_tgoto$o

../obj_g/lib_ti$o :	$(tinfo)/lib_ti.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/lib_ti.c -o ../obj_g/lib_ti$o

../obj_g/lib_tparm$o :	$(tinfo)/lib_tparm.c \
			../include/term.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/lib_tparm.c -o ../obj_g/lib_tparm$o

../obj_g/lib_tputs$o :	$(tinfo)/lib_tputs.c \
			../include/term.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/lib_tputs.c -o ../obj_g/lib_tputs$o

../obj_g/lib_trace$o :	$(trace)/lib_trace.c \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(trace)/lib_trace.c -o ../obj_g/lib_trace$o

../obj_g/lib_traceatr$o :	$(trace)/lib_traceatr.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(trace)/lib_traceatr.c -o ../obj_g/lib_traceatr$o

../obj_g/lib_tracebits$o :	$(trace)/lib_tracebits.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(trace)/lib_tracebits.c -o ../obj_g/lib_tracebits$o

../obj_g/lib_tracechr$o :	$(trace)/lib_tracechr.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(trace)/lib_tracechr.c -o ../obj_g/lib_tracechr$o

../obj_g/lib_ttyflags$o :	$(tinfo)/lib_ttyflags.c \
			../include/term.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/lib_ttyflags.c -o ../obj_g/lib_ttyflags$o

../obj_g/lib_twait$o :	$(serial)/lib_twait.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(serial)/lib_twait.c -o ../obj_g/lib_twait$o

../obj_g/name_match$o :	$(tinfo)/name_match.c \
			../include/term.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/name_match.c -o ../obj_g/name_match$o

../obj_g/names$o :	./names.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/names.c -o ../obj_g/names$o

../obj_g/parse_entry$o :	$(tinfo)/parse_entry.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h \
			../include/parametrized.h \
			$(INCDIR)/capdefaults.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/parse_entry.c -o ../obj_g/parse_entry$o

../obj_g/read_entry$o :	$(tinfo)/read_entry.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/read_entry.c -o ../obj_g/read_entry$o

../obj_g/read_termcap$o :	$(tinfo)/read_termcap.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/read_termcap.c -o ../obj_g/read_termcap$o

../obj_g/setbuf$o :	$(tinfo)/setbuf.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/setbuf.c -o ../obj_g/setbuf$o

../obj_g/strings$o :	$(tinfo)/strings.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/strings.c -o ../obj_g/strings$o

../obj_g/trace_buf$o :	$(trace)/trace_buf.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(trace)/trace_buf.c -o ../obj_g/trace_buf$o

../obj_g/trace_tries$o :	$(trace)/trace_tries.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(trace)/trace_tries.c -o ../obj_g/trace_tries$o

../obj_g/tries$o :	$(base)/tries.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/tries.c -o ../obj_g/tries$o

../obj_g/trim_sgr0$o :	$(tinfo)/trim_sgr0.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/trim_sgr0.c -o ../obj_g/trim_sgr0$o

../obj_g/unctrl$o :	./unctrl.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/unctrl.c -o ../obj_g/unctrl$o

../obj_g/visbuf$o :	$(trace)/visbuf.c \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(trace)/visbuf.c -o ../obj_g/visbuf$o

../obj_g/write_entry$o :	$(tinfo)/write_entry.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(tinfo)/write_entry.c -o ../obj_g/write_entry$o

../obj_g/define_key$o :	$(base)/define_key.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/define_key.c -o ../obj_g/define_key$o

../obj_g/key_defined$o :	$(base)/key_defined.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/key_defined.c -o ../obj_g/key_defined$o

../obj_g/keybound$o :	$(base)/keybound.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/keybound.c -o ../obj_g/keybound$o

../obj_g/keyok$o :	$(base)/keyok.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/keyok.c -o ../obj_g/keyok$o

../obj_g/version$o :	$(base)/version.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_DEBUG) -c ../ncurses/$(base)/version.c -o ../obj_g/version$o

depend : $(AUTO_SRC)
	makedepend -- $(CPPFLAGS) -- $(C_SRC)

# DO NOT DELETE THIS LINE -- make depend depends on it.
