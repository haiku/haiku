/* parser.h -- Everything you wanted to know about the parser, but were
   afraid to ask. */

/* Copyright (C) 1995, 2008,2009 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash.  If not, see <http://www.gnu.org/licenses/>.
*/

#if !defined (_PARSER_H_)
#  define _PARSER_H_

#  include "command.h"
#  include "input.h"

/* Possible states for the parser that require it to do special things. */
#define PST_CASEPAT	0x00001		/* in a case pattern list */
#define PST_ALEXPNEXT	0x00002		/* expand next word for aliases */
#define PST_ALLOWOPNBRC	0x00004		/* allow open brace for function def */
#define PST_NEEDCLOSBRC	0x00008		/* need close brace */
#define PST_DBLPAREN	0x00010		/* double-paren parsing */
#define PST_SUBSHELL	0x00020		/* ( ... ) subshell */
#define PST_CMDSUBST	0x00040		/* $( ... ) command substitution */
#define PST_CASESTMT	0x00080		/* parsing a case statement */
#define PST_CONDCMD	0x00100		/* parsing a [[...]] command */
#define PST_CONDEXPR	0x00200		/* parsing the guts of [[...]] */
#define PST_ARITHFOR	0x00400		/* parsing an arithmetic for command */
#define PST_ALEXPAND	0x00800		/* OK to expand aliases - unused */
#define PST_CMDTOKEN	0x01000		/* command token OK - unused */
#define PST_COMPASSIGN	0x02000		/* parsing x=(...) compound assignment */
#define PST_ASSIGNOK	0x04000		/* assignment statement ok in this context */
#define PST_EOFTOKEN	0x08000		/* yylex checks against shell_eof_token */
#define PST_REGEXP	0x10000		/* parsing an ERE/BRE as a single word */
#define PST_HEREDOC	0x20000		/* reading body of here-document */
#define PST_REPARSE	0x40000		/* re-parsing in parse_string_to_word_list */

/* Definition of the delimiter stack.  Needed by parse.y and bashhist.c. */
struct dstack {
/* DELIMITERS is a stack of the nested delimiters that we have
   encountered so far. */
  char *delimiters;

/* Offset into the stack of delimiters. */
  int delimiter_depth;

/* How many slots are allocated to DELIMITERS. */
  int delimiter_space;
};

#endif /* _PARSER_H_ */
