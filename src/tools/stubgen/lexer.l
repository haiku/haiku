%{
/*
 *  FILE: lexer.l
 *  AUTH: Michael John Radwin <mjr@acm.org>
 *
 *  DESC: stubgen lexer.  Portions borrowed from Newcastle
 *  University's Arjuna project (http://arjuna.ncl.ac.uk/), and
 *  Jeff Lee's ANSI Grammar
 *  (ftp://ftp.uu.net/usenet/net.sources/ansi.c.grammar.Z)
 *
 *  DATE: Thu Aug 15 13:10:06 EDT 1996
 *   $Id: lexer.l 10 2002-07-09 12:24:59Z ejakowatz $
 *
 *  Copyright (c) 1996-1998  Michael John Radwin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Modification history:
 *  $Log: lexer.l,v $
 *  Revision 1.1  2002/07/09 12:24:59  ejakowatz
 *  It is accomplished ...
 *
 *  Revision 1.1  2001/11/07 10:06:07  ithamar
 *  Added stubgen to CVS
 *
 *  Revision 1.33  1998/07/27 19:16:57  mradwin
 *  added some c++ keywords
 *  need to handle typename, using, and namespace
 *
 *  Revision 1.32  1998/05/11 19:49:11  mradwin
 *  Version 2.03 (updated copyright information).
 *
 *  Revision 1.31  1998/04/07 23:39:55  mradwin
 *  changed error-handling code significantly.  functions
 *  like count() are now contributing to linebuf so we get correct
 *  parse error messages during lineno == 1 and other situations.
 *  also, instead of calling fatal() for collect*() functions,
 *  we return -1 and let the parser recover more gracefully.
 *
 *  Revision 1.30  1998/01/12 19:39:11  mradwin
 *  modified rcsid
 *
 *  Revision 1.29  1997/11/13 22:37:31  mradwin
 *  changed char[] to char * to make non-gcc compilers
 *  a little happier.  We need to #define const to nothing
 *  for other compilers as well.
 *
 *  Revision 1.28  1997/11/13 21:29:30  mradwin
 *  moved code from parser.y to main.c
 *
 *  Revision 1.27  1997/11/13 21:10:17  mradwin
 *  renamed stubgen.[ly] to parser.y lexer.l
 *
 *  Revision 1.26  1997/11/11 03:52:06  mradwin
 *  changed fatal()
 *
 *  Revision 1.25  1997/11/05 03:02:02  mradwin
 *  Modified logging routines.
 *
 *  Revision 1.24  1997/11/01 23:12:43  mradwin
 *  greatly improved error-recovery.  errors no longer spill over
 *  into other files because the yyerror state is properly reset.
 *
 *  Revision 1.23  1997/10/26 23:16:32  mradwin
 *  changed inform_user and fatal functions to use varargs
 *
 *  Revision 1.22  1997/10/26 22:46:48  mradwin
 *  support macros within comments, etc.
 *
 *  Revision 1.21  1997/10/16 19:42:48  mradwin
 *  added support for elipses, static member/array initializers,
 *  and bitfields.
 *
 *  Revision 1.20  1997/10/16 17:36:06  mradwin
 *  Fixed compiler warning on win32 from <ctype.h> and isspace()
 *
 *  Revision 1.19  1997/10/16 17:12:59  mradwin
 *  handle extern "C" blocks better now, and support multi-line
 *  macros.  still need error-checking.
 *
 *  Revision 1.18  1997/10/15 22:09:06  mradwin
 *  changed tons of names.  stubelem -> sytaxelem,
 *  stubin -> infile, stubout -> outfile, stublog -> logfile.
 *
 *  Revision 1.17  1997/10/15 21:45:13  mradwin
 *  rearranged table.[ch] and util.[ch] so that util pkg
 *  knows nothing about syntaxelems.
 *
 *  Revision 1.16  1997/10/15 17:42:37  mradwin
 *  added support for 'extern "C" { ... }' blocks.
 *
 *  Revision 1.15  1997/09/05 19:17:06  mradwin
 *  works for scanning old versions, except for parameter
 *  names that differ between .H and .C files.
 *
 *  Revision 1.14  1997/09/05 16:37:41  mradwin
 *  rcsid
 *
 *  Revision 1.13  1997/09/05 16:34:36  mradwin
 *  GPL-ized code.
 *
 *  Revision 1.12  1997/09/05 16:13:18  mradwin
 *  changed email address to acm.org
 *
 *  Revision 1.11  1996/09/12 14:44:49  mjr
 *  Added throw decl recognition (great, another 4 bytes in syntaxelem)
 *  and cleaned up the grammar so that const_opt appears in far fewer
 *  places.  const_opt is by default 0 as well, so we don't need to
 *  pass it as an arg to new_elem().
 *
 *  I also added a fix to a potential bug with the MINIT and INLIN
 *  exclusive start states.  I think they could have been confused
 *  by braces within comments, so now I'm grabbing comments in those
 *  states as well.
 *
 *  Revision 1.10  1996/09/12 03:46:10  mjr
 *  No concrete changes in code.  Just added some sanity by
 *  factoring out code into util.[ch] and putting some prototypes
 *  that were in table.h into stubgen.y where they belong.
 *
 *  Revision 1.9  1996/09/01 20:59:48  mjr
 *  Added collectMemberInitList() function, which is similar
 *  to collectInlineDef() and also the exclusive state MINIT
 *
 *  Revision 1.8  1996/08/23 05:09:19  mjr
 *  fixed up some more portability things
 *
 *  Revision 1.7  1996/08/22 02:43:47  mjr
 *  added parse error message (using O'Reilly p. 274)
 *
 *  Revision 1.6  1996/08/21 18:33:50  mjr
 *  removed the buffer for inlines.  we don't care anyway.
 *  now we can't overflow on inlines!
 *
 *  Revision 1.5  1996/08/21 17:40:56  mjr
 *  added some cpp directives for porting to WIN32
 *
 *  Revision 1.4  1996/08/19 17:01:33  mjr
 *  no echo now
 *
 *  Revision 1.3  1996/08/15 21:24:58  mjr
 *  *** empty log message ***
 */
%}

D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			[Ee][+-]?{D}+
FS			(f|F|l|L)
IS			(u|U|l|L)*

%{
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "table.h"
#include "util.h"

#ifdef WIN32
/* definitions of exit, malloc, realloc, and free */
#include <stdlib.h>
#endif

#if 0 /* #ifdef WIN32 */
#include "y_tab.h"
#else
#include "y.tab.h"
#endif

#ifdef __cplusplus
#define STUB_INPUT() yyinput()
#else
#define STUB_INPUT() input()
#endif

/* when we return a string, duplicate it so we can free it later.
   we always allocate memory so we can uniformly free() it. */
#define RETURN_STR(x) tokens_seen++; yylval.string = strdup(yytext); return(x)

/* make that nasty union a value that will bus error if we misinterpret
   the value as a pointer */
#define RETURN_VAL(x) tokens_seen++; yylval.flag = 37; return(x)

static const char rcsid[] = "$Id: lexer.l 10 2002-07-09 12:24:59Z ejakowatz $";

static void count();
static void comment();
static void macro();

char linebuf[1024]; /* null-terminated at beginning of each file */
int lineno;         /* set to 1        at beginning of each file */
int column;         /* set to 0        at beginning of each file */
int tokens_seen;    /* set to 0        at beginning of each file */

%}

%x INLIN MINIT
%%
\n.*			{ /* 
                           * for debugging purposes, we grab an entire
                           * line and buffer it, then grab tokens out of
                           * it.  This lets us have more informative
                           * error messages.  See yyerror() in parser.y
                           */
                          strncpy(linebuf, yytext+1, 1024);
			  lineno++;
			  column = 0;
			  yyless(1); /* give back everything but \n */
			}
"/*"			{ comment(); }

"//".*                  { count(); }
"#"                     { macro(); /* was #.* { count(); } */ }

"static"		{ count(); tokens_seen++; }
"volatile"		{ count(); tokens_seen++; }
"auto"			{ count(); tokens_seen++; }
"extern"		{ count(); RETURN_VAL(EXTERN); }
"register"		{ count(); tokens_seen++; }
"typedef"		{ count(); tokens_seen++; }
"struct"		{ count(); RETURN_VAL(STRUCT); }
"union"			{ count(); RETURN_VAL(UNION); }
"enum"			{ count(); RETURN_VAL(ENUM); }
"const"			{ count(); RETURN_VAL(CONST); }
"template"		{ count(); RETURN_VAL(TEMPLATE); }

"typename"              { count(); tokens_seen++; /* FIXME */ }
"using"                 { count(); tokens_seen++; /* FIXME */ }
"namespace"             { count(); RETURN_VAL(CLASS); /* FIXME */ }

"dllexport"             { count(); tokens_seen++; }
"dllimport"             { count(); tokens_seen++; }

"explicit"              { count(); tokens_seen++; }
"mutable"               { count(); tokens_seen++; }
"inline"                { count(); tokens_seen++; }
"virtual"               { count(); tokens_seen++; }
"class"                 { count(); RETURN_VAL(CLASS); }
"delete"                { count(); RETURN_VAL(DELETE); }
"new"                   { count(); RETURN_VAL(NEW); }
"friend"                { count(); RETURN_VAL(FRIEND); }
"operator"              { count(); RETURN_VAL(OPERATOR); }
"protected"             { count(); RETURN_VAL(PROTECTED); }
"private"               { count(); RETURN_VAL(PRIVATE); }
"public"                { count(); RETURN_VAL(PUBLIC); }
"throw"                 { count(); RETURN_VAL(THROW); }

"char"			{ count(); RETURN_STR(CHAR); }
"short"			{ count(); RETURN_STR(SHORT); }
"int"			{ count(); RETURN_STR(INT); }
"long"			{ count(); RETURN_STR(LONG); }
"signed"		{ count(); RETURN_STR(SIGNED); }
"unsigned"		{ count(); RETURN_STR(UNSIGNED); }
"float"			{ count(); RETURN_STR(FLOAT); }
"double"		{ count(); RETURN_STR(DOUBLE); }
"void"			{ count(); RETURN_STR(VOID); }

{L}({L}|{D})*		{ count(); RETURN_STR(IDENTIFIER); }

0[xX]{H}+{IS}?		{ count(); RETURN_STR(CONSTANT); }
0{D}+{IS}?		{ count(); RETURN_STR(CONSTANT); }
{D}+{IS}?		{ count(); RETURN_STR(CONSTANT); }
'(\\.|[^\\'])+'		{ count(); RETURN_STR(CONSTANT); /* 'fontlck */ }

{D}+{E}{FS}?		{ count(); RETURN_STR(CONSTANT); }
{D}*"."{D}+({E})?{FS}?	{ count(); RETURN_STR(CONSTANT); }
{D}+"."{D}*({E})?{FS}?	{ count(); RETURN_STR(CONSTANT); }

\"(\\.|[^\\"])*\"	{ count(); RETURN_STR(STRING_LITERAL); /* "fontlck */ }

">>="			{ count(); RETURN_VAL(RIGHT_ASSIGN); }
"<<="			{ count(); RETURN_VAL(LEFT_ASSIGN); }
"+="			{ count(); RETURN_VAL(ADD_ASSIGN); }
"-="			{ count(); RETURN_VAL(SUB_ASSIGN); }
"*="			{ count(); RETURN_VAL(MUL_ASSIGN); }
"/="			{ count(); RETURN_VAL(DIV_ASSIGN); }
"%="			{ count(); RETURN_VAL(MOD_ASSIGN); }
"&="			{ count(); RETURN_VAL(AND_ASSIGN); }
"^="			{ count(); RETURN_VAL(XOR_ASSIGN); }
"|="			{ count(); RETURN_VAL(OR_ASSIGN); }
">>"			{ count(); RETURN_VAL(RIGHT_OP); }
"<<"			{ count(); RETURN_VAL(LEFT_OP); }
"++"			{ count(); RETURN_VAL(INC_OP); }
"--"			{ count(); RETURN_VAL(DEC_OP); }
"->"			{ count(); RETURN_VAL(PTR_OP); }
"->*"			{ count(); RETURN_VAL(MEM_PTR_OP); }
"&&"			{ count(); RETURN_VAL(AND_OP); }
"||"			{ count(); RETURN_VAL(OR_OP); }
"<="			{ count(); RETURN_VAL(LE_OP); }
">="			{ count(); RETURN_VAL(GE_OP); }
"=="			{ count(); RETURN_VAL(EQ_OP); }
"!="			{ count(); RETURN_VAL(NE_OP); }
";"			{ count(); RETURN_VAL(';'); }
"{"			{ count(); RETURN_VAL('{'); }
"}"			{ count(); RETURN_VAL('}'); }
","			{ count(); RETURN_VAL(','); }
":"			{ count(); RETURN_VAL(':'); }
"="			{ count(); RETURN_VAL('='); }
"("			{ count(); RETURN_VAL('('); }
")"			{ count(); RETURN_VAL(')'); }
"["			{ count(); RETURN_VAL('['); }
"]"			{ count(); RETURN_VAL(']'); }
"."			{ count(); RETURN_VAL('.'); }
"&"			{ count(); RETURN_VAL('&'); }
"!"			{ count(); RETURN_VAL('!'); }
"~"			{ count(); RETURN_VAL('~'); }
"-"			{ count(); RETURN_VAL('-'); }
"+"			{ count(); RETURN_VAL('+'); }
"*"			{ count(); RETURN_VAL('*'); }
"/"			{ count(); RETURN_VAL('/'); }
"%"			{ count(); RETURN_VAL('%'); }
"<"			{ count(); RETURN_VAL('<'); }
">"			{ count(); RETURN_VAL('>'); }
"^"			{ count(); RETURN_VAL('^'); }
"|"			{ count(); RETURN_VAL('|'); }
"?"			{ count(); RETURN_VAL('?'); }
"::"			{ count(); RETURN_VAL(CLCL); }
"..."			{ count(); RETURN_VAL(ELIPSIS); }

<INLIN>"/*"		{ comment(); }
<INLIN>"//".*           { count(); }
<INLIN>"#"              { macro(); /* was #.* { count(); } */ }
<INLIN>. |
<INLIN>\n               { RETURN_VAL((int) yytext[0]); }

<MINIT>"/*"		{ comment(); }
<MINIT>"//".*           { count(); }
<MINIT>"#"              { macro(); /* was #.* { count(); } */ }
<MINIT>. |
<MINIT>\n               { RETURN_VAL((int) yytext[0]); }

[ \t\v\f]		{ count(); }
.			{ count(); /* ignore bad characters */ }

%%

/*
 * called when EOF is encountered.  Return 1 so the scanner will return
 * the zero token to report end-of-file.
 */
int yywrap()
{
    return(1);
}

static void comment()
{
    int c1 = 0, c2 = STUB_INPUT();
  
    linebuf[column] = c2;
    column++;
    for(;;) {
	if (c2 == EOF)
	    break;
	if (c1 == '*' && c2 == '/')
	    break;
	if (c2 == '\n') {
	    linebuf[0] = '\0';
	    column = 0;
	    lineno++;
	}
	
	c1 = c2;
	c2 = STUB_INPUT();
	linebuf[column] = c2;
	column++;
    }
}


static void macro()
{
  int c1 = 0, c2 = STUB_INPUT(), nonws = 0;
  
  log_printf("MACRO reading begining...\n#");
  log_printf("%c", c2);

  linebuf[column] = c2;
  column++;
  for(;;) {
    if (c2 == EOF)
      break;
    if (!isspace(c1))
      nonws = c1;
    if (nonws == '\\' && c2 == '\n') {
      linebuf[0] = '\0';
      column = 0;
      lineno++;
    } else if (c2 == '\n') {
      linebuf[0] = '\0';
      column = 0;
      lineno++;
      break;
    }
    
    c1 = c2;
    c2 = STUB_INPUT();
    linebuf[column] = c2;
    log_printf("%c", c2);
    column++;
  }
  log_printf("MACRO reading done.\n");
}


static void count()
{
    int i;

    if (lineno == 1)
	strcat(linebuf, yytext);

    for (i = 0; yytext[i] != '\0'; i++)
	if (yytext[i] == '\n')
	    column = 0;
	else if (yytext[i] == '\t')
	    column += 8 - (column % 8);
	else
	    column++;
    
    /* equiv to fprintf(yyout, "%s", yytext); */
    /* ECHO; */
}

/*
 * Collect the contents of inline functions, reading them char by char. 
 * thanks to the arjuna stubgen project for this one
 */
int collectInlineDef()
{
    int bracelevel = 1;
    int token;

    /* the magic of exclusive start states makes it all possible */
    BEGIN INLIN;

    while (bracelevel > 0) {
        token = yylex();
	column++;
/*	fprintf(stderr, "INLIN: read token %c\n", token); */
	if (token > 0) {
	    /* Assume single char */
	    switch (token) {
	    case '{':
		bracelevel++;
		break;
	    case '}':
		bracelevel--;
		if (bracelevel == 0)
		{
		    column--;
		    unput(token);
		    break;
		}
		break;
	    case '\n':
		column = 0;
		lineno++;
		break;
	    }
	} else {
	    /* fatal error: Unexpected EOF reading inline function */
	    return -1;
	}
    }

    /* we now return you to your regularly scheduled start state */
    BEGIN 0;

    return 0;
}


/*
 * hmmm... looks familiar.  more control-y programming.
 */
int collectMemberInitList()
{
    int token;
    int insideList = 1;

    /* the magic of exclusive start states makes it all possible */
    BEGIN MINIT;

    while(insideList) {
        token = yylex();
	column++;
/*	fprintf(stderr, "MINIT: read token %c\n", token); */
	if (token > 0) {
	    /* Assume single char */
	    switch (token)
	    {
	    case '{':
		insideList = 0;
		unput(token);
		break;
	    case '\n':
		column = 0;
		lineno++;
		break;
	    }
	} else {
	    /* fatal error: Unexpected EOF reading member initialization */
	    return -1;
	}
    }

    /* we now return you to your regularly scheduled start state */
    BEGIN 0;

    return 0;
}
