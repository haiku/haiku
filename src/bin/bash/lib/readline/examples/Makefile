# This is the Makefile for the examples subdirectory of readline. -*- text -*-
#
# Copyright (C) 1994 Free Software Foundation, Inc.

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

EXECUTABLES = fileman rltest rl
CFLAGS  = -g -I../.. -I.. -DREADLINE_LIBRARY
LDFLAGS = -g -L..

.c.o:
	$(CC) $(CFLAGS) -c $<

all: $(EXECUTABLES)


rl: rl.o
	$(CC) $(LDFLAGS) -o $@ rl.o -lreadline -ltermcap

fileman: fileman.o
	$(CC) $(LDFLAGS) -o $@ fileman.o -lreadline -ltermcap

rltest: rltest.o
	$(CC) $(LDFLAGS) -o $@ rltest.o -lreadline -ltermcap

rlcat: rlcat.o
	$(CC) $(LDFLAGS) -o $@ rlcat.o -lreadline -ltermcap

fileman.o: fileman.c
rltest.o: rltest.c
rl.o: rl.c
rlcat.o: rlcat.c
