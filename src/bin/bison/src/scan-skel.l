/* Scan Bison Skeletons.                                       -*- C -*-

   Copyright (C) 2001, 2002, 2003 Free Software Foundation, Inc.

   This file is part of Bison, the GNU Compiler Compiler.

   Bison is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   Bison is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bison; see the file COPYING.  If not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

%option nodefault noyywrap nounput never-interactive debug
%option prefix="skel_" outfile="lex.yy.c"

%{
#include "system.h"

#include <error.h>
#include <quotearg.h>

#include "getargs.h"
#include "files.h"

int skel_lex (void);

#define QPUTS(String) \
   fputs (quotearg_style (c_quoting_style, (String)), yyout)

%}
%%

%{
  int lineno IF_LINT (= 0);
  char *outname = NULL;
%}

"@output ".*\n {
  char const *filename = yytext + sizeof "@output " - 1;
  yytext[yyleng - 1] = '\0';

  if (*filename == '@')
    {
      if (strcmp (filename, "@output_header_name@") == 0)
	filename = spec_defines_file;
      else if (strcmp (filename, "@output_parser_name@") == 0)
	filename = parser_file_name;
      else
	abort ();
    }

  XFREE (outname);
  outname = xstrdup (filename);
  xfclose (yyout);
  yyout = xfopen (outname, "w");
  lineno = 1;
}

"@@"	   fputc ('@', yyout);
"@{"	   fputc ('[', yyout);
"@}"	   fputc (']', yyout);

"@oline@"  fprintf (yyout, "%d", lineno + 1);
"@ofile@"  QPUTS (outname);
"@output_parser_name@"	QPUTS (parser_file_name);
"@output_header_name@"  QPUTS (spec_defines_file);

"@"	   abort ();
\n	   lineno++; ECHO;
[^@\n]+	   ECHO;

<<EOF>>	   xfclose (yyout); free (outname); return EOF;
%%

/*------------------------.
| Scan a Bison skeleton.  |
`------------------------*/

void scan_skel (FILE *);

void
scan_skel (FILE *in)
{
  skel_in = in;
  skel__flex_debug = trace_flag & trace_skeleton;
  skel_lex ();
  /* Reclaim Flex's buffers.  */
  yy_delete_buffer (YY_CURRENT_BUFFER);
}
