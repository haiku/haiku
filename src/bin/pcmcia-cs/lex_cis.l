/* Special state for handling include files */
%x src

%{
/*
 * lex_cis.l 1.15 2001/08/24 12:21:41
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License
 * at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and
 * limitations under the License. 
 *
 * The initial developer of the original code is David A. Hinds
 * <dahinds@users.sourceforge.net>.  Portions created by David A. Hinds
 * are Copyright (C) 1999 David A. Hinds.  All Rights Reserved.
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License version 2 (the "GPL"), in
 * which case the provisions of the GPL are applicable instead of the
 * above.  If you wish to allow the use of your version of this file
 * only under the terms of the GPL and not to allow others to use
 * your version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice and
 * other provisions required by the GPL.  If you do not delete the
 * provisions above, a recipient may use your version of this file
 * under either the MPL or the GPL.
 */

#undef src
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#define src 1

#include <pcmcia/cs_types.h>
#include <pcmcia/cistpl.h>

#include "pack_cis.h"
#include "yacc_cis.h"

/* For assembling nice error messages */
int current_lineno;

static int lex_number(char *);
static int lex_units(char *, int, int);
static int lex_float(char *);
static int lex_string(char *);

%}

int	[0-9]+
hex	0x[0-9a-fA-F]+
flt	[0-9]+\.[0-9]*
str	\"([^"]|\\.)*\"

%%

\n		current_lineno++;
[ \t]*		/* skip */ ;
[ ]*[#;].*	/* skip */ ;

funcid		return FUNCID;
mfc		return MFC;
manfid		return MANFID;
vers_1		return VERS_1;
checksum	return CHECKSUM;

common_jedec	return CJEDEC;
attr_jedec	return AJEDEC;

dev_info	return DEV_INFO;
attr_dev_info	return ATTR_DEV_INFO;
no_info		return NO_INFO;
NULL		return lex_number("0");
ROM		return lex_number("1");
EPROM		return lex_number("3");
EEPROM		return lex_number("4");
FLASH		return lex_number("5");
SRAM		return lex_number("6");
DRAM		return lex_number("7");
fn_specific	return lex_number("13");

config		return CONFIG;
base		return BASE;
mask		return MASK;
last_index	return LAST_INDEX;
\[post\]	return POST;
\[rom\]		return ROM;

cftable_entry	return CFTABLE;
\[default\]	return DEFAULT;
\[bvd\]		return BVD;
\[wp\]		return WP;
\[rdybsy\]	return RDYBSY;
\[mwait\]	return MWAIT;
\[audio\]	return AUDIO;
\[readonly\]	return READONLY;
\[pwrdown\]	return PWRDOWN;

Vcc		return VCC;
Vpp1		return VPP1;
Vpp2		return VPP2;
Vnom		return VNOM;
Vmin		return VMIN;
Vmax		return VMAX;
Istatic		return ISTATIC;
Iavg		return IAVG;
Ipeak		return IPEAK;
Idown		return IDOWN;

io		return IO;
memory		return MEM;
\[8bit\]	return BIT8;
\[16bit\]	return BIT16;
\[lines		return LINES;
\[range\]	return RANGE;

irq		return IRQ_NO;
\[level\]	return LEVEL;
\[pulse\]	return PULSE;
\[shared\]	return SHARED;

timing		return TIMING;
wait		return WAIT;
ready		return READY;
reserved	return RESERVED;

multi_function	return lex_number("0");
memory_card	return lex_number("1");
serial_port	return lex_number("2");
parallel_port	return lex_number("3");
fixed_disk	return lex_number("4");
video_adapter	return lex_number("5");
network_adapter	return lex_number("6");
aims_card	return lex_number("7");
scsi_adapter	return lex_number("8");

{int}		return lex_number(yytext);
{hex}		return lex_number(yytext);

{int}b		return lex_units(yytext, 1, SIZE);
{int}kb		return lex_units(yytext, 1024, SIZE);
{int}mb		return lex_units(yytext, 1024*1024, SIZE);

{flt}s		return lex_units(yytext, 1000000000, TIME);
{flt}ms		return lex_units(yytext, 1000000, TIME);
{flt}us		return lex_units(yytext, 1000, TIME);
{flt}ns		return lex_units(yytext, 1, TIME);
{int}s		return lex_units(yytext, 1000000000, TIME);
{int}ms		return lex_units(yytext, 1000000, TIME);
{int}us		return lex_units(yytext, 1000, TIME);
{int}ns		return lex_units(yytext, 1, TIME);

{flt}V		return lex_units(yytext, 100000, VOLTAGE);
{flt}mV		return lex_units(yytext, 100, VOLTAGE);
{flt}uV		return lex_units(yytext, 0.1, VOLTAGE);
{int}V		return lex_units(yytext, 100000, VOLTAGE);
{int}mV		return lex_units(yytext, 100, VOLTAGE);
{int}uV		return lex_units(yytext, 0.1, VOLTAGE);

{flt}A		return lex_units(yytext, 10000000, CURRENT);
{flt}mA		return lex_units(yytext, 10000, CURRENT);
{flt}uA		return lex_units(yytext, 10, CURRENT);
{int}A		return lex_units(yytext, 10000000, CURRENT);
{int}mA		return lex_units(yytext, 10000, CURRENT);
{int}uA		return lex_units(yytext, 10, CURRENT);

{flt}		return lex_float(yytext);

{str}		return lex_string(yytext);

.		return yytext[0];

%%

#ifndef yywrap
int yywrap() { return 1; }
#endif

/*======================================================================

    Stuff to parse basic data types

======================================================================*/

static int lex_number(char *s)
{
    yylval.num = strtoul(s, NULL, 0);
    return NUMBER;
}

static int lex_float(char *s)
{
    yylval.flt = strtod(s, NULL);
    return FLOAT;
}

static int lex_units(char *s, int scale, int token)
{
    float f;
    sscanf(s, "%f", &f);
    yylval.num = scale*f + 0.5;
    return token;
}

static int lex_string(char *s)
{
    int n = strlen(s);
    yylval.str = malloc(n-1);
    strncpy(yylval.str, s+1, n-2);
    yylval.str[n-2] = '\0';
    return STRING;
}

/*======================================================================

    The main parser entry point

======================================================================*/

void parse_cis(FILE *f)
{
    current_lineno = 1;
    yyrestart(f);
    yyparse();
    fclose(f);
}

