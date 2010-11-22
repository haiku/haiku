/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include <ctype.h>
#include <limits.h>

#include <PosixCtype.h>
#include <PosixLocaleConv.h>

#ifndef _KERNEL_MODE
#include <langinfo.h>
#include <PosixLanginfo.h>
#include <PosixLCTimeInfo.h>
#endif


/*
 * The values below initialize the structs for the "C"/"POSIX" locale
 * which is the default locale (and the only one supported without any locale
 * backend).
 */


namespace BPrivate {
namespace Libroot {


/*
 * The following arrays have 384 elements where the elements at index -128..-2
 * mirror the elements at index 128..255 (to protect against invocations of
 * ctype macros with negative character values).
 * The element at index -1 is a dummy element containing the neutral/identity
 * value used when the array is accessed as in 'isblank(EOF)' (i.e. with
 * index -1).
 */
const unsigned short gPosixClassInfo[384] = {
	/*-128 */	0, 0, 0, 0, 0, 0, 0, 0,
	/*-120 */	0, 0, 0, 0, 0, 0, 0, 0,
	/*-112 */	0, 0, 0, 0, 0, 0, 0, 0,
	/*-104 */	0, 0, 0, 0, 0, 0, 0, 0,
	/* -96 */	0, 0, 0, 0, 0, 0, 0, 0,
	/* -88 */	0, 0, 0, 0, 0, 0, 0, 0,
	/* -80 */	0, 0, 0, 0, 0, 0, 0, 0,
	/* -72 */	0, 0, 0, 0, 0, 0, 0, 0,
	/* -64 */	0, 0, 0, 0, 0, 0, 0, 0,
	/* -56 */	0, 0, 0, 0, 0, 0, 0, 0,
	/* -48 */	0, 0, 0, 0, 0, 0, 0, 0,
	/* -40 */	0, 0, 0, 0, 0, 0, 0, 0,
	/* -32 */	0, 0, 0, 0, 0, 0, 0, 0,
	/* -24 */	0, 0, 0, 0, 0, 0, 0, 0,
	/* -16 */	0, 0, 0, 0, 0, 0, 0, 0,
	/*  -8 */	0, 0, 0, 0, 0, 0, 0,
	/*  -1 */   0,	// neutral value
	/*   0 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
	/*   8 */	_IScntrl, _ISblank|_IScntrl|_ISspace, _IScntrl|_ISspace, _IScntrl|_ISspace, _IScntrl|_ISspace, _IScntrl|_ISspace, _IScntrl, _IScntrl,
	/*  16 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
	/*  24 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
	/*  32 */	_ISblank|_ISspace|_ISprint, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph,
	/*  40 */	_ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph,
	/*  48 */	_ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph,
	/*  56 */	_ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph,
	/*  64 */	_ISpunct|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph,
	/*  72 */	_ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph,
	/*  80 */	_ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph,
	/*  88 */	_ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph,
	/*  96 */	_ISpunct|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph,
	/* 104 */	_ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph,
	/* 112 */	_ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph,
	/* 120 */	_ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _IScntrl,
	/* 128 */	0, 0, 0, 0, 0, 0, 0, 0,
	/* 136 */	0, 0, 0, 0, 0, 0, 0, 0,
	/* 144 */	0, 0, 0, 0, 0, 0, 0, 0,
	/* 152 */	0, 0, 0, 0, 0, 0, 0, 0,
	/* 160 */	0, 0, 0, 0, 0, 0, 0, 0,
	/* 168 */	0, 0, 0, 0, 0, 0, 0, 0,
	/* 176 */	0, 0, 0, 0, 0, 0, 0, 0,
	/* 184 */	0, 0, 0, 0, 0, 0, 0, 0,
	/* 192 */	0, 0, 0, 0, 0, 0, 0, 0,
	/* 200 */	0, 0, 0, 0, 0, 0, 0, 0,
	/* 208 */	0, 0, 0, 0, 0, 0, 0, 0,
	/* 216 */	0, 0, 0, 0, 0, 0, 0, 0,
	/* 224 */	0, 0, 0, 0, 0, 0, 0, 0,
	/* 232 */	0, 0, 0, 0, 0, 0, 0, 0,
	/* 240 */	0, 0, 0, 0, 0, 0, 0, 0,
	/* 248 */	0, 0, 0, 0, 0, 0, 0, 0
};

const int gPosixToLowerMap[384] = {
	/*-128 */	128, 129, 130, 131, 132, 133, 134, 135,
	/*-120 */	136, 137, 138, 139, 140, 141, 142, 143,
	/*-112 */	144, 145, 146, 147, 148, 149, 150, 151,
	/*-104 */	152, 153, 154, 155, 156, 157, 158, 159,
	/* -96 */	160, 161, 162, 163, 164, 165, 166, 167,
	/* -88 */	168, 169, 170, 171, 172, 173, 174, 175,
	/* -80 */	176, 177, 178, 179, 180, 181, 182, 183,
	/* -72 */	184, 185, 186, 187, 188, 189, 190, 191,
	/* -64 */	192, 193, 194, 195, 196, 197, 198, 199,
	/* -56 */	200, 201, 202, 203, 204, 205, 206, 207,
	/* -48 */	208, 209, 210, 211, 212, 213, 214, 215,
	/* -40 */	216, 217, 218, 219, 220, 221, 222, 223,
	/* -32 */	224, 225, 226, 227, 228, 229, 230, 231,
	/* -24 */	232, 233, 234, 235, 236, 237, 238, 239,
	/* -16 */	240, 241, 242, 243, 244, 245, 246, 247,
	/*  -8 */	248, 249, 250, 251, 252, 253, 254,
	/*  -1 */    -1,	// identity value
	/*   0 */	  0,   1,   2,   3,   4,   5,   6,   7,
	/*   8 */	  8,   9,  10,  11,  12,  13,  14,  15,
	/*  16 */	 16,  17,  18,  19,  20,  21,  22,  23,
	/*  24 */	 24,  25,  26,  27,  28,  29,  30,  31,
	/*  32 */	 32,  33,  34,  35,  36,  37,  38,  39,
	/*  40 */	 40,  41,  42,  43,  44,  45,  46,  47,
	/*  48 */	'0', '1', '2', '3', '4', '5', '6', '7',
	/*  56 */	'8', '9',  58,  59,  60,  61,  62,  63,
	/*  64 */	 64, 'a', 'b', 'c', 'd', 'e', 'f', 'g',
	/*  72 */	'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
	/*  80 */	'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
	/*  88 */	'x', 'y', 'z',  91,  92,  93,  94,  95,
	/*  96 */	 96, 'a', 'b', 'c', 'd', 'e', 'f', 'g',
	/* 104 */	'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
	/* 112 */	'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
	/* 120 */	'x', 'y', 'z', 123, 124, 125, 126, 127,
	/* 128 */	128, 129, 130, 131, 132, 133, 134, 135,
	/* 136 */	136, 137, 138, 139, 140, 141, 142, 143,
	/* 144 */	144, 145, 146, 147, 148, 149, 150, 151,
	/* 152 */	152, 153, 154, 155, 156, 157, 158, 159,
	/* 160 */	160, 161, 162, 163, 164, 165, 166, 167,
	/* 168 */	168, 169, 170, 171, 172, 173, 174, 175,
	/* 176 */	176, 177, 178, 179, 180, 181, 182, 183,
	/* 184 */	184, 185, 186, 187, 188, 189, 190, 191,
	/* 192 */	192, 193, 194, 195, 196, 197, 198, 199,
	/* 200 */	200, 201, 202, 203, 204, 205, 206, 207,
	/* 208 */	208, 209, 210, 211, 212, 213, 214, 215,
	/* 216 */	216, 217, 218, 219, 220, 221, 222, 223,
	/* 224 */	224, 225, 226, 227, 228, 229, 230, 231,
	/* 232 */	232, 233, 234, 235, 236, 237, 238, 239,
	/* 240 */	240, 241, 242, 243, 244, 245, 246, 247,
	/* 248 */	248, 249, 250, 251, 252, 253, 254, 255
};


const int gPosixToUpperMap[384] = {
	/*-128 */	128, 129, 130, 131, 132, 133, 134, 135,
	/*-120 */	136, 137, 138, 139, 140, 141, 142, 143,
	/*-112 */	144, 145, 146, 147, 148, 149, 150, 151,
	/*-104 */	152, 153, 154, 155, 156, 157, 158, 159,
	/* -96 */	160, 161, 162, 163, 164, 165, 166, 167,
	/* -88 */	168, 169, 170, 171, 172, 173, 174, 175,
	/* -80 */	176, 177, 178, 179, 180, 181, 182, 183,
	/* -72 */	184, 185, 186, 187, 188, 189, 190, 191,
	/* -64 */	192, 193, 194, 195, 196, 197, 198, 199,
	/* -56 */	200, 201, 202, 203, 204, 205, 206, 207,
	/* -48 */	208, 209, 210, 211, 212, 213, 214, 215,
	/* -40 */	216, 217, 218, 219, 220, 221, 222, 223,
	/* -32 */	224, 225, 226, 227, 228, 229, 230, 231,
	/* -24 */	232, 233, 234, 235, 236, 237, 238, 239,
	/* -16 */	240, 241, 242, 243, 244, 245, 246, 247,
	/*  -8 */	248, 249, 250, 251, 252, 253, 254,
	/*  -1 */    -1,	// identity value
	/*   0 */	  0,   1,   2,   3,   4,   5,   6,   7,
	/*   8 */	  8,   9,  10,  11,  12,  13,  14,  15,
	/*  16 */	 16,  17,  18,  19,  20,  21,  22,  23,
	/*  24 */	 24,  25,  26,  27,  28,  29,  30,  31,
	/*  32 */	 32,  33,  34,  35,  36,  37,  38,  39,
	/*  40 */	 40,  41,  42,  43,  44,  45,  46,  47,
	/*  48 */	'0', '1', '2', '3', '4', '5', '6', '7',
	/*  56 */	'8', '9',  58,  59,  60,  61,  62,  63,
	/*  64 */	 64, 'A', 'B', 'C', 'D', 'E', 'F', 'G',
	/*  72 */	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	/*  80 */	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
	/*  88 */	'X', 'Y', 'Z',  91,  92,  93,  94,  95,
	/*  96 */	 96, 'A', 'B', 'C', 'D', 'E', 'F', 'G',
	/* 104 */	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	/* 112 */	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
	/* 120 */	'X', 'Y', 'Z', 123, 124, 125, 126, 127,
	/* 128 */	128, 129, 130, 131, 132, 133, 134, 135,
	/* 136 */	136, 137, 138, 139, 140, 141, 142, 143,
	/* 144 */	144, 145, 146, 147, 148, 149, 150, 151,
	/* 152 */	152, 153, 154, 155, 156, 157, 158, 159,
	/* 160 */	160, 161, 162, 163, 164, 165, 166, 167,
	/* 168 */	168, 169, 170, 171, 172, 173, 174, 175,
	/* 176 */	176, 177, 178, 179, 180, 181, 182, 183,
	/* 184 */	184, 185, 186, 187, 188, 189, 190, 191,
	/* 192 */	192, 193, 194, 195, 196, 197, 198, 199,
	/* 200 */	200, 201, 202, 203, 204, 205, 206, 207,
	/* 208 */	208, 209, 210, 211, 212, 213, 214, 215,
	/* 216 */	216, 217, 218, 219, 220, 221, 222, 223,
	/* 224 */	224, 225, 226, 227, 228, 229, 230, 231,
	/* 232 */	232, 233, 234, 235, 236, 237, 238, 239,
	/* 240 */	240, 241, 242, 243, 244, 245, 246, 247,
	/* 248 */	248, 249, 250, 251, 252, 253, 254, 255
};


struct lconv gPosixLocaleConv = {
	(char*)".",	// decimal point
	(char*)"",	// thousands separator
	(char*)"",	// grouping
	(char*)"",	// international currency symbol
	(char*)"",	// local currency symbol
	(char*)"",	// monetary decimal point
	(char*)"",	// monetary thousands separator
	(char*)"",	// monetary grouping
	(char*)"",	// positive sign
	(char*)"",	// negative sign
	CHAR_MAX,	// int_frac_digits
	CHAR_MAX,	// frac_digits
	CHAR_MAX,	// p_cs_precedes
	CHAR_MAX,	// p_sep_by_space
	CHAR_MAX,	// n_cs_precedes
	CHAR_MAX,	// n_sep_by_space
	CHAR_MAX,	// p_sign_posn
	CHAR_MAX,	// n_sign_posn
	CHAR_MAX,	// int_p_cs_precedes
	CHAR_MAX,	// int_p_sep_by_space
	CHAR_MAX,	// int_n_cs_precedes
	CHAR_MAX,	// int_n_sep_by_space
	CHAR_MAX,	// int_p_sign_posn
	CHAR_MAX	// int_n_sign_posn
};


#ifndef _KERNEL_MODE

const struct lc_time_t gPosixLCTimeInfo = {
	{
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	},
	{
		"January", "February", "March", "April", "May", "June",
		"July", "August", "September", "October", "November", "December"
	},
	{
		"Sun", "Mon", "Tue", "Wed",	"Thu", "Fri", "Sat"
	},
	{
		"Sunday", "Monday", "Tuesday", "Wednesday",	"Thursday", "Friday",
		"Saturday"
	},
	"%H:%M:%S",
	"%m/%d/%y",
	"%a %b %e %H:%M:%S %Y",
	"AM",
	"PM",
	"%a %b %e %H:%M:%S %Z %Y",
	{
		"January", "February", "March", "April", "May", "June",
		"July", "August", "September", "October", "November", "December"
	},
	"md",
	"%I:%M:%S %p"
};


const char* gPosixLanginfo[_NL_LANGINFO_LAST] = {
	"US-ASCII",
	gPosixLCTimeInfo.c_fmt,
	gPosixLCTimeInfo.x_fmt,
	gPosixLCTimeInfo.X_fmt,
	gPosixLCTimeInfo.ampm_fmt,
	gPosixLCTimeInfo.am,
	gPosixLCTimeInfo.pm,

	gPosixLCTimeInfo.weekday[0],
	gPosixLCTimeInfo.weekday[1],
	gPosixLCTimeInfo.weekday[2],
	gPosixLCTimeInfo.weekday[3],
	gPosixLCTimeInfo.weekday[4],
	gPosixLCTimeInfo.weekday[5],
	gPosixLCTimeInfo.weekday[6],

	gPosixLCTimeInfo.wday[0],
	gPosixLCTimeInfo.wday[1],
	gPosixLCTimeInfo.wday[2],
	gPosixLCTimeInfo.wday[3],
	gPosixLCTimeInfo.wday[4],
	gPosixLCTimeInfo.wday[5],
	gPosixLCTimeInfo.wday[6],

	gPosixLCTimeInfo.month[0],
	gPosixLCTimeInfo.month[1],
	gPosixLCTimeInfo.month[2],
	gPosixLCTimeInfo.month[3],
	gPosixLCTimeInfo.month[4],
	gPosixLCTimeInfo.month[5],
	gPosixLCTimeInfo.month[6],
	gPosixLCTimeInfo.month[7],
	gPosixLCTimeInfo.month[8],
	gPosixLCTimeInfo.month[9],
	gPosixLCTimeInfo.month[10],
	gPosixLCTimeInfo.month[11],

	gPosixLCTimeInfo.mon[0],
	gPosixLCTimeInfo.mon[1],
	gPosixLCTimeInfo.mon[2],
	gPosixLCTimeInfo.mon[3],
	gPosixLCTimeInfo.mon[4],
	gPosixLCTimeInfo.mon[5],
	gPosixLCTimeInfo.mon[6],
	gPosixLCTimeInfo.mon[7],
	gPosixLCTimeInfo.mon[8],
	gPosixLCTimeInfo.mon[9],
	gPosixLCTimeInfo.mon[10],
	gPosixLCTimeInfo.mon[11],

	"%EC, %Ey, %EY",
	"%Ex",
	"%Ec",
	"%EX",
	"%O",

	gPosixLocaleConv.decimal_point,
	gPosixLocaleConv.thousands_sep,

	"^[yY]",
	"^[nN]",

	gPosixLocaleConv.currency_symbol
};

#endif // !_KERNEL_MODE


}	// namespace Libroot
}	// namespace BPrivate


const unsigned short* __ctype_b = &BPrivate::Libroot::gPosixClassInfo[128];
const int* __ctype_tolower = &BPrivate::Libroot::gPosixToLowerMap[128];
const int* __ctype_toupper = &BPrivate::Libroot::gPosixToUpperMap[128];
