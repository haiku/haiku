/* File names and variables for bison,
   Copyright (C) 1984, 1989, 2000, 2001, 2002 Free Software Foundation, Inc.

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef FILES_H_
# define FILES_H_

# include "uniqstr.h"

/* File name specified with -o for the output file, or 0 if no -o.  */
extern char *spec_outfile;

/* File name for the parser (i.e., the one above, or its default.) */
extern char *parser_file_name;

/* Symbol prefix specified with -p, or 0 if no -p.  */
extern const char *spec_name_prefix;

/* File name prefix specified with -b, or 0 if no -b.  */
extern char *spec_file_prefix;

/* --verbose. */
extern char *spec_verbose_file;

/* File name specified for the output VCG graph.  */
extern char *spec_graph_file;

/* File name specified with --defines.  */
extern char *spec_defines_file;

/* Read grammar specifications. */
extern FILE *finput;


/* If semantic parser, output a .h file that defines YYSTYPE... */
extern struct obstack pre_prologue_obstack;
extern struct obstack post_prologue_obstack;

/* The file name as given on the command line.
   Not named "input_file" because Flex uses this name for an argument,
   and therefore GCC warns about a name clash. */
extern uniqstr grammar_file;

/* The current file name.  Might change with %include, or with #line.  */
extern uniqstr current_file;

void compute_output_file_names (void);

FILE *xfopen (const char *name, const char *mode);
void xfclose (FILE *ptr);

/* Prefix used to generate output file names.  */
extern char *short_base_name;

#endif /* !FILES_H_ */
