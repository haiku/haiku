%{
/*
 * yacc_cis.y 1.13 2001/08/24 12:21:41
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

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>

#include "pack_cis.h"

/* If bison: generate nicer error messages */ 
#define YYERROR_VERBOSE 1
 
extern int current_lineno;

void yyerror(char *msg, ...);
static tuple_info_t *new_tuple(u_char type, cisparse_t *parse);

%}

%token STRING NUMBER FLOAT VOLTAGE CURRENT SIZE
%token VERS_1 MANFID FUNCID CONFIG CFTABLE MFC CHECKSUM
%token POST ROM BASE LAST_INDEX CJEDEC AJEDEC
%token DEV_INFO ATTR_DEV_INFO NO_INFO
%token TIME TIMING WAIT READY RESERVED
%token VNOM VMIN VMAX ISTATIC IAVG IPEAK IDOWN
%token VCC VPP1 VPP2 IO MEM
%token DEFAULT BVD WP RDYBSY MWAIT AUDIO READONLY PWRDOWN
%token BIT8 BIT16 LINES RANGE
%token IRQ_NO MASK LEVEL PULSE SHARED

%union {
    char *str;
    u_long num;
    float flt;
    cistpl_power_t pwr;
    cisparse_t *parse;
    tuple_info_t *tuple;
}

%type <str> STRING
%type <num> NUMBER SIZE VOLTAGE CURRENT TIME
%type <flt> FLOAT
%type <pwr> pwr pwrlist
%type <parse> vers_1 manfid funcid config cftab io mem irq timing
%type <parse> dev_info attr_dev_info checksum cjedec ajedec
%type <tuple> tuple chain cis;
%%

cis:	  chain
		{ cis_root = $1; }
	| chain mfc
		{ cis_root = $1; }
	;

chain:	  /* nothing */
		{ $$ = NULL; }
	| chain tuple
		{
		    if ($1 == NULL) {
			$$ = $2;
		    } else if ($2 == NULL) {
			$$ = $1;
		    } else {
			tuple_info_t *tail = $1;
			while (tail->next != NULL) tail = tail->next;
			tail->next = $2;
			$$ = $1;
		    }
		} 
	;

mfc:	  MFC '{' chain '}'
		{ mfc[nf++] = $3; }
	| mfc ',' '{' chain '}'
		{ mfc[nf++] = $4; }
	;
	
tuple:	  dev_info
		{ $$ = new_tuple(CISTPL_DEVICE, $1); }
	| attr_dev_info
		{ $$ = new_tuple(CISTPL_DEVICE_A, $1); }
	| vers_1
		{ $$ = new_tuple(CISTPL_VERS_1, $1); }
	| manfid
		{ $$ = new_tuple(CISTPL_MANFID, $1); }
	| funcid
		{ $$ = new_tuple(CISTPL_FUNCID, $1); }
	| config
		{ $$ = new_tuple(CISTPL_CONFIG, $1); }
	| cftab
		{ $$ = new_tuple(CISTPL_CFTABLE_ENTRY, $1); }
	| checksum
		{ $$ = NULL; }
	| error
		{ $$ = NULL; }
	| cjedec
		{ $$ = new_tuple(CISTPL_JEDEC_C, $1); }
	| ajedec
		{ $$ = new_tuple(CISTPL_JEDEC_A, $1); }
	;

dev_info: DEV_INFO
		{ $$ = calloc(1, sizeof(cisparse_t)); }
	| dev_info NUMBER TIME ',' SIZE
		{
		    $$->device.dev[$$->device.ndev].type = $2;
		    $$->device.dev[$$->device.ndev].speed = $3;
		    $$->device.dev[$$->device.ndev].size = $5;
		    $$->device.ndev++;
		}
	| dev_info NO_INFO
	;

attr_dev_info: ATTR_DEV_INFO
		{ $$ = calloc(1, sizeof(cisparse_t)); }
	| attr_dev_info NUMBER TIME ',' SIZE
		{
		    $$->device.dev[$$->device.ndev].type = $2;
		    $$->device.dev[$$->device.ndev].speed = $3;
		    $$->device.dev[$$->device.ndev].size = $5;
		    $$->device.ndev++;
		}
	| attr_dev_info NO_INFO
	;

vers_1:	  VERS_1 FLOAT
		{
		    $$ = calloc(1, sizeof(cisparse_t));
		    $$->version_1.major = $2;
		    $2 -= floor($2+0.01);
		    while (fabs($2 - floor($2+0.5)) > 0.01) {
			$2 *= 10;
		    }
		    $$->version_1.minor = $2+0.01;
		}
	| vers_1 ',' STRING
		{
		    cistpl_vers_1_t *v = &$$->version_1;
		    u_int pos = 0;
		    if (v->ns) {
			pos = v->ofs[v->ns-1];
			pos += strlen(v->str+pos)+1;
		    }
		    v->ofs[v->ns] = pos;
		    strcpy(v->str+pos, $3);
		    v->ns++;
		}
	;

manfid:	  MANFID NUMBER ',' NUMBER
		{
		    $$ = calloc(1, sizeof(cisparse_t));
		    $$->manfid.manf = $2;
		    $$->manfid.card = $4;
		}
	;

funcid:	  FUNCID NUMBER 
		{
		    $$ = calloc(1, sizeof(cisparse_t));
		    $$->funcid.func = $2;
		}
	| funcid POST
		{ $$->funcid.sysinit |= CISTPL_SYSINIT_POST; }
	| funcid ROM
		{ $$->funcid.sysinit |= CISTPL_SYSINIT_ROM; }
	;

cjedec:	  CJEDEC NUMBER NUMBER
		{
		    $$ = calloc(1, sizeof(cisparse_t));
		    $$->jedec.id[0].mfr = $2;
		    $$->jedec.id[0].info = $3;
		    $$->jedec.nid = 1;
		}
	| cjedec ',' NUMBER NUMBER
		{
		    $$->jedec.id[$$->jedec.nid].mfr = $3;
		    $$->jedec.id[$$->jedec.nid++].info = $4;
		}
	;

ajedec:	  AJEDEC NUMBER NUMBER
		{
		    $$ = calloc(1, sizeof(cisparse_t));
		    $$->jedec.id[0].mfr = $2;
		    $$->jedec.id[0].info = $3;
		    $$->jedec.nid = 1;
		}
	| ajedec ',' NUMBER NUMBER
		{
		    $$->jedec.id[$$->jedec.nid].mfr = $3;
		    $$->jedec.id[$$->jedec.nid++].info = $4;
		}
	;

config:	  CONFIG BASE NUMBER MASK NUMBER  LAST_INDEX NUMBER
		{
		    $$ = calloc(1, sizeof(cisparse_t));
		    $$->config.base = $3;
		    $$->config.rmask[0] = $5;
		    $$->config.last_idx = $7;
		}
	;

pwr:	  VNOM VOLTAGE
		{
		    $$.present = CISTPL_POWER_VNOM;
		    $$.param[0] = $2;
		}
	| VMIN VOLTAGE
		{
		    $$.present = CISTPL_POWER_VMIN;
		    $$.param[0] = $2;
		}
	| VMAX VOLTAGE
		{
		    $$.present = CISTPL_POWER_VMAX;
		    $$.param[0] = $2;
		}
	| ISTATIC CURRENT
		{
		    $$.present = CISTPL_POWER_ISTATIC;
		    $$.param[0] = $2;
		}
	| IAVG CURRENT
		{
		    $$.present = CISTPL_POWER_IAVG;
		    $$.param[0] = $2;
		}
	| IPEAK CURRENT
		{
		    $$.present = CISTPL_POWER_IPEAK;
		    $$.param[0] = $2;
		}
	| IDOWN CURRENT
		{
		    $$.present = CISTPL_POWER_IDOWN;
		    $$.param[0] = $2;
		}
	;

pwrlist:  /* nothing */
		{
		    $$.present = 0;
		}
	| pwrlist pwr
		{
		    $$.present |= 1<<($2.present);
		    $$.param[$2.present] = $2.param[0];
		}
	;

timing:	  cftab TIMING
	| timing WAIT TIME
	| timing READY TIME
	| timing RESERVED TIME
	;

io:	  cftab IO NUMBER '-' NUMBER
		{
		    int n = $$->cftable_entry.io.nwin;
		    $$->cftable_entry.io.win[n].base = $3;
		    $$->cftable_entry.io.win[n].len = $5-$3+1;
		    $$->cftable_entry.io.nwin++;
		}
	| io ',' NUMBER '-' NUMBER
		{
		    int n = $$->cftable_entry.io.nwin;
		    $$->cftable_entry.io.win[n].base = $3;
		    $$->cftable_entry.io.win[n].len = $5-$3+1;
		    $$->cftable_entry.io.nwin++;
		}
	| io BIT8
		{ $$->cftable_entry.io.flags |= CISTPL_IO_8BIT; }
	| io BIT16
		{ $$->cftable_entry.io.flags |= CISTPL_IO_16BIT; }
	| io LINES '=' NUMBER ']'
		{ $$->cftable_entry.io.flags |= $4; }
	| io RANGE
	;	

mem:	  cftab MEM NUMBER '-' NUMBER '@' NUMBER
		{
		    int n = $$->cftable_entry.mem.nwin;
		    $$->cftable_entry.mem.win[n].card_addr = $3;
		    $$->cftable_entry.mem.win[n].host_addr = $7;
		    $$->cftable_entry.mem.win[n].len = $5-$3+1;
		    $$->cftable_entry.mem.nwin++;
		}
	| mem ',' NUMBER '-' NUMBER '@' NUMBER
		{
		    int n = $$->cftable_entry.mem.nwin;
		    $$->cftable_entry.mem.win[n].card_addr = $3;
		    $$->cftable_entry.mem.win[n].host_addr = $7;
		    $$->cftable_entry.mem.win[n].len = $5-$3+1;
		    $$->cftable_entry.mem.nwin++;
		}
	| mem BIT8
		{ $$->cftable_entry.io.flags |= CISTPL_IO_8BIT; }
	| mem BIT16
		{ $$->cftable_entry.io.flags |= CISTPL_IO_16BIT; }
	;	

irq:	  cftab IRQ_NO NUMBER
		{ $$->cftable_entry.irq.IRQInfo1 = ($3 & 0x0f); }
	| cftab IRQ_NO MASK NUMBER
		{
		    $$->cftable_entry.irq.IRQInfo1 = IRQ_INFO2_VALID;
		    $$->cftable_entry.irq.IRQInfo2 = $4;
		}
	| irq PULSE
		{ $$->cftable_entry.irq.IRQInfo1 |= IRQ_PULSE_ID; }
	| irq LEVEL
		{ $$->cftable_entry.irq.IRQInfo1 |= IRQ_LEVEL_ID; }
	| irq SHARED
		{ $$->cftable_entry.irq.IRQInfo1 |= IRQ_SHARE_ID; }
	;

cftab:	  CFTABLE NUMBER
		{
		    $$ = calloc(1, sizeof(cisparse_t));
		    $$->cftable_entry.index = $2;
		}
	| cftab DEFAULT
		{ $$->cftable_entry.flags |= CISTPL_CFTABLE_DEFAULT; }
	| cftab BVD
		{ $$->cftable_entry.flags |= CISTPL_CFTABLE_BVDS; }
	| cftab WP
		{ $$->cftable_entry.flags |= CISTPL_CFTABLE_WP; }
	| cftab RDYBSY
		{ $$->cftable_entry.flags |= CISTPL_CFTABLE_RDYBSY; }
	| cftab MWAIT
		{ $$->cftable_entry.flags |= CISTPL_CFTABLE_MWAIT; }
	| cftab AUDIO
		{ $$->cftable_entry.flags |= CISTPL_CFTABLE_AUDIO; }
	| cftab READONLY
		{ $$->cftable_entry.flags |= CISTPL_CFTABLE_READONLY; }
	| cftab PWRDOWN
		{ $$->cftable_entry.flags |= CISTPL_CFTABLE_PWRDOWN; }
	| cftab VCC pwrlist
		{ $$->cftable_entry.vcc = $3; }
	| cftab VPP1 pwrlist
		{ $$->cftable_entry.vpp1 = $3; }
	| cftab VPP2 pwrlist
		{ $$->cftable_entry.vpp2 = $3; }
	| io
	| mem
	| irq
	| timing
	;

checksum: CHECKSUM NUMBER '-' NUMBER '=' NUMBER
	{ $$ = NULL; }

%%

static tuple_info_t *new_tuple(u_char type, cisparse_t *parse)
{
    tuple_info_t *t = calloc(1, sizeof(tuple_info_t));
    t->type = type;
    t->parse = parse;
    t->next = NULL;
}

void yyerror(char *msg, ...)
{
    va_list ap;
    char str[256];

    va_start(ap, msg);
    sprintf(str, "error at line %d: ", current_lineno);
    vsprintf(str+strlen(str), msg, ap);
    fprintf(stderr, "%s\n", str);
    va_end(ap);
}

#ifdef DEBUG
void main(int argc, char *argv[])
{
    if (argc > 1)
	parse_cis(argv[1]);
}
#endif
