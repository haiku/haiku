/*
 * Copyright 2003-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <parsedate.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <OS.h>


#define TRACE_PARSEDATE 0
#if TRACE_PARSEDATE
#	define TRACE(x) printf x ;
#else
#	define TRACE(x) ;
#endif


/* The date format is as follows:
 *
 *	a/A		weekday
 *	d		day of month
 *	b/B		month name
 *	m		month
 *	y/Y		year
 *	H/I		hours
 *	M		minute
 *	S		seconds
 *	p		meridian (i.e. am/pm)
 *	T		time unit: last hour, next tuesday, today, ...
 *	z/Z		time zone
 *	-		dash or slash
 *
 *	Any of ",.:" is allowed and will be expected in the input string as is.
 *	You can enclose a single field with "[]" to mark it as being optional.
 *	A space stands for white space.
 *	No other character is allowed.
 */

static const char * const kFormatsTable[] = {
	"[A][,] B d[,] H:M:S [p] Y[,] [Z]",
	"[A][,] B d[,] [Y][,] H:M:S [p] [Z]",
	"[A][,] B d[,] [Y][,] H:M [p][,] [Z]",
	"[A][,] B d[,] [Y][,] H [p][,] [Z]",
	"[A][,] B d[,] H:M [p][,] [Y] [Z]",
	"[A][,] d B[,] [Y][,] H:M [p][,] [Z]",
	"[A][,] d B[,] [Y][,] H:M:S [p][,] [Z]",
	"[A][,] d B[,] H:M:S [Y][,] [p][,] [Z]",
	"[A][,] d B[,] H:M [Y][,] [p][,] [Z]",
	"d.m.y H:M:S [p] [Z]",
	"d.m.y H:M [p] [Z]",
	"d.m.y",
	"[A][,] m-d-y[,] [H][:][M] [p]",
	"[A][,] m-d[,] H:M [p]",
	"[A][,] m-d[,] H[p]",
	"[A][,] m-d",
	"[A][,] B d[,] Y",
	"[A][,] H:M [p]",
	"[A][,] H [p]",
	"H:M [p][,] [A]",
	"H [p][,] [A]",
	"[A][,] B d[,] H:M:S [p] [Z] [Y]",
	"[A][,] B d[,] H:M [p] [Z] [Y]",
	"[A][,] d B [,] H:M:S [p] [Z] [Y]",
	"[A][,] d B [,] H:M [p] [Z] [Y]",
	"[A][,] d-B-Y H:M:S [p] [Z]",
	"[A][,] d-B-Y H:M [p] [Z]",
	"d B Y H:M:S [Z]",
	"d B Y H:M [Z]",
	"y-m-d",
	"y-m-d H:M:S [p] [Z]",
	"m-d-y H[p]",
	"m-d-y H:M[p]",
	"m-d-y H:M:S[p]",
	"H[p] m-d-y",
	"H:M[p] m-d-y",
	"H:M:S[p] m-d-y",
	"A[,] H:M:S [p] [Z]",
	"A[,] H:M [p] [Z]",
	"H:M:S [p] [Z]",
	"H:M [p] [Z]",
	"A[,] [B] [d] [Y]",
	"A[,] [d] [B] [Y]",
	"B d[,][Y] H[p][,] [Z]",
	"B d[,] H[p]",
	"B d [,] H:M [p]",
	"d B [,][Y] H [p] [Z]",
	"d B [,] H:M [p]",
	"B d [,][Y]",
	"B d [,] H:M [p][,] [Y]",
	"B d [,] H [p][,] [Y]",
	"d B [,][Y]",
	"H[p] [,] B d",
	"H:M[p] [,] B d",
	"T [T][T][T][T][T]",
	"T H:M:S [p]",
	"T H:M [p]",
	"T H [p]",
	"H:M [p] T",
	"H [p] T",
	"H [p]",
	NULL
};
static const char* const* sFormatsTable = kFormatsTable;


enum field_type {
	TYPE_UNKNOWN = 0,

	TYPE_DAY,
	TYPE_MONTH,
	TYPE_YEAR,
	TYPE_WEEKDAY,
	TYPE_HOUR,
	TYPE_MINUTE,
	TYPE_SECOND,
	TYPE_TIME_ZONE,
	TYPE_MERIDIAN,

	TYPE_DASH,
	TYPE_DOT,
	TYPE_COMMA,
	TYPE_COLON,

	TYPE_UNIT,
	TYPE_MODIFIER,
	TYPE_END,
};

#define FLAG_NONE				0
#define FLAG_RELATIVE			1
#define FLAG_NOT_MODIFIABLE		2
#define FLAG_NOW				4
#define FLAG_NEXT_LAST_THIS		8
#define FLAG_PLUS_MINUS			16
#define FLAG_HAS_DASH			32

enum units {
	UNIT_NONE,
	UNIT_YEAR,
	UNIT_MONTH,
	UNIT_DAY,
	UNIT_SECOND,
};

enum value_type {
	VALUE_NUMERICAL,
	VALUE_STRING,
	VALUE_CHAR,
};

enum value_modifier {
	MODIFY_MINUS	= -2,
	MODIFY_LAST		= -1,
	MODIFY_NONE		= 0,
	MODIFY_THIS		= MODIFY_NONE,
	MODIFY_NEXT		= 1,
	MODIFY_PLUS		= 2,
};

struct known_identifier {
	const char	*string;
	const char	*alternate_string;
	uint8		type;
	uint8		flags;
	uint8		unit;
	int32		value;
};

static const known_identifier kIdentifiers[] = {
	{"today",		NULL,	TYPE_UNIT, FLAG_RELATIVE | FLAG_NOT_MODIFIABLE,
		UNIT_DAY, 0},
	{"tomorrow",	NULL,	TYPE_UNIT, FLAG_RELATIVE | FLAG_NOT_MODIFIABLE,
		UNIT_DAY, 1},
	{"yesterday",	NULL,	TYPE_UNIT, FLAG_RELATIVE | FLAG_NOT_MODIFIABLE,
		UNIT_DAY, -1},
	{"now",			NULL,	TYPE_UNIT,
		FLAG_RELATIVE | FLAG_NOT_MODIFIABLE | FLAG_NOW, 0},

	{"this",		NULL,	TYPE_MODIFIER, FLAG_NEXT_LAST_THIS, UNIT_NONE,
		MODIFY_THIS},
	{"next",		NULL,	TYPE_MODIFIER, FLAG_NEXT_LAST_THIS, UNIT_NONE,
		MODIFY_NEXT},
	{"last",		NULL,	TYPE_MODIFIER, FLAG_NEXT_LAST_THIS, UNIT_NONE,
		MODIFY_LAST},

	{"years",		"year",	TYPE_UNIT, FLAG_RELATIVE, UNIT_YEAR, 1},
	{"months",		"month",TYPE_UNIT, FLAG_RELATIVE, UNIT_MONTH, 1},
	{"weeks",		"week",	TYPE_UNIT, FLAG_RELATIVE, UNIT_DAY, 7},
	{"days",		"day",	TYPE_UNIT, FLAG_RELATIVE, UNIT_DAY, 1},
	{"hour",		NULL,	TYPE_UNIT, FLAG_RELATIVE, UNIT_SECOND, 1 * 60 * 60},
	{"hours",		"hrs",	TYPE_UNIT, FLAG_RELATIVE, UNIT_SECOND, 1 * 60 * 60},
	{"second",		"sec",	TYPE_UNIT, FLAG_RELATIVE, UNIT_SECOND, 1},
	{"seconds",		"secs",	TYPE_UNIT, FLAG_RELATIVE, UNIT_SECOND, 1},
	{"minute",		"min",	TYPE_UNIT, FLAG_RELATIVE, UNIT_SECOND, 60},
	{"minutes",		"mins",	TYPE_UNIT, FLAG_RELATIVE, UNIT_SECOND, 60},

	{"am",			NULL,	TYPE_MERIDIAN, FLAG_NOT_MODIFIABLE, UNIT_SECOND, 0},
	{"pm",			NULL,	TYPE_MERIDIAN, FLAG_NOT_MODIFIABLE, UNIT_SECOND,
		12 * 60 * 60},

	{"sunday",		"sun",	TYPE_WEEKDAY, FLAG_NONE, UNIT_DAY, 0},
	{"monday",		"mon",	TYPE_WEEKDAY, FLAG_NONE, UNIT_DAY, 1},
	{"tuesday",		"tue",	TYPE_WEEKDAY, FLAG_NONE, UNIT_DAY, 2},
	{"wednesday",	"wed",	TYPE_WEEKDAY, FLAG_NONE, UNIT_DAY, 3},
	{"thursday",	"thu",	TYPE_WEEKDAY, FLAG_NONE, UNIT_DAY, 4},
	{"friday",		"fri",	TYPE_WEEKDAY, FLAG_NONE, UNIT_DAY, 5},
	{"saturday",	"sat",	TYPE_WEEKDAY, FLAG_NONE, UNIT_DAY, 6},

	{"january",		"jan",	TYPE_MONTH, FLAG_NONE, UNIT_MONTH, 1},
	{"february",	"feb", 	TYPE_MONTH, FLAG_NONE, UNIT_MONTH, 2},
	{"march",		"mar",	TYPE_MONTH, FLAG_NONE, UNIT_MONTH, 3},
	{"april",		"apr",	TYPE_MONTH, FLAG_NONE, UNIT_MONTH, 4},
	{"may",			"may",	TYPE_MONTH, FLAG_NONE, UNIT_MONTH, 5},
	{"june",		"jun",	TYPE_MONTH, FLAG_NONE, UNIT_MONTH, 6},
	{"july",		"jul",	TYPE_MONTH, FLAG_NONE, UNIT_MONTH, 7},
	{"august",		"aug",	TYPE_MONTH, FLAG_NONE, UNIT_MONTH, 8},
	{"september",	"sep",	TYPE_MONTH, FLAG_NONE, UNIT_MONTH, 9},
	{"october",		"oct",	TYPE_MONTH, FLAG_NONE, UNIT_MONTH, 10},
	{"november",	"nov",	TYPE_MONTH, FLAG_NONE, UNIT_MONTH, 11},
	{"december",	"dec",	TYPE_MONTH, FLAG_NONE, UNIT_MONTH, 12},

	{"GMT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 0},
	{"UTC",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 0},
	// the following list has been generated from info found at
	//     http://www.timegenie.com/timezones
	{"ACDT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1050 * 36},
	{"ACIT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 800 * 36},
	{"ACST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 950 * 36},
	{"ACT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -500 * 36},
	{"ACWST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 875 * 36},
	{"ADT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -300 * 36},
	{"AEDT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1100 * 36},
	{"AEST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1000 * 36},
	{"AFT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 450 * 36},
	{"AKDT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -800 * 36},
	{"AKST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -900 * 36},
	{"AMDT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 500 * 36},
	{"AMST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 400 * 36},
	{"ANAST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1300 * 36},
	{"ANAT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1200 * 36},
	{"APO",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 825 * 36},
	{"ARDT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -200 * 36},
	{"ART",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -300 * 36},
	{"AST",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -400 * 36},
	{"AWST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 800 * 36},
	{"AZODT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 0 * 36},
	{"AZOST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -100 * 36},
	{"AZST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 500 * 36},
	{"AZT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 400 * 36},
	{"BIT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -1200 * 36},
	{"BDT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 600 * 36},
	{"BEST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -200 * 36},
	{"BIOT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 600 * 36},
	{"BNT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 800 * 36},
	{"BOT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -400 * 36},
	{"BRST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -200 * 36},
	{"BRT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -300 * 36},
	{"BST",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 100 * 36},
	{"BTT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 600 * 36},
	{"BWDT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -300 * 36},
	{"BWST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -400 * 36},
	{"CAST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 500 * 36},
	{"CAT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 200 * 36},
	{"CCT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 650 * 36},
	{"CDT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -500 * 36},
	{"CEST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 200 * 36},
	{"CET",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 100 * 36},
	{"CGST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -200 * 36},
	{"CGT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -300 * 36},
	{"CHADT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1375 * 36},
	{"CHAST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1275 * 36},
	{"CHST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1000 * 36},
	{"CIST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -800 * 36},
	{"CKT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -1000 * 36},
	{"CLDT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -300 * 36},
	{"CLST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -400 * 36},
	{"COT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -500 * 36},
	{"CST",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -600 * 36},
	{"CVT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -100 * 36},
	{"CXT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 700 * 36},
	{"DAVT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 700 * 36},
	{"DTAT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1000 * 36},
	{"EADT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -500 * 36},
	{"EAST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -600 * 36},
	{"EAT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 300 * 36},
	{"ECT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -500 * 36},
	{"EDT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -400 * 36},
	{"EEST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 300 * 36},
	{"EET",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 200 * 36},
	{"EGT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -100 * 36},
	{"EGST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 0 * 36},
	{"EKST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 600 * 36},
	{"EST",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -500 * 36},
	{"FJT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1200 * 36},
	{"FKDT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -300 * 36},
	{"FKST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -400 * 36},
	{"GALT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -600 * 36},
	{"GET",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 400 * 36},
	{"GFT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -300 * 36},
	{"GILT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1200 * 36},
	{"GIT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -900 * 36},
	{"GST",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 400 * 36},
	{"GYT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -400 * 36},
	{"HADT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -900 * 36},
	{"HAST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -1000 * 36},
	{"HKST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 800 * 36},
	{"HMT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 500 * 36},
	{"ICT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 700 * 36},
	{"IDT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 300 * 36},
	{"IRDT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 450 * 36},
	{"IRKST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 900 * 36},
	{"IRKT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 800 * 36},
	{"IRST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 350 * 36},
	{"IST",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 200 * 36},
	{"JFDT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -300 * 36},
	{"JFST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -400 * 36},
	{"JST",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 900 * 36},
	{"KGST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 600 * 36},
	{"KGT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 500 * 36},
	{"KRAST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 800 * 36},
	{"KRAT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 700 * 36},
	{"KOST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1100 * 36},
	{"KOVT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 700 * 36},
	{"KOVST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 800 * 36},
	{"KST",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 900 * 36},
	{"LHDT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1100 * 36},
	{"LHST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1050 * 36},
	{"LINT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1400 * 36},
	{"LKT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 600 * 36},
	{"MAGST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1200 * 36},
	{"MAGT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1100 * 36},
	{"MAWT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 600 * 36},
	{"MBT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 800 * 36},
	{"MDT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -600 * 36},
	{"MIT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -950 * 36},
	{"MHT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1200 * 36},
	{"MMT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 650 * 36},
	{"MNT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 800 * 36},
	{"MNST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 900 * 36},
	{"MSD",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 400 * 36},
	{"MSK",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 300 * 36},
	{"MST",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -700 * 36},
	{"MUST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 500 * 36},
	{"MUT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 400 * 36},
	{"MVT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 500 * 36},
	{"MYT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 800 * 36},
	{"NCT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1100 * 36},
	{"NDT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -250 * 36},
	{"NFT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1150 * 36},
	{"NPT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 575 * 36},
	{"NRT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1200 * 36},
	{"NOVST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 700 * 36},
	{"NOVT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 600 * 36},
	{"NST",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -350 * 36},
	{"NUT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -1100 * 36},
	{"NZDT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1300 * 36},
	{"NZST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1200 * 36},
	{"OMSK",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 700 * 36},
	{"OMST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 600 * 36},
	{"PDT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -700 * 36},
	{"PETST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1300 * 36},
	{"PET",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -500 * 36},
	{"PETT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1200 * 36},
	{"PGT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1000 * 36},
	{"PHOT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1300 * 36},
	{"PHT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 800 * 36},
	{"PIT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 800 * 36},
	{"PKT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 500 * 36},
	{"PKST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 600 * 36},
	{"PMDT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -200 * 36},
	{"PMST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -300 * 36},
	{"PONT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1100 * 36},
	{"PST",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -800 * 36},
	{"PWT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 900 * 36},
	{"PYST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -300 * 36},
	{"PYT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -400 * 36},
	{"RET",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 400 * 36},
	{"ROTT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -300 * 36},
	{"SAMST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 500 * 36},
	{"SAMT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 400 * 36},
	{"SAST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 200 * 36},
	{"SBT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1100 * 36},
	{"SCDT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1300 * 36},
	{"SCST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1200 * 36},
	{"SCT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 400 * 36},
	{"SGT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 800 * 36},
	{"SIT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 800 * 36},
	{"SLT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -400 * 36},
	{"SLST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -300 * 36},
	{"SRT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -300 * 36},
	{"SST",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -1100 * 36},
	{"SYST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 300 * 36},
	{"SYT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 200 * 36},
	{"TAHT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -1000 * 36},
	{"TFT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 500 * 36},
	{"TJT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 500 * 36},
	{"TKT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -1000 * 36},
	{"TMT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 500 * 36},
	{"TOT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1300 * 36},
	{"TPT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 900 * 36},
	{"TRUT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1000 * 36},
	{"TVT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1200 * 36},
	{"TWT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 800 * 36},
	{"UYT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -300 * 36},
	{"UYST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -200 * 36},
	{"UZT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 500 * 36},
	{"VLAST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1100 * 36},
	{"VLAT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1000 * 36},
	{"VOST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 600 * 36},
	{"VST",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, -450 * 36},
	{"VUT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1100 * 36},
	{"WAST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 200 * 36},
	{"WAT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 100 * 36},
	{"WEST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 100 * 36},
	{"WET",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 0 * 36},
	{"WFT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1200 * 36},
	{"WIB",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 700 * 36},
	{"WIT",		NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 900 * 36},
	{"WITA",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 800 * 36},
	{"WKST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 500 * 36},
	{"YAKST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1000 * 36},
	{"YAKT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 900 * 36},
	{"YAPT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 1000 * 36},
	{"YEKST",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 600 * 36},
	{"YEKT",	NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 500 * 36},

	{NULL}
};

#define MAX_ELEMENTS	32

class DateMask {
	public:
		DateMask() : fMask(0UL) {}

		void Set(uint8 type) { fMask |= Flag(type); }
		bool IsSet(uint8 type) { return (fMask & Flag(type)) != 0; }

		bool HasTime();
		bool IsComplete();

	private:
		inline uint32 Flag(uint8 type) const { return 1UL << type; }

		uint32	fMask;
};


struct parsed_element {
	uint8		base_type;
	uint8		type;
	uint8		flags;
	uint8		unit;
	uint8		value_type;
	int8		modifier;
	bigtime_t	value;

	void SetCharType(uint8 fieldType, int8 modify = MODIFY_NONE);

	void Adopt(const known_identifier& identifier);
	void AdoptUnit(const known_identifier& identifier);
	bool IsNextLastThis();
};


void
parsed_element::SetCharType(uint8 fieldType, int8 modify)
{
	base_type = type = fieldType;
	value_type = VALUE_CHAR;
	modifier = modify;
}


void
parsed_element::Adopt(const known_identifier& identifier)
{
	base_type = type = identifier.type;
	flags = identifier.flags;
	unit = identifier.unit;

	if (identifier.type == TYPE_MODIFIER)
		modifier = identifier.value;

	value_type = VALUE_STRING;
	value = identifier.value;
}


void
parsed_element::AdoptUnit(const known_identifier& identifier)
{
	base_type = type = TYPE_UNIT;
	flags = identifier.flags;
	unit = identifier.unit;
	value *= identifier.value;
}


inline bool
parsed_element::IsNextLastThis()
{
	return base_type == TYPE_MODIFIER
		&& (modifier == MODIFY_NEXT || modifier == MODIFY_LAST
			|| modifier == MODIFY_THIS);
}


//	#pragma mark -


bool
DateMask::HasTime()
{
	// this will cause
	return IsSet(TYPE_HOUR);
}


/*!	This method checks if the date mask is complete in the
	sense that it doesn't need to have a prefilled "struct tm"
	when its time value is computed.
*/
bool
DateMask::IsComplete()
{
	// mask must be absolute, at last
	if ((fMask & Flag(TYPE_UNIT)) != 0)
		return false;

	// minimal set of flags to have a complete set
	return !(~fMask & (Flag(TYPE_DAY) | Flag(TYPE_MONTH)));
}


//	#pragma mark -


static status_t
preparseDate(const char* dateString, parsed_element* elements)
{
	int32 index = 0, modify = MODIFY_NONE;
	char c;

	if (dateString == NULL)
		return B_ERROR;

	memset(&elements[0], 0, sizeof(parsed_element));

	for (; (c = dateString[0]) != '\0'; dateString++) {
		// we don't care about spaces
		if (isspace(c)) {
			modify = MODIFY_NONE;
			continue;
		}

		// if we're reached our maximum number of elements, bail out
		if (index >= MAX_ELEMENTS)
			return B_ERROR;

		if (c == ',') {
			elements[index].SetCharType(TYPE_COMMA);
		} else if (c == '.') {
			elements[index].SetCharType(TYPE_DOT);
		} else if (c == '/') {
			// "-" is handled differently (as a modifier)
			elements[index].SetCharType(TYPE_DASH);
		} else if (c == ':') {
			elements[index].SetCharType(TYPE_COLON);
		} else if (c == '+') {
			modify = MODIFY_PLUS;

			// this counts for the next element
			continue;
		} else if (c == '-') {
			modify = MODIFY_MINUS;
			elements[index].flags = FLAG_HAS_DASH;

			// this counts for the next element
			continue;
		} else if (isdigit(c)) {
			// fetch whole number

			elements[index].type = TYPE_UNKNOWN;
			elements[index].value_type = VALUE_NUMERICAL;
			elements[index].value = atoll(dateString);
			elements[index].modifier = modify;

			// skip number
			while (isdigit(dateString[1]))
				dateString++;

			// check for "1st", "2nd, "3rd", "4th", ...

			const char* suffixes[] = {"th", "st", "nd", "rd"};
			const char* validSuffix = elements[index].value > 3
				? "th" : suffixes[elements[index].value];
			if (!strncasecmp(dateString + 1, validSuffix, 2)
				&& !isalpha(dateString[3])) {
				// for now, just ignore the suffix - but we might be able
				// to deduce some meaning out of it, since it's not really
				// possible to put it in anywhere
				dateString += 2;
			}
		} else if (isalpha(c)) {
			// fetch whole string

			const char* string = dateString;
			while (isalpha(dateString[1]))
				dateString++;
			int32 length = dateString + 1 - string;

			// compare with known strings
			// ToDo: should understand other languages as well...

			const known_identifier* identifier = kIdentifiers;
			for (; identifier->string; identifier++) {
				if (!strncasecmp(identifier->string, string, length)
					&& !identifier->string[length])
					break;

				if (identifier->alternate_string != NULL
					&& !strncasecmp(identifier->alternate_string, string, length)
					&& !identifier->alternate_string[length])
					break;
			}
			if (identifier->string == NULL) {
				// unknown string, we don't have to parse any further
				return B_ERROR;
			}

			if (index > 0 && identifier->type == TYPE_UNIT) {
				// this is just a unit, so it will give the last value a meaning

				if (elements[--index].value_type != VALUE_NUMERICAL
					&& !elements[index].IsNextLastThis())
					return B_ERROR;

				elements[index].AdoptUnit(*identifier);
			} else if (index > 0 && elements[index - 1].IsNextLastThis()) {
				if (identifier->type == TYPE_MONTH
					|| identifier->type == TYPE_WEEKDAY) {
					index--;

					switch (elements[index].value) {
						case -1:
							elements[index].modifier = MODIFY_LAST;
							break;
						case 0:
							elements[index].modifier = MODIFY_THIS;
							break;
						case 1:
							elements[index].modifier = MODIFY_NEXT;
							break;
					}
					elements[index].Adopt(*identifier);
					elements[index].type = TYPE_UNIT;
				} else
					return B_ERROR;
			} else {
				elements[index].Adopt(*identifier);
			}
		}

		// see if we can join any preceding modifiers

		if (index > 0
			&& elements[index - 1].type == TYPE_MODIFIER
			&& (elements[index].flags & FLAG_NOT_MODIFIABLE) == 0) {
			// copy the current one to the last and go on
			elements[index].modifier = elements[index - 1].modifier;
			elements[index].value *= elements[index - 1].value;
			elements[index].flags |= elements[index - 1].flags;
			elements[index - 1] = elements[index];
		} else {
			// we filled out one parsed_element
			index++;
		}

		if (index < MAX_ELEMENTS)
			memset(&elements[index], 0, sizeof(parsed_element));
	}

	// were there any elements?
	if (index == 0)
		return B_ERROR;

	elements[index].type = TYPE_END;

	return B_OK;
}


static void
computeRelativeUnit(parsed_element& element, struct tm& tm, int* _flags)
{
	// set the relative start depending on unit

	switch (element.unit) {
		case UNIT_YEAR:
			tm.tm_mon = 0;	// supposed to fall through
		case UNIT_MONTH:
			tm.tm_mday = 1;	// supposed to fall through
		case UNIT_DAY:
			tm.tm_hour = 0;
			tm.tm_min = 0;
			tm.tm_sec = 0;
			break;
	}

	// adjust value

	if ((element.flags & FLAG_RELATIVE) != 0) {
		bigtime_t value = element.value;
		if (element.modifier == MODIFY_MINUS)
			value = -element.value;

		if (element.unit == UNIT_MONTH)
			tm.tm_mon += value;
		else if (element.unit == UNIT_DAY)
			tm.tm_mday += value;
		else if (element.unit == UNIT_SECOND) {
			tm.tm_sec += value;
			*_flags |= PARSEDATE_MINUTE_RELATIVE_TIME;
		} else if (element.unit == UNIT_YEAR)
			tm.tm_year += value;
	} else if (element.base_type == TYPE_WEEKDAY) {
		tm.tm_mday += element.value - tm.tm_wday;

		if (element.modifier == MODIFY_NEXT)
			tm.tm_mday += 7;
		else if (element.modifier == MODIFY_LAST)
			tm.tm_mday -= 7;
	} else if (element.base_type == TYPE_MONTH) {
		tm.tm_mon = element.value - 1;

		if (element.modifier == MODIFY_NEXT)
			tm.tm_year++;
		else if (element.modifier == MODIFY_LAST)
			tm.tm_year--;
	}
}


/*!	Uses the format assignment (through "format", and "optional") for the
	parsed elements and calculates the time value with respect to "now".
	Will also set the day/minute relative flags in "_flags".
*/
static time_t
computeDate(const char* format, bool* optional, parsed_element* elements,
	time_t now, DateMask dateMask, int* _flags)
{
	TRACE(("matches: %s\n", format));

	parsed_element* element = elements;
	uint32 position = 0;
	struct tm tm;

	if (now == -1)
		now = time(NULL);

	int nowYear = -1;
	if (dateMask.IsComplete())
		memset(&tm, 0, sizeof(tm));
	else {
		localtime_r(&now, &tm);
		nowYear = tm.tm_year;
		if (dateMask.HasTime()) {
			tm.tm_min = 0;
			tm.tm_sec = 0;
		}

		*_flags = PARSEDATE_RELATIVE_TIME;
	}

	while (element->type != TYPE_END) {
		// skip whitespace
		while (isspace(format[0]))
			format++;

		if (format[0] == '[' && format[2] == ']') {
			// does this optional parameter not match our date string?
			if (!optional[position]) {
				format += 3;
				position++;
				continue;
			}

			format++;
		}

		switch (element->value_type) {
			case VALUE_CHAR:
				// skip the single character
				break;

			case VALUE_NUMERICAL:
				switch (format[0]) {
					case 'd':
						tm.tm_mday = element->value;
						break;
					case 'm':
						tm.tm_mon = element->value - 1;
						break;
					case 'H':
					case 'I':
						tm.tm_hour = element->value;
						break;
					case 'M':
						tm.tm_min = element->value;
						break;
					case 'S':
						tm.tm_sec = element->value;
						break;
					case 'y':
					case 'Y':
					{
						if (nowYear < 0) {
							struct tm tmNow;
							localtime_r(&now, &tmNow);
							nowYear	= tmNow.tm_year;
						}
						int nowYearInCentury = nowYear % 100;
						int nowCentury = 1900 + nowYear - nowYearInCentury;

						tm.tm_year = element->value;
						if (tm.tm_year < 1900) {
							// just a relative year like 11 (2011)

							// interpret something like 50 as 1950 but
							// something like 11 as 2011 (assuming now is 2011)
							if (nowYearInCentury + 10 < tm.tm_year % 100)
								tm.tm_year -= 100;
							
							tm.tm_year += nowCentury - 1900;
						}
						else {
							tm.tm_year -= 1900;
						}
						break;
					}
					case 'z':	// time zone
					case 'Z':
					{
						bigtime_t value
							= (element->value - element->value % 100) * 36
								+ (element->value % 100) * 60;
						if (element->modifier == MODIFY_MINUS)
							value *= -1;
						tm.tm_sec -= value + timezone;
						break;
					}
					case 'T':
						computeRelativeUnit(*element, tm, _flags);
						break;
					case '-':
						// there is no TYPE_DASH element for this (just a flag)
						format++;
						continue;
				}
				break;

			case VALUE_STRING:
				switch (format[0]) {
					case 'a':	// weekday
					case 'A':
						// we'll apply this element later, if still necessary
						if (!dateMask.IsComplete())
							computeRelativeUnit(*element, tm, _flags);
						break;
					case 'b':	// month
					case 'B':
						tm.tm_mon = element->value - 1;
						break;
					case 'p':	// meridian
						tm.tm_sec += element->value;
						break;
					case 'z':	// time zone
					case 'Z':
						tm.tm_sec -= element->value + timezone;
						break;
					case 'T':	// time unit
						if ((element->flags & FLAG_NOW) != 0) {
							*_flags = PARSEDATE_MINUTE_RELATIVE_TIME
								| PARSEDATE_RELATIVE_TIME;
							break;
						}

						computeRelativeUnit(*element, tm, _flags);
						break;
				}
				break;
		}

		// format matched at this point, check next element
		format++;
		if (format[0] == ']')
			format++;

		position++;
		element++;
	}

	return mktime(&tm);
}


// #pragma mark - public API


time_t
parsedate_etc(const char* dateString, time_t now, int* _flags)
{
	// preparse date string so that it can be easily compared to our formats

	parsed_element elements[MAX_ELEMENTS];

	if (preparseDate(dateString, elements) < B_OK) {
		*_flags = PARSEDATE_INVALID_DATE;
		return B_ERROR;
	}

#if TRACE_PARSEDATE
	printf("parsedate(\"%s\", now %ld)\n", dateString, now);
	for (int32 index = 0; elements[index].type != TYPE_END; index++) {
		parsed_element e = elements[index];

		printf("  %ld: type = %u, base_type = %u, unit = %u, flags = %u, "
			"modifier = %u, value = %Ld (%s)\n", index, e.type, e.base_type,
			e.unit, e.flags, e.modifier, e.value,
			e.value_type == VALUE_NUMERICAL ? "numerical"
				: (e.value_type == VALUE_STRING ? "string" : "char"));
	}
#endif

	bool optional[MAX_ELEMENTS];

	for (int32 index = 0; sFormatsTable[index]; index++) {
		// test if this format matches our date string

		const char* format = sFormatsTable[index];
		uint32 position = 0;
		DateMask dateMask;

		parsed_element* element = elements;
		while (element->type != TYPE_END) {
			// skip whitespace
			while (isspace(format[0]))
				format++;

			if (format[0] == '[' && format[2] == ']') {
				optional[position] = true;
				format++;
			} else
				optional[position] = false;

			switch (element->value_type) {
				case VALUE_CHAR:
					// check the allowed single characters

					switch (element->type) {
						case TYPE_DOT:
							if (format[0] != '.')
								goto next_format;
							break;
						case TYPE_DASH:
							if (format[0] != '-')
								goto next_format;
							break;
						case TYPE_COMMA:
							if (format[0] != ',')
								goto next_format;
							break;
						case TYPE_COLON:
							if (format[0] != ':')
								goto next_format;
							break;
						default:
							goto next_format;
					}
					break;

				case VALUE_NUMERICAL:
					// make sure that unit types are respected
					if (element->type == TYPE_UNIT && format[0] != 'T')
						goto next_format;

					switch (format[0]) {
						case 'd':
							if (element->value > 31)
								goto next_format;

							dateMask.Set(TYPE_DAY);
							break;
						case 'm':
							if (element->value > 12)
								goto next_format;

							dateMask.Set(TYPE_MONTH);
							break;
						case 'H':
						case 'I':
							if (element->value > 24)
								goto next_format;

							dateMask.Set(TYPE_HOUR);
							break;
						case 'M':
							dateMask.Set(TYPE_MINUTE);
						case 'S':
							if (element->value > 59)
								goto next_format;

							break;
						case 'y':
						case 'Y':
							// accept all values
							break;
						case 'z':	// time zone
						case 'Z':
							// a numerical timezone must be introduced by '+'
							// or '-' and it must not exceed 2399
							if ((element->modifier != MODIFY_MINUS
									&& element->modifier != MODIFY_PLUS)
								|| element->value > 2399)
								goto next_format;
							break;
						case 'T':
							dateMask.Set(TYPE_UNIT);
							break;
						case '-':
							if ((element->flags & FLAG_HAS_DASH) != 0) {
								element--;	// consider this element again
								break;
							}
							// supposed to fall through
						default:
							goto next_format;
					}
					break;

				case VALUE_STRING:
					switch (format[0]) {
						case 'a':	// weekday
						case 'A':
							if (element->type != TYPE_WEEKDAY)
								goto next_format;
							break;
						case 'b':	// month
						case 'B':
							if (element->type != TYPE_MONTH)
								goto next_format;

							dateMask.Set(TYPE_MONTH);
							break;
						case 'p':	// meridian
							if (element->type != TYPE_MERIDIAN)
								goto next_format;
							break;
						case 'z':	// time zone
						case 'Z':
							if (element->type != TYPE_TIME_ZONE)
								goto next_format;
							break;
						case 'T':	// time unit
							if (element->type != TYPE_UNIT)
								goto next_format;

							dateMask.Set(TYPE_UNIT);
							break;
						default:
							goto next_format;
					}
					break;
			}

			// format matched at this point, check next element
			if (optional[position])
				format++;
			format++;
			position++;
			element++;
			continue;

		next_format:
			// format didn't match element - let's see if the current
			// one is only optional (in which case we can continue)
			if (!optional[position])
				goto skip_format;

			optional[position] = false;
			format += 2;
			position++;
				// skip the closing ']'
		}

		// check if the format is already empty (since we reached our last
		// element)
		while (format[0]) {
			if (format[0] == '[')
				format += 3;
			else if (isspace(format[0]))
				format++;
			else
				break;
		}
		if (format[0])
			goto skip_format;

		// made it here? then we seem to have found our guy

		return computeDate(sFormatsTable[index], optional, elements, now,
			dateMask, _flags);

	skip_format:
		// check if the next format has the same beginning as the skipped one,
		// and if so, skip that one, too.

		int32 length = format + 1 - sFormatsTable[index];

		while (sFormatsTable[index + 1]
			&& !strncmp(sFormatsTable[index], sFormatsTable[index + 1], length))
			index++;
	}

	// didn't find any matching formats
	return B_ERROR;
}


time_t
parsedate(const char* dateString, time_t now)
{
	int flags = 0;

	return parsedate_etc(dateString, now, &flags);
}


void
set_dateformats(const char** table)
{
	sFormatsTable = table ? table : kFormatsTable;
}


const char**
get_dateformats(void)
{
	return const_cast<const char**>(sFormatsTable);
}

