# This Makefile for building libbuiltins.a is in -*- text -*- for Emacs.
#
# Copyright (C) 1996 Free Software Foundation, Inc.     

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA.

#
SHELL = /bin/sh
RANLIB = ranlib
CC = gcc
CC_FOR_BUILD = $(CC)
AR = ar
ARFLAGS = cr
RM = rm -f
CP = cp

EXEEXT = 

prefix = /usr/local

srcdir = .
VPATH = .
topdir = ..
includedir = ${prefix}/include
datadir = ${prefix}/share

# Support an alternate destination root directory for package building
DESTDIR =

INSTALL = /bin/install -c
INSTALL_DATA = ${INSTALL} -m 644
BUILD_DIR = /Dano/home/obos/current/src/apps/bin/bash

PROFILE_FLAGS = 
CFLAGS = -g -O2
CFLAGS_FOR_BUILD = -g
CPPFLAGS = 
CPPFLAGS_FOR_BUILD = 
LOCAL_CFLAGS =  ${DEBUG}
DEFS = -DHAVE_CONFIG_H
LOCAL_DEFS = -DSHELL

LIBS = 
LDFLAGS =  $(LOCAL_LDFLAGS) $(CFLAGS)
LDFLAGS_FOR_BUILD = $(LDFLAGS)
LOCAL_LDFLAGS = 
#LIBS_FOR_BUILD = @LIBS_FOR_BUILD@
LIBS_FOR_BUILD = $(LIBS)

BASHINCDIR = ${topdir}/include

RL_INCLUDEDIR = 

HELPDIR = 
MKDIRS = ${topdir}/support/mkdirs

INCLUDES = -I. -I..  -I$(topdir) -I$(BASHINCDIR) -I$(topdir)/lib -I$(srcdir)

BASE_CCFLAGS = ${PROFILE_FLAGS} $(DEFS) $(LOCAL_DEFS) $(SYSTEM_FLAGS) \
	 ${INCLUDES} $(LOCAL_CFLAGS)

CCFLAGS = $(BASE_CCFLAGS) $(CPPFLAGS) $(CFLAGS)

CCFLAGS_FOR_BUILD = $(BASE_CCFLAGS) $(CPPFLAGS_FOR_BUILD) $(CFLAGS_FOR_BUILD)

GCC_LINT_FLAGS = -Wall -Wshadow -Wpointer-arith -Wcast-qual \
		 -Wcast-align -Wstrict-prototypes -Wconversion \
		 -Wmissing-prototypes -Wtraditional -Wredundant-decls -pedantic

MKBUILTINS = mkbuiltins$(EXEEXT)
DIRECTDEFINE = -D $(srcdir)
HELPDIRDEFINE = 

# xxx this is bad style
RL_LIBSRC = $(topdir)/lib/readline

.SUFFIXES:
.SUFFIXES: .def .c .o
# How to make a .o file from a .def file.
.def.o:
	$(RM) $@
	./$(MKBUILTINS) $(DIRECTDEFINE) $<
	$(CC) -c $(CCFLAGS) $*.c || ( $(RM) $*.c ; exit 1 )
#	$(RM) $*.c

# How to make a .c file from a .def file.
.def.c:
	$(RM) $@
	./$(MKBUILTINS) $(DIRECTDEFINE) $<

# default rule for making a .o file from a .c file
.c.o:
	$(RM) $@
	$(CC) -c $(CCFLAGS) $<

DEFSRC =  $(srcdir)/alias.def $(srcdir)/bind.def $(srcdir)/break.def \
	  $(srcdir)/builtin.def $(srcdir)/cd.def $(srcdir)/colon.def \
	  $(srcdir)/command.def $(srcdir)/declare.def $(srcdir)/echo.def \
	  $(srcdir)/enable.def $(srcdir)/eval.def $(srcdir)/getopts.def \
	  $(srcdir)/exec.def $(srcdir)/exit.def $(srcdir)/fc.def \
	  $(srcdir)/fg_bg.def $(srcdir)/hash.def $(srcdir)/help.def \
	  $(srcdir)/history.def $(srcdir)/jobs.def $(srcdir)/kill.def \
	  $(srcdir)/let.def $(srcdir)/read.def $(srcdir)/return.def \
	  $(srcdir)/set.def $(srcdir)/setattr.def $(srcdir)/shift.def \
	  $(srcdir)/source.def $(srcdir)/suspend.def $(srcdir)/test.def \
	  $(srcdir)/times.def $(srcdir)/trap.def $(srcdir)/type.def \
	  $(srcdir)/ulimit.def $(srcdir)/umask.def $(srcdir)/wait.def \
	  $(srcdir)/reserved.def $(srcdir)/pushd.def $(srcdir)/shopt.def \
	  $(srcdir)/printf.def $(srcdir)/complete.def

STATIC_SOURCE = common.c evalstring.c evalfile.c getopt.c bashgetopt.c \
		getopt.h 

OFILES = builtins.o \
	alias.o bind.o break.o builtin.o cd.o colon.o command.o \
	common.o declare.o echo.o enable.o eval.o evalfile.o \
	evalstring.o exec.o \
	exit.o fc.o fg_bg.o hash.o help.o history.o jobs.o kill.o let.o \
	pushd.o read.o return.o set.o setattr.o shift.o source.o \
	suspend.o test.o times.o trap.o type.o ulimit.o umask.o \
	wait.o getopts.o shopt.o printf.o getopt.o bashgetopt.o complete.o

CREATED_FILES = builtext.h builtins.c psize.aux pipesize.h

all: $(MKBUILTINS) libbuiltins.a

libbuiltins.a: $(MKBUILTINS) $(OFILES) builtins.o
	$(RM) $@
	$(AR) $(ARFLAGS) $@ $(OFILES)
	-$(RANLIB) $@

builtext.h builtins.c: $(MKBUILTINS) $(DEFSRC)
	@-if test -f builtins.c; then mv -f builtins.c old-builtins.c; fi
	@-if test -f builtext.h; then mv -f builtext.h old-builtext.h; fi
	./$(MKBUILTINS) -externfile builtext.h -structfile builtins.c \
	    -noproduction $(DIRECTDEFINE) $(HELPDIRDEFINE) $(DEFSRC)
	@-if cmp -s old-builtext.h builtext.h 2>/dev/null; then \
		mv old-builtext.h builtext.h; \
	 else \
		$(RM) old-builtext.h; \
	 fi
	@-if cmp -s old-builtins.c builtins.c 2>/dev/null; then \
		mv old-builtins.c builtins.c; \
	 else \
		$(RM) old-builtins.c; \
	 fi

helpdoc:	$(MKBUILTINS) $(DEFSRC)
	./$(MKBUILTINS) ${HELPDIRDEFINE} -noproduction $(DIRECTDEFINE) $(DEFSRC)

install-help:
	@-if test -n "${HELPDIR}" && test -d helpfiles ; then \
		test -d ${HELPDIR} || ${SHELL} ${MKDIRS} $(DESTDIR)$(HELPDIR) ;\
		( cd helpfiles ; \
		  for f in *; do \
			echo installing $$f; \
			${INSTALL_DATA} $$f $(DESTDIR)$(HELPDIR); \
		  done; ) ; \
	fi

install:	

mkbuiltins.o: ../config.h
mkbuiltins.o: mkbuiltins.c
	$(RM) $@
	$(CC_FOR_BUILD) -c $(CCFLAGS_FOR_BUILD) $<

mkbuiltins$(EXEEXT): mkbuiltins.o
	$(CC_FOR_BUILD) $(LDFLAGS_FOR_BUILD) -o $(MKBUILTINS) mkbuiltins.o $(LIBS_FOR_BUILD)

# rules for deficient makes, like SunOS
mkbuiltins.o: mkbuiltins.c
builtins.o: builtins.c
common.o: common.c
bashgetopt.o: bashgetopt.c
getopt.o: getopt.c
evalstring.o: evalstring.c          
evalfile.o: evalfile.c  

ulimit.o: pipesize.h

pipesize.h:	psize.aux
	$(SHELL) $(srcdir)/psize.sh > $@

psize.aux:	psize.c
	$(CC_FOR_BUILD) $(CCFLAGS_FOR_BUILD) -o $@ $(srcdir)/psize.c

documentation: builtins.texi

builtins.texi: $(MKBUILTINS)
	./$(MKBUILTINS) -documentonly $(DEFSRC)

clean:
	$(RM) $(OFILES) $(CREATED_FILES) $(MKBUILTINS) mkbuiltins.o libbuiltins.a
	-test -d helpfiles && $(RM) -r helpfiles

mostlyclean: 
	$(RM) $(OFILES) libbuiltins.a

distclean maintainer-clean: clean
	$(RM) Makefile

$(OFILES):	$(MKBUILTINS) ../config.h

alias.o: alias.def
bind.o: bind.def
break.o: break.def
builtin.o: builtin.def
cd.o: cd.def
colon.o: colon.def
command.o: command.def
declare.o: declare.def
echo.o: echo.def
enable.o: enable.def
eval.o: eval.def
exec.o: exec.def
exit.o: exit.def
fc.o: fc.def
fg_bg.o: fg_bg.def
hash.o: hash.def
help.o: help.def
history.o: history.def
jobs.o: jobs.def
kill.o: kill.def
let.o: let.def
printf.o: printf.def
pushd.o: pushd.def
read.o: read.def
return.o: return.def
set.o: set.def
setattr.o: setattr.def
shift.o: shift.def
shopt.o: shopt.def
source.o: source.def
suspend.o: suspend.def
test.o: test.def
times.o: times.def
trap.o: trap.def
type.o: type.def
ulimit.o: ulimit.def
umask.o: umask.def
wait.o: wait.def
getopts.o: getopts.def
reserved.o: reserved.def
complete.o: complete.def

# C files
bashgetopt.o: ../config.h $(topdir)/bashansi.h $(BASHINCDIR)/ansi_stdlib.h
bashgetopt.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/bashjmp.h
bashgetopt.o: $(topdir)/command.h $(topdir)/general.h $(topdir)/xmalloc.h $(topdir)/error.h
bashgetopt.o: $(topdir)/variables.h $(topdir)/conftypes.h $(topdir)/quit.h $(BASHINCDIR)/maxpath.h
bashgetopt.o: $(topdir)/unwind_prot.h $(topdir)/dispose_cmd.h
bashgetopt.o: $(topdir)/make_cmd.h $(topdir)/subst.h $(topdir)/sig.h
bashgetopt.o: $(topdir)/pathnames.h $(topdir)/externs.h $(srcdir)/common.h
bashgetopt.o: $(BASHINCDIR)/chartypes.h
common.o: $(topdir)/bashtypes.h $(BASHINCDIR)/posixstat.h $(topdir)/bashansi.h $(BASHINCDIR)/ansi_stdlib.h
common.o: $(topdir)/shell.h $(topdir)/syntax.h ../config.h $(topdir)/bashjmp.h $(BASHINCDIR)/posixjmp.h
common.o: $(topdir)/sig.h $(topdir)/command.h
common.o: $(topdir)/general.h $(topdir)/xmalloc.h $(BASHINCDIR)/stdc.h $(BASHINCDIR)/memalloc.h
common.o: $(topdir)/variables.h $(topdir)/conftypes.h $(topdir)/input.h
common.o: $(topdir)/siglist.h $(topdir)/bashhist.h $(topdir)/quit.h
common.o: $(topdir)/unwind_prot.h $(BASHINCDIR)/maxpath.h $(topdir)/jobs.h
common.o: $(topdir)/builtins.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
common.o: $(topdir)/subst.h $(topdir)/execute_cmd.h $(topdir)/error.h
common.o: $(topdir)/externs.h $(topdir)/pathnames.h ./builtext.h
common.o: $(BASHINCDIR)/chartypes.h
evalfile.o: $(topdir)/bashtypes.h $(BASHINCDIR)/posixstat.h ${BASHINCDIR}/filecntl.h
evalfile.o: $(topdir)/bashansi.h $(BASHINCDIR)/ansi_stdlib.h
evalfile.o: $(topdir)/shell.h $(topdir)/syntax.h ../config.h $(topdir)/bashjmp.h
evalfile.o: $(topdir)/command.h $(topdir)/general.h $(topdir)/xmalloc.h $(topdir)/error.h
evalfile.o: $(topdir)/variables.h $(topdir)/conftypes.h $(topdir)/quit.h $(BASHINCDIR)/maxpath.h
evalfile.o: $(topdir)/unwind_prot.h $(topdir)/dispose_cmd.h
evalfile.o: $(topdir)/make_cmd.h $(topdir)/subst.h $(topdir)/sig.h
evalfile.o: $(topdir)/pathnames.h $(topdir)/externs.h
evalfile.o: $(topdir)/jobs.h $(topdir)/builtins.h $(topdir)/flags.h
evalfile.o: $(topdir)/input.h $(topdir)/execute_cmd.h
evalfile.o: $(topdir)/bashhist.h $(srcdir)/common.h
evalstring.o: ../config.h $(topdir)/bashansi.h $(BASHINCDIR)/ansi_stdlib.h
evalstring.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/bashjmp.h $(BASHINCDIR)/posixjmp.h
evalstring.o: $(topdir)/sig.h $(topdir)/command.h $(topdir)/siglist.h
evalstring.o: $(BASHINCDIR)/memalloc.h $(topdir)/variables.h $(topdir)/conftypes.h $(topdir)/input.h
evalstring.o: $(topdir)/quit.h $(topdir)/unwind_prot.h
evalstring.o: $(BASHINCDIR)/maxpath.h $(topdir)/jobs.h $(topdir)/builtins.h
evalstring.o: $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h $(topdir)/subst.h
evalstring.o: $(topdir)/externs.h $(topdir)/jobs.h $(topdir)/builtins.h
evalstring.o: $(topdir)/flags.h $(topdir)/input.h $(topdir)/execute_cmd.h
evalstring.o: $(topdir)/bashhist.h $(srcdir)/common.h
evalstring.o: $(topdir)/trap.h $(topdir)/redir.h
getopt.o: ../config.h $(BASHINCDIR)/memalloc.h
getopt.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/bashjmp.h $(topdir)/command.h
getopt.o: $(topdir)/general.h $(topdir)/xmalloc.h $(topdir)/error.h $(topdir)/variables.h $(topdir)/conftypes.h
getopt.o: $(topdir)/quit.h $(BASHINCDIR)/maxpath.h $(topdir)/unwind_prot.h
getopt.o: $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h $(topdir)/subst.h
getopt.o: $(topdir)/sig.h $(topdir)/pathnames.h $(topdir)/externs.h
getopt.o: $(srcdir)/getopt.h
mkbuiltins.o: ../config.h $(topdir)/bashtypes.h $(BASHINCDIR)/posixstat.h
mkbuiltins.o: ${BASHINCDIR}/filecntl.h
mkbuiltins.o: $(topdir)/bashansi.h $(BASHINCDIR)/ansi_stdlib.h

# def files
alias.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
alias.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h $(BASHINCDIR)/maxpath.h
alias.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
alias.o: $(topdir)/subst.h $(topdir)/externs.h $(srcdir)/common.h
alias.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
bind.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h $(topdir)/error.h
bind.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
bind.o: $(topdir)/subst.h $(topdir)/externs.h $(srcdir)/bashgetopt.h
bind.o: $(topdir)/general.h $(topdir)/xmalloc.h $(BASHINCDIR)/maxpath.h $(topdir)/bashline.h
bind.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h 
break.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h 
break.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h
break.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
break.o: $(topdir)/subst.h $(topdir)/externs.h $(BASHINCDIR)/maxpath.h
break.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
builtin.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
builtin.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h $(topdir)/externs.h
builtin.o: $(topdir)/quit.h $(srcdir)/common.h $(BASHINCDIR)/maxpath.h
builtin.o: $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h $(topdir)/subst.h
builtin.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h 
builtin.o: $(srcdir)/bashgetopt.h
cd.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h $(topdir)/error.h
cd.o: $(topdir)/general.h $(topdir)/xmalloc.h $(topdir)/quit.h $(topdir)/dispose_cmd.h
cd.o: $(topdir)/make_cmd.h $(topdir)/subst.h $(topdir)/externs.h
cd.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
cd.o: $(srcdir)/common.h $(BASHINCDIR)/maxpath.h
command.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
command.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h $(topdir)/externs.h
command.o: $(topdir)/quit.h $(srcdir)/bashgetopt.h $(BASHINCDIR)/maxpath.h
command.o: $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h $(topdir)/subst.h
command.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h 
declare.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
declare.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h
declare.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
declare.o: $(topdir)/subst.h $(topdir)/externs.h $(BASHINCDIR)/maxpath.h
declare.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
declare.o: $(topdir)/arrayfunc.h $(srcdir)/bashgetopt.h
echo.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h $(topdir)/error.h
echo.o: $(topdir)/general.h $(topdir)/xmalloc.h $(topdir)/subst.h $(topdir)/externs.h
echo.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
echo.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
echo.o: $(BASHINCDIR)/maxpath.h
enable.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
enable.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h
enable.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
enable.o: $(topdir)/subst.h $(topdir)/externs.h
enable.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
enable.o: $(BASHINCDIR)/maxpath.h
enable.o: $(topdir)/pcomplete.h
eval.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
eval.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h
eval.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
eval.o: $(topdir)/subst.h $(topdir)/externs.h
eval.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
eval.o: $(BASHINCDIR)/maxpath.h
exec.o: $(topdir)/bashtypes.h
exec.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
exec.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h
exec.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
exec.o: $(topdir)/subst.h $(topdir)/externs.h $(topdir)/flags.h
exec.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
exec.o: $(srcdir)/common.h $(topdir)/execute_cmd.h $(BASHINCDIR)/maxpath.h
exec.o: $(topdir)/findcmd.h
exit.o: $(topdir)/bashtypes.h
exit.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
exit.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h
exit.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
exit.o: $(topdir)/subst.h $(topdir)/externs.h
exit.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
exit.o: $(BASHINCDIR)/maxpath.h ./builtext.h
fc.o: $(topdir)/bashtypes.h $(BASHINCDIR)/posixstat.h
fc.o: $(topdir)/builtins.h $(topdir)/command.h $(srcdir)/bashgetopt.h
fc.o: $(topdir)/bashhist.h
fc.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h $(topdir)/error.h
fc.o: $(topdir)/general.h $(topdir)/xmalloc.h $(BASHINCDIR)/maxpath.h
fc.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
fc.o: $(topdir)/subst.h $(topdir)/externs.h $(topdir)/shell.h $(topdir)/syntax.h
fc.o: $(topdir)/flags.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
fc.o: $(topdir)/bashansi.h $(BASHINCDIR)/ansi_stdlib.h $(BASHINCDIR)/chartypes.h
fg_bg.o: $(topdir)/bashtypes.h $(srcdir)/bashgetopt.h
fg_bg.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
fg_bg.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h
fg_bg.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
fg_bg.o: $(topdir)/subst.h $(topdir)/externs.h $(BASHINCDIR)/maxpath.h
fg_bg.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
getopts.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
getopts.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h
getopts.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
getopts.o: $(topdir)/subst.h $(topdir)/externs.h $(BASHINCDIR)/maxpath.h
getopts.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
hash.o: $(topdir)/builtins.h $(topdir)/command.h $(topdir)/quit.h
hash.o: $(topdir)/findcmd.h $(topdir)/hashlib.h
hash.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
hash.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h
hash.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
hash.o: $(srcdir)/common.h $(BASHINCDIR)/maxpath.h
help.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
help.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h
help.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
help.o: $(topdir)/subst.h $(topdir)/externs.h $(BASHINCDIR)/maxpath.h
help.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
help.o: ${srcdir}/common.h
history.o: $(topdir)/bashtypes.h
history.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
history.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h
history.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
history.o: $(topdir)/subst.h $(topdir)/externs.h
history.o: ${BASHINCDIR}/filecntl.h $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h
history.o: $(topdir)/variables.h $(topdir)/conftypes.h $(topdir)/bashhist.h $(BASHINCDIR)/maxpath.h
inlib.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
inlib.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h
inlib.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
inlib.o: $(BASHINCDIR)/maxpath.h $(topdir)/subst.h $(topdir)/externs.h
inlib.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
jobs.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h $(topdir)/error.h
jobs.o: $(topdir)/general.h $(topdir)/xmalloc.h $(topdir)/quit.h $(srcdir)/bashgetopt.h
jobs.o: $(BASHINCDIR)/maxpath.h $(topdir)/externs.h
jobs.o: $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h $(topdir)/subst.h
jobs.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h 
kill.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h $(topdir)/error.h
kill.o: $(topdir)/general.h $(topdir)/xmalloc.h $(topdir)/subst.h $(topdir)/externs.h
kill.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
kill.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/trap.h $(topdir)/unwind_prot.h
kill.o: $(topdir)/variables.h $(topdir)/conftypes.h $(BASHINCDIR)/maxpath.h
let.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
let.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h
let.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
let.o: $(topdir)/subst.h $(topdir)/externs.h $(BASHINCDIR)/maxpath.h
let.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
printf.o: ../config.h $(BASHINCDIR)/memalloc.h $(topdir)/bashjmp.h
printf.o: $(topdir)/command.h $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h
printf.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
printf.o: $(topdir)/subst.h $(topdir)/externs.h $(topdir)/sig.h
printf.o: $(topdir)/pathnames.h $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h
printf.o: $(topdir)/variables.h $(topdir)/conftypes.h $(BASHINCDIR)/stdc.h $(srcdir)/bashgetopt.h
printf.o: $(topdir)/bashtypes.h ${srcdir}/common.h $(BASHINCDIR)/chartypes.h
pushd.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
pushd.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h
pushd.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
pushd.o: $(topdir)/subst.h $(topdir)/externs.h
pushd.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
pushd.o: $(BASHINCDIR)/maxpath.h $(srcdir)/common.h ./builtext.h
read.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
read.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h
read.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
read.o: $(topdir)/subst.h $(topdir)/externs.h $(BASHINCDIR)/maxpath.h
read.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
read.o: $(BASHINCDIR)/shtty.h
read.o: $(topdir)/arrayfunc.h
return.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
return.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h
return.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
return.o: $(topdir)/subst.h $(topdir)/externs.h $(BASHINCDIR)/maxpath.h
return.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
set.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
set.o: $(topdir)/general.h $(topdir)/xmalloc.h $(topdir)/subst.h $(topdir)/externs.h
set.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
set.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
set.o: $(BASHINCDIR)/maxpath.h $(topdir)/error.h
set.o: $(topdir)/arrayfunc.h
setattr.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
setattr.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h $(BASHINCDIR)/maxpath.h
setattr.o: $(topdir)/quit.h $(srcdir)/common.h $(srcdir)/bashgetopt.h
setattr.o: $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h $(topdir)/subst.h
setattr.o: $(topdir)/externs.h
setattr.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h 
setattr.o: $(topdir)/arrayfunc.h
shift.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
shift.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h
shift.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
shift.o: $(topdir)/subst.h $(topdir)/externs.h $(BASHINCDIR)/maxpath.h
shift.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
source.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
source.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h $(topdir)/findcmd.h
source.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
source.o: $(topdir)/subst.h $(topdir)/externs.h $(BASHINCDIR)/maxpath.h
source.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
source.o: $(srcdir)/bashgetopt.h
suspend.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
suspend.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h
suspend.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
suspend.o: $(topdir)/subst.h $(topdir)/externs.h $(BASHINCDIR)/maxpath.h
suspend.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
test.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
test.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h
test.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
test.o: $(topdir)/subst.h $(topdir)/externs.h $(BASHINCDIR)/maxpath.h
test.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
test.o: $(topdir)/test.h
times.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
times.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h
times.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
times.o: $(topdir)/subst.h $(topdir)/externs.h $(BASHINCDIR)/maxpath.h
times.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
times.o: $(BASHINCDIR)/posixtime.h
trap.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
trap.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h $(topdir)/externs.h
trap.o: $(topdir)/quit.h $(srcdir)/common.h $(BASHINCDIR)/maxpath.h
trap.o: $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h $(topdir)/subst.h
trap.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
trap.o: $(topdir)/findcmd.h
type.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
type.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h
type.o: $(topdir)/quit.h $(srcdir)/common.h $(BASHINCDIR)/maxpath.h
type.o: $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h $(topdir)/subst.h
type.o: $(topdir)/externs.h $(topdir)/hashcmd.h
type.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h 
ulimit.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
ulimit.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h
ulimit.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
ulimit.o: $(topdir)/subst.h $(topdir)/externs.h $(BASHINCDIR)/maxpath.h
ulimit.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
umask.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
umask.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h
umask.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
umask.o: $(topdir)/subst.h $(topdir)/externs.h $(BASHINCDIR)/maxpath.h
umask.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
umask.o: $(BASHINCDIR)/chartypes.h
wait.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
wait.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h
wait.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
wait.o: $(topdir)/subst.h $(topdir)/externs.h $(BASHINCDIR)/maxpath.h
wait.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
wait.o: $(BASHINCDIR)/chartypes.h
shopt.o: $(topdir)/command.h ../config.h $(BASHINCDIR)/memalloc.h
shopt.o: $(topdir)/error.h $(topdir)/general.h $(topdir)/xmalloc.h 
shopt.o: $(topdir)/quit.h $(topdir)/dispose_cmd.h $(topdir)/make_cmd.h
shopt.o: $(topdir)/subst.h $(topdir)/externs.h $(BASHINCDIR)/maxpath.h
shopt.o: $(topdir)/shell.h $(topdir)/syntax.h $(topdir)/unwind_prot.h $(topdir)/variables.h $(topdir)/conftypes.h
shopt.o: $(srcdir)/common.h $(srcdir)/bashgetopt.h

complete.o: ../config.h
complete.o: ${topdir}/shell.h $(topdir)/syntax.h ${topdir}/bashjmp.h ${BASHINCDIR}/posixjmp.h ${topdir}/sig.h  
complete.o: ${topdir}/unwind_prot.h ${topdir}/variables.h
complete.o: ${topdir}/bashtypes.h ${topdir}/bashansi.h ${BASHINCDIR}/ansi_stdlib.h
complete.o: ${topdir}/builtins.h
complete.o: ${topdir}/pcomplete.h
complete.o: ${srcdir}/common.h ${srcdir}/bashgetopt.h

#bind.o: $(RL_LIBSRC)chardefs.h $(RL_LIBSRC)readline.h $(RL_LIBSRC)keymaps.h
