#
# Simple Makefile for the support programs.
#
# documentation support: man2html
# testing support: printenv recho zecho
#
# bashbug lives here but is created by the top-level makefile
#
# Currently only man2html is built
#
#
# Copyright (C) 1998 Free Software Foundation, Inc.     

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
# Boilerplate
#
topdir = ..
srcdir = .
VPATH = .
BUILD_DIR = /Dano/home/obos/current/src/apps/bin/bash

RM	= rm -f
SHELL	= /bin/sh
CC = gcc

EXEEXT = 

#
# Compiler options:
#
PROFILE_FLAGS = 

CFLAGS = -g -O2
CFLAGS_FOR_BUILD = -g
CPPFLAGS = 
CPPFLAGS_FOR_BUILD = 
LOCAL_CFLAGS = 
DEFS = -DHAVE_CONFIG_H
LOCAL_DEFS = -DSHELL

LOCAL_LDFLAGS = 
LIBS = 
LDFLAGS =  $(LOCAL_LDFLAGS) $(CFLAGS)
LDFLAGS_FOR_BUILD = $(LDFLAGS)

INCLUDES = -I${BUILD_DIR} -I${topdir}

BASE_CCFLAGS = ${PROFILE_FLAGS} $(DEFS) $(LOCAL_DEFS) $(SYSTEM_FLAGS) \
	  ${INCLUDES} $(LOCAL_CFLAGS)

CCFLAGS = $(BASE_CCFLAGS) $(CPPFLAGS) $(CFLAGS)
CCFLAGS_FOR_BUILD = $(BASE_CCFLAGS) $(CPPFLAGS_FOR_BUILD) $(CFLAGS_FOR_BUILD)

SRC1 =	man2html.c
OBJ1 =	man2html.o

.c.o:
	$(RM) $@
	$(CC_FOR_BUILD) -c $(CCFLAGS_FOR_BUILD) $<

all: man2html$(EXEEXT)

man2html$(EXEEXT): $(OBJ1)
	$(CC) $(CCFLAGS) $(OBJ1) -o $@ ${LIBS}

clean:
	rm man2html

man2html.o: man2html.c
