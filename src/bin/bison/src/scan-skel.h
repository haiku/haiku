/* Scan Bison Skeletons.

   Copyright (C) 2005 Free Software Foundation, Inc.

   This file is part of Bison, the GNU Compiler Compiler.

   Bison is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   Bison is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bison; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

void scan_skel (FILE *);

/* Pacify "make syntax-check".  */
extern FILE *skel_in;
extern FILE *skel_out;
extern int skel__flex_debug;
extern int skel_lineno;

/* Pacify "gcc -Wmissing-prototypes" when flex 2.5.31 is used.  */
int skel_get_lineno (void);
FILE *skel_get_in (void);
FILE *skel_get_out (void);
int skel_get_leng (void);
char *skel_get_text (void);
void skel_set_lineno (int);
void skel_set_in (FILE *);
void skel_set_out (FILE *);
int skel_get_debug (void);
void skel_set_debug (int);
int skel_lex_destroy (void);
