/*
 * Copyright (c) 2003 Matthijs Hollemans
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

%{
//------------------------------------------------------------------------------

#include <stack>
#include <string.h>

#include "rdef.h"
#include "compile.h"
#include "private.h"
#include "parser.hpp"

#define LEXERROR(msg) abort_compile(RDEF_COMPILE_ERR, msg);

// Initial size (and increment) of lexbuf.
#define LEX_BUF_SIZE  (64*1024)

// Temporary buffer that the lexer uses to parse string and raw literals. 
// The buffer will grow if necessary, to accommodate large data blocks.
static uint8* lexbuf;

static uint8* lexptr;   // current write position in lexbuf
static size_t lexsize;  // current size of the lex buffer
static size_t lexcnt;   // how full lexbuf currently is

static void resetbuf();     // resets lexptr and lexcnt
static void addbuf(uint8);  // appends byte to lexbuf

// When we encounter an #include directive, we push the current 
// buffer, filename, and line number on the include stack, so we 
// can resume lexing that file when we're done with the include.
struct include_t {
	YY_BUFFER_STATE buffer;
	char* filename;
	int lineno;
};

static std::stack<include_t> include_stack;

static void open_include();
static void close_include();

//------------------------------------------------------------------------------
%}

%option noyywrap
%option yylineno

LETTER      [a-zA-Z]
BIN         [01]
OCT         [0-7]
DEC         [0-9]
HEX         [0-9a-fA-F]
IDENT       [a-zA-Z_][a-zA-Z0-9_]*
WSPACE      [ \r\t\n\f]
EXP         [eE][+-]?{DEC}+

%x COMMENT
%x STRDATA
%x RAWDATA
%x INCLUDE

%%

enum                     return ENUM;
resource                 return RESOURCE;
array                    return ARRAY;
message                  return MESSAGE;
archive                  return ARCHIVE;
type                     return RTYPE;
import                   return IMPORT;

false                    yylval.b = false; return BOOL;
true                     yylval.b = true;  return BOOL;

0[xX]{HEX}{1,16}         { yylval.i = strtoull(yytext + 2, NULL, 16);
                           return INTEGER; }
0{OCT}{1,24}             { yylval.i = strtoull(yytext, NULL, 8);
                           return INTEGER; }
0[bB]{BIN}{1,64}         { yylval.i = strtoull(yytext + 2, NULL, 2);
                           return INTEGER; }
{DEC}+                   { yylval.i = strtoull(yytext, NULL, 10);
                           return INTEGER; }
'....'                   { yylval.i = (yytext[1] << 24)
                                    | (yytext[2] << 16)
                                    | (yytext[3] << 8)
                                    |  yytext[4];
                           return INTEGER; }

{DEC}+{EXP}              yylval.f = strtod(yytext, NULL); return FLOAT;
{DEC}*\.{DEC}+{EXP}?     yylval.f = strtod(yytext, NULL); return FLOAT;
{DEC}+\.{DEC}*{EXP}?     yylval.f = strtod(yytext, NULL); return FLOAT;

#{DEC}+                  { yylval.t = strtoul(yytext + 1, NULL, 10);
                           return TYPECODE; }
#0[xX]{HEX}{1,8}         { yylval.t = strtoul(yytext + 3, NULL, 16);
                           return TYPECODE; }
#'....'                  { yylval.t = (yytext[2] << 24)
                                    | (yytext[3] << 16)
                                    | (yytext[4] << 8)
                                    |  yytext[5];
                           return TYPECODE; }

{IDENT}                  { yylval.I = (char*) alloc_mem(yyleng + 1);
                           memcpy(yylval.I, yytext, yyleng + 1);
                           return IDENT; }

\"                       BEGIN(STRDATA); resetbuf();
<STRDATA>\"{WSPACE}+\"   /* concatenate two literals */
<STRDATA>\"              { BEGIN(INITIAL); 
                           addbuf('\0'); 
                           yylval.d.type = get_type("string");
                           yylval.d.size = lexcnt;
                           yylval.d.ptr  = alloc_mem(lexcnt);
                           memcpy(yylval.d.ptr, lexbuf, lexcnt); 
                           return STRING; }
<STRDATA>\n              LEXERROR("string not terminated")
<STRDATA>\\{OCT}{3}      addbuf(strtol(yytext + 1, NULL, 8));
<STRDATA>\\0[xX]{HEX}{2} addbuf(strtol(yytext + 3, NULL, 16));
<STRDATA>\\[xX]{HEX}{2}  addbuf(strtol(yytext + 2, NULL, 16));
<STRDATA>\\b             addbuf('\b');
<STRDATA>\\f             addbuf('\f');
<STRDATA>\\n             addbuf('\n');
<STRDATA>\\r             addbuf('\r');
<STRDATA>\\t             addbuf('\t');
<STRDATA>\\v             addbuf('\v');
<STRDATA>\\0             addbuf('\0');
<STRDATA>\\.             addbuf(yytext[1]);
<STRDATA>.               addbuf(yytext[0]);

$\"                      BEGIN(RAWDATA); resetbuf();
<RAWDATA>\"{WSPACE}+$\"  /* concatenate two literals */
<RAWDATA>\"              { BEGIN(INITIAL); 
                           yylval.d.type = get_type("raw");
                           yylval.d.size = lexcnt;
                           yylval.d.ptr  = alloc_mem(lexcnt);
                           memcpy(yylval.d.ptr, lexbuf, lexcnt); 
                           return RAW; }
<RAWDATA>\n              LEXERROR("raw data not terminated")
<RAWDATA>{HEX}{2}        addbuf(strtol(yytext, NULL, 16));
<RAWDATA>{HEX}           LEXERROR("number of characters must be even")
<RAWDATA>.               LEXERROR("invalid character in raw data")

"/*"                     BEGIN(COMMENT);  /* eat multi-line comment */
<COMMENT>[^*\n]*         /* eat anything that is not a '*' */
<COMMENT>"*"+[^*/\n]*    /* eat up '*'s not followed by '/'s */
<COMMENT>\n
<COMMENT>"*"+"/"         BEGIN(INITIAL);
<COMMENT><<EOF>>         LEXERROR("forgot to close /*..*/ comment")

"//"[^\n]*               /* eat single-line comment */
{WSPACE}+                /* eat whitespace */

\#include[ \t]+\"        BEGIN(INCLUDE);
<INCLUDE>[ \t]*          /* eat the whitespace */
<INCLUDE>[^ \t\n\"]+\"   open_include();
<INCLUDE>\n              LEXERROR("error in include statement")
<INCLUDE><<EOF>>         LEXERROR("error in include statement")
<<EOF>>                  { if (include_stack.empty())
                               yyterminate();
                           else
                               close_include(); }

.                        return yytext[0];

%%
//------------------------------------------------------------------------------

void
resetbuf()
{
	lexptr = lexbuf;
	lexcnt = 0;
}


void
addbuf(uint8 b)
{
	if (lexcnt == lexsize) {
		lexsize += LEX_BUF_SIZE;
		lexbuf = (uint8*) realloc(lexbuf, lexsize);
		if (lexbuf == NULL)
			abort_compile(B_NO_MEMORY, "out of memory");

		lexptr = lexbuf + lexcnt;
	}

	*lexptr++ = b;
	++lexcnt;
}


void
open_include()
{
	yytext[yyleng - 1] = '\0';  // remove trailing " quote

	char tmpname[B_PATH_NAME_LENGTH];
	if (open_file_from_include_dir(yytext, tmpname)) {
		yyin = fopen(tmpname, "r");
		if (yyin != NULL) {
			include_t incl;
			incl.buffer   = YY_CURRENT_BUFFER;
			incl.lineno   = yylineno;
			incl.filename = strdup(lexfile);
			include_stack.push(incl);

			strcpy(lexfile, tmpname);
			yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));
			yylineno = 1;

			BEGIN(INITIAL);
			return;
		}
	}

	abort_compile(RDEF_COMPILE_ERR, "cannot open include %s", yytext);
}


void
close_include()
{
	fclose(yyin);
	yy_delete_buffer(YY_CURRENT_BUFFER);

	include_t incl = include_stack.top();
	include_stack.pop();

	yy_switch_to_buffer(incl.buffer);
	yylineno = incl.lineno;
	strcpy(lexfile, incl.filename);

	free(incl.filename);
}


void
init_lexer()
{
	lexsize = LEX_BUF_SIZE;
	lexbuf = (uint8*) malloc(lexsize);
	if (lexbuf == NULL)
		abort_compile(B_NO_MEMORY, "out of memory");

	yyrestart(yyin);  // necessary for multiple input files
	yylineno = 1;
}


void
clean_up_lexer()
{
	while (!include_stack.empty()) {
		close_include();
	}

	if (stdin != yyin)
		fclose(yyin);
	yy_delete_buffer(YY_CURRENT_BUFFER);

	free(lexbuf);
}

