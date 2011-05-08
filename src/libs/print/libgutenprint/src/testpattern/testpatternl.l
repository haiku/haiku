/*
 * "$Id: testpatternl.l,v 1.26 2010/09/06 17:08:39 rlk Exp $"
 *
 *   Test pattern generator for Gimp-Print
 *
 *   Copyright 2001 Robert Krawitz <rlk@alum.mit.edu>
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

%{

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "testpattern.h"

#define YY_NO_UNPUT

int mylineno = 1;

static char *
c_strstrip(char *s)
{
  size_t len = strlen(s);
  if (len >= 2 && ((s[0] == '"' && s[len - 1] == '"') ||
		   (s[0] == '\'' && s[len - 1] == '\'')))
    {
      char *sn = malloc(len - 1);
      memcpy(sn, s + 1, len - 2);
      sn[len - 2] = '\0';
      return sn;
    }
  else
    return strdup(s);
}

#define DBG(x)						\
do							\
  {							\
    if (getenv("STP_TESTPATTERN_DEBUG"))		\
      fprintf(stderr, "'%s' => %s\n", yytext, #x);	\
  }							\
 while (0)


%}

%option noyywrap

ldigit		[1-9]
digit		[0-9]
odigit		[0-7]
xdigit		[0-9a-fA-F]
integer		([-+]?(0|{ldigit}{digit}*))|(0{odigit}*)|(0[xX]{xdigit}+)
float		[-+]?{digit}+(\.{digit}+)?([eE][-+]?{digit}+)?
string		([\"][^\"]*[\"])|([a-zA-Z0-9_][-a-zA-Z0-9_]*)|([\'][^\']*[\'])
ws		[ \t]+
color		((l_)?(cyan|magenta|yellow|black))|(l_l_black)|(d_yellow)

%%

gamma			DBG(GAMMA); return GAMMA;
level			DBG(LEVEL); return LEVEL;
steps			DBG(STEPS); return STEPS;
ink_limit		DBG(INK_LIMIT); return INK_LIMIT;
printer			DBG(PRINTER); return PRINTER;
parameter		DBG(PARAMETER); return PARAMETER;
parameter_int		DBG(PARAMETER_INT); return PARAMETER_INT;
parameter_bool		DBG(PARAMETER_BOOL); return PARAMETER_BOOL;
parameter_string	DBG(PARAMETER); return PARAMETER;
parameter_float		DBG(PARAMETER_FLOAT); return PARAMETER_FLOAT;
parameter_curve		DBG(PARAMETER_CURVE); return PARAMETER_CURVE;
density			DBG(DENSITY); return DENSITY;
top			DBG(TOP); return TOP;
left			DBG(LEFT); return LEFT;
size_mode		DBG(SIZE_MODE); return SIZE_MODE;
relative		DBG(RELATIVE); return RELATIVE;
pt			DBG(PT); return PT;
in			DBG(IN); return IN;
mm			DBG(MM); return MM;
hsize			DBG(HSIZE); return HSIZE;
vsize			DBG(VSIZE); return VSIZE;
blackline		DBG(BLACKLINE); return BLACKLINE;
pattern			DBG(PATTERN); return PATTERN;
xpattern		DBG(XPATTERN); return XPATTERN;
image			DBG(IMAGE); return IMAGE;
extended		DBG(EXTENDED); return EXTENDED;
grid			DBG(GRID); return GRID;
channel			DBG(CHANNEL); return CHANNEL;
page_size		DBG(PAGESIZE); return PAGESIZE;
;			DBG(SEMI); return SEMI;
mode			DBG(MODE); return MODE;
cmyk			yylval.ival = CMYK;DBG(CMYK); return CMYK;
kcmy			yylval.ival = CMYK;DBG(KCMY); return KCMY;
rgb			yylval.ival = RGB;DBG(RGB); return RGB;
cmy			yylval.ival = RGB;DBG(CMY); return CMY;
grey			yylval.ival = GRAY;DBG(GRAY); return GRAY;
gray			yylval.ival = GRAY;DBG(GRAY); return GRAY;
white			yylval.ival = WHITE;DBG(WHITE); return WHITE;
output			DBG(OUTPUT); return OUTPUT;
message			DBG(MESSAGE); return MESSAGE;
noscale			DBG(NOSCALE); return NOSCALE;
start_job		DBG(START_JOB); return START_JOB;
end_job			DBG(END_JOB); return END_JOB;
end			DBG(END); return END;

{color}			yylval.sval = yytext;DBG(COLOR); return COLOR;
{integer}		yylval.ival = strtol(yytext, NULL, 0); yylval.dval = (double) yylval.ival; DBG(tINT); return tINT;
{float}			yylval.dval = strtod(yytext, NULL); DBG(tDOUBLE); return tDOUBLE;
{string}		yylval.sval = c_strstrip(yytext); DBG(tSTRING); return tSTRING;
{ws}			DBG(whitespace); 	/* Skip blanks/tabs */
#[^\n]*			DBG(comment); 	/* Skip comments */
\n			DBG(newline); mylineno++;
