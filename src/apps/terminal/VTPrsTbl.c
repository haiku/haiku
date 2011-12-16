/*
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "VTparse.h"

#define USE_MBCS
#define USE_ISO2022

// #pragma mark UTF8 coding ground table
int gUTF8GroundTable[] =
{
/*	NUL		SOH		STX		ETX	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	EOT		ENQ		ACK		BEL	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_BELL,
/*	BS		HT		NL		VT	*/
CASE_BS,
CASE_TAB,
CASE_LF,
CASE_LF, /* CASE_UP*/
/*	NP		CR		SO		SI	*/
CASE_LF, /* CASE_IGNORE*/
CASE_CR,
CASE_SO,
CASE_SI,
/*	DLE		DC1		DC2		DC3	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	DC4		NAK		SYN		ETB	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	CAN		EM		SUB		ESC	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_ESC,
/*	FS		GS		RS		US	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	SP		!		"		#	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	$		%		&		'	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	(		)		*		+	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	,		-		.		/	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	0		1		2		3	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	4		5		6		7	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	8		9		:		;	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	<		=		>		?	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT, 
/*	@		A		B		C	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	D		E		F		G	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	H		I		J		K	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	L		M		N		O	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	P		Q		R		S	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	T		U		V		W	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	X		Y		Z		[	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	\		]		^		_	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	`		a		b		c	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	d		e		f		g	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	h		i		j		k	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	l		m		n		o	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	p		q		r		s	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	t		u		v		w	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	x		y		z		{	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	|		}		~		DEL	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_IGNORE,
/*      0x80            0x81            0x82            0x83    */
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0x84            0x85            0x86            0x87    */
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0x88            0x89            0x8a            0x8b    */
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0x8c            0x8d            0x8e            0x8f    */
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0x90            0x91            0x92            0x93    */
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0x94            0x95            0x96            0x97    */
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0x99            0x99            0x9a            0x9b    */
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0x9c            0x9d            0x9e            0x9f    */
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0xa0		0xa1		0xa2		0xa3	*/
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0xa4		0xa5		0xa6		0xa7	*/
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0xa8		0xa9		0xaa		0xab	*/
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0xac		0xad		0xae		0xaf	*/
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0xb0		0xb1		0xb2		0xb3	*/
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0xb4		0xb5		0xb6		0xb7	*/
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0xb8		0xb9		0xba		0xbb	*/
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0xbc		0xbd		0xbe		0xbf	*/
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0xc0		0xc1		0xc2		0xc3	*/
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
/*      0xc4		0xc5		0xc6		0xc7	*/
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
/*      0xc8		0xc9		0xca		0xcb	*/
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
/*      0xcc		0xcd		0xce		0xcf	*/
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
/*      0xd0		0xd1		0xd2		0xd3	*/
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
/*      0xd4		0xd5		0xd6		0xd7	*/
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
/*      0xd8		0xd9		0xda		0xdb	*/
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
/*      0xdc		0xdd		0xde		0xdf	*/
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
/*      0xe0		0xe1		0xe2		0xe3	*/
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
/*      0xe4		0xe5		0xe6		0xe7	*/
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
/*      0xe8		0xe9		0xea		0xeb	*/
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
/*      0xec		0xed		0xee		0xef	*/
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
/*      0xf0		0xf1		0xf2		0xf3	*/
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
/*      0xf4		0xf5		0xf6		0xf7	*/
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
/*      0xf8		0xf9		0xfa		0xfb	*/
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
/*      0xfc		0xfd		0xfe		0xff	*/
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
};

// #pragma mark charset 96 table
int gCS96GroundTable[] =
{
/*	NUL		SOH		STX		ETX	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	EOT		ENQ		ACK		BEL	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_BELL,
/*	BS		HT		NL		VT	*/
CASE_BS,
CASE_TAB,
CASE_LF,
CASE_LF, /* CASE_UP*/
/*	NP		CR		SO		SI	*/
CASE_LF, /* CASE_IGNORE*/
CASE_CR,
CASE_SO,
CASE_SI,
/*	DLE		DC1		DC2		DC3	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	DC4		NAK		SYN		ETB	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	CAN		EM		SUB		ESC	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_ESC,
/*	FS		GS		RS		US	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	SP		!		"		#	*/
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
/*	$		%		&		'	*/
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
/*	(		)		*		+	*/
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
/*	,		-		.		/	*/
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
/*	0		1		2		3	*/
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
/*	4		5		6		7	*/
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
/*	8		9		:		;	*/
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
/*	<		=		>		?	*/
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96, 
/*	@		A		B		C	*/
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
/*	D		E		F		G	*/
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
/*	H		I		J		K	*/
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
/*	L		M		N		O	*/
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
/*	P		Q		R		S	*/
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
/*	T		U		V		W	*/
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
/*	X		Y		Z		[	*/
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
/*	\		]		^		_	*/
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
/*	`		a		b		c	*/
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
/*	d		e		f		g	*/
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
/*	h		i		j		k	*/
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
/*	l		m		n		o	*/
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
/*	p		q		r		s	*/
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
/*	t		u		v		w	*/
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
/*	x		y		z		{	*/
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
/*	|		}		~		DEL	*/
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
CASE_PRINT_CS96,
/*      0x80            0x81            0x82            0x83    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x84            0x85            0x86            0x87    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x88            0x89            0x8a            0x8b    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x8c            0x8d            0x8e            0x8f    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x90            0x91            0x92            0x93    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x94            0x95            0x96            0x97    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x99            0x99            0x9a            0x9b    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x9c            0x9d            0x9e            0x9f    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0xa0		0xa1		0xa2		0xa3	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0xa4		0xa5		0xa6		0xa7	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0xa8		0xa9		0xaa		0xab	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0xac		0xad		0xae		0xaf	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0xb0		0xb1		0xb2		0xb3	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0xb4		0xb5		0xb6		0xb7	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0xb8		0xb9		0xba		0xbb	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0xbc		0xbd		0xbe		0xbf	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0xc0		0xc1		0xc2		0xc3	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0xc4		0xc5		0xc6		0xc7	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0xc8		0xc9		0xca		0xcb	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0xcc		0xcd		0xce		0xcf	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0xd0		0xd1		0xd2		0xd3	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0xd4		0xd5		0xd6		0xd7	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0xd8		0xd9		0xda		0xdb	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0xdc		0xdd		0xde		0xdf	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0xe0		0xe1		0xe2		0xe3	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0xe4		0xe5		0xe6		0xe7	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0xe8		0xe9		0xea		0xeb	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0xec		0xed		0xee		0xef	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0xf0		0xf1		0xf2		0xf3	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0xf4		0xf5		0xf6		0xf7	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0xf8		0xf9		0xfa		0xfb	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0xfc		0xfd		0xfe		0xff	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
};

// #pragma mark ISO8859 table
int gISO8859GroundTable[] =
{
/*	NUL		SOH		STX		ETX	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	EOT		ENQ		ACK		BEL	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_BELL,
/*	BS		HT		NL		VT	*/
CASE_BS,
CASE_TAB,
CASE_LF,
CASE_LF, /*CASE_UP,*/
/*	NP		CR		SO		SI	*/
CASE_LF, /*CASE_IGNORE,*/
CASE_CR,
CASE_SO,
CASE_SI,
/*	DLE		DC1		DC2		DC3	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	DC4		NAK		SYN		ETB	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	CAN		EM		SUB		ESC	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_ESC,
/*	FS		GS		RS		US	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	SP		!		"		#	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	$		%		&		'	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	(		)		*		+	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	,		-		.		/	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	0		1		2		3	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	4		5		6		7	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	8		9		:		;	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	<		=		>		?	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT, 
/*	@		A		B		C	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	D		E		F		G	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	H		I		J		K	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	L		M		N		O	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	P		Q		R		S	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	T		U		V		W	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	X		Y		Z		[	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	\		]		^		_	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	`		a		b		c	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	d		e		f		g	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	h		i		j		k	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	l		m		n		o	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	p		q		r		s	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	t		u		v		w	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	x		y		z		{	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	|		}		~		DEL	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*      0x80            0x81            0x82            0x83    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x84            0x85            0x86            0x87    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x88            0x89            0x8a            0x8b    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x8c            0x8d            0x8e            0x8f    */
CASE_IGNORE,
CASE_IGNORE,
CASE_SS2,
CASE_SS3,
/*      0x90            0x91            0x92            0x93    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x94            0x95            0x96            0x97    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x99            0x99            0x9a            0x9b    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_CSI_STATE,
/*      0x9c            0x9d            0x9e            0x9f    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      nobreakspace    exclamdown      cent            sterling        */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      currency        yen             brokenbar       section         */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      diaeresis       copyright       ordfeminine     guillemotleft   */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      notsign         hyphen          registered      macron          */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      degree          plusminus       twosuperior     threesuperior   */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      acute           mu              paragraph       periodcentered  */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      cedilla         onesuperior     masculine       guillemotright  */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      onequarter      onehalf         threequarters   questiondown    */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      Agrave          Aacute          Acircumflex     Atilde          */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      Adiaeresis      Aring           AE              Ccedilla        */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      Egrave          Eacute          Ecircumflex     Ediaeresis      */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      Igrave          Iacute          Icircumflex     Idiaeresis      */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      Eth             Ntilde          Ograve          Oacute          */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      Ocircumflex     Otilde          Odiaeresis      multiply        */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      Ooblique        Ugrave          Uacute          Ucircumflex     */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      Udiaeresis      Yacute          Thorn           ssharp          */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      agrave          aacute          acircumflex     atilde          */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      adiaeresis      aring           ae              ccedilla        */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      egrave          eacute          ecircumflex     ediaeresis      */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      igrave          iacute          icircumflex     idiaeresis      */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      eth             ntilde          ograve          oacute          */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      ocircumflex     otilde          odiaeresis      division        */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      oslash          ugrave          uacute          ucircumflex     */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      udiaeresis      yacute          thorn           ydiaeresis      */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
};

// #pragma mark WinCP table (Windows cp1252, cp1251, koi-8r etc.)
int gWinCPGroundTable[] =
{
/*	NUL		SOH		STX		ETX	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	EOT		ENQ		ACK		BEL	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_BELL,
/*	BS		HT		NL		VT	*/
CASE_BS,
CASE_TAB,
CASE_LF,
CASE_LF, /*CASE_UP,*/
/*	NP		CR		SO		SI	*/
CASE_LF, /*CASE_IGNORE,*/
CASE_CR,
CASE_SO,
CASE_SI,
/*	DLE		DC1		DC2		DC3	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	DC4		NAK		SYN		ETB	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	CAN		EM		SUB		ESC	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_ESC,
/*	FS		GS		RS		US	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	SP		!		"		#	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	$		%		&		'	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	(		)		*		+	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	,		-		.		/	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	0		1		2		3	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	4		5		6		7	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	8		9		:		;	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	<		=		>		?	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT, 
/*	@		A		B		C	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	D		E		F		G	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	H		I		J		K	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	L		M		N		O	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	P		Q		R		S	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	T		U		V		W	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	X		Y		Z		[	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	\		]		^		_	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	`		a		b		c	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	d		e		f		g	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	h		i		j		k	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	l		m		n		o	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	p		q		r		s	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	t		u		v		w	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	x		y		z		{	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	|		}		~		DEL	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT, //TODO???
/*      0x80            0x81            0x82            0x83    */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      0x84            0x85            0x86            0x87    */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      0x88            0x89            0x8a            0x8b    */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      0x8c            0x8d            0x8e            0x8f    */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      0x90            0x91            0x92            0x93    */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      0x94            0x95            0x96            0x97    */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      0x99            0x99            0x9a            0x9b    */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      0x9c            0x9d            0x9e            0x9f    */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      nobreakspace    exclamdown      cent            sterling        */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      currency        yen             brokenbar       section         */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      diaeresis       copyright       ordfeminine     guillemotleft   */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      notsign         hyphen          registered      macron          */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      degree          plusminus       twosuperior     threesuperior   */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      acute           mu              paragraph       periodcentered  */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      cedilla         onesuperior     masculine       guillemotright  */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      onequarter      onehalf         threequarters   questiondown    */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      Agrave          Aacute          Acircumflex     Atilde          */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      Adiaeresis      Aring           AE              Ccedilla        */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      Egrave          Eacute          Ecircumflex     Ediaeresis      */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      Igrave          Iacute          Icircumflex     Idiaeresis      */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      Eth             Ntilde          Ograve          Oacute          */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      Ocircumflex     Otilde          Odiaeresis      multiply        */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      Ooblique        Ugrave          Uacute          Ucircumflex     */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      Udiaeresis      Yacute          Thorn           ssharp          */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      agrave          aacute          acircumflex     atilde          */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      adiaeresis      aring           ae              ccedilla        */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      egrave          eacute          ecircumflex     ediaeresis      */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      igrave          iacute          icircumflex     idiaeresis      */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      eth             ntilde          ograve          oacute          */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      ocircumflex     otilde          odiaeresis      division        */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      oslash          ugrave          uacute          ucircumflex     */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
/*      udiaeresis      yacute          thorn           ydiaeresis      */
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
CASE_PRINT_GR,
};

// #pragma mark ESC [ - CSI table
int gCsiTable[] =
{
/*	NUL		SOH		STX		ETX	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	EOT		ENQ		ACK		BEL	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_BELL,
/*	BS		HT		NL		VT	*/
CASE_BS,
CASE_TAB,
CASE_VMOT,
CASE_VMOT,
/*	NP		CR		SO		SI	*/
CASE_VMOT,
CASE_CR,
CASE_SO,
CASE_SI,
/*	DLE		DC1		DC2		DC3	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	DC4		NAK		SYN		ETB	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	CAN		EM		SUB		ESC	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_ESC,
/*	FS		GS		RS		US	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	SP		!		"		#	*/
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
/*	$		%		&		'	*/
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
/*	(		)		*		+	*/
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
/*	,		-		.		/	*/
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
/*	0		1		2		3	*/
CASE_ESC_DIGIT,
CASE_ESC_DIGIT,
CASE_ESC_DIGIT,
CASE_ESC_DIGIT,
/*	4		5		6		7	*/
CASE_ESC_DIGIT,
CASE_ESC_DIGIT,
CASE_ESC_DIGIT,
CASE_ESC_DIGIT,
/*	8		9		:		;	*/
CASE_ESC_DIGIT,
CASE_ESC_DIGIT,
CASE_IGNORE,
CASE_ESC_SEMI,
/*	<		=		>		?	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_DEC_STATE,
/*	@		A		B		C	*/
CASE_ICH,
CASE_CUU,
CASE_CUD,
CASE_CUF,
/*	D		E		F		G	*/
CASE_CUB,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_HPA,
/*	H		I		J		K	*/
CASE_CUP,
CASE_GROUND_STATE,
CASE_ED,
CASE_EL,
/*	L		M		N		O	*/
CASE_IL,
CASE_DL,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	P		Q		R		S	*/
CASE_DCH,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_SU,
/*	T		U		V		W	*/
CASE_SD,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	X		Y		Z		[	*/
CASE_ECH,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	\		]		^		_	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	`		a		b		c	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_DA1,
/*	d		e		f		g	*/
CASE_VPA,
CASE_GROUND_STATE,
CASE_CUP,
CASE_TBC,
/*	h		i		j		k	*/
CASE_SET,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	l		m		n		o	*/
CASE_RST,
CASE_SGR,
CASE_CPR,
CASE_GROUND_STATE,
/*	p		q		r		s	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_DECSTBM,
CASE_GROUND_STATE,
/*	t		u		v		w	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	x		y		z		{	*/
CASE_DECREQTPARM,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	|		}		~		DEL	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      0x80            0x81            0x82            0x83    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x84            0x85            0x86            0x87    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x88            0x89            0x8a            0x8b    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x8c            0x8d            0x8e            0x8f    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x90            0x91            0x92            0x93    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x94            0x95            0x96            0x97    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x99            0x99            0x9a            0x9b    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x9c            0x9d            0x9e            0x9f    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      nobreakspace    exclamdown      cent            sterling        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      currency        yen             brokenbar       section         */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      diaeresis       copyright       ordfeminine     guillemotleft   */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      notsign         hyphen          registered      macron          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      degree          plusminus       twosuperior     threesuperior   */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      acute           mu              paragraph       periodcentered  */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      cedilla         onesuperior     masculine       guillemotright  */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      onequarter      onehalf         threequarters   questiondown    */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Agrave          Aacute          Acircumflex     Atilde          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Adiaeresis      Aring           AE              Ccedilla        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Egrave          Eacute          Ecircumflex     Ediaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Igrave          Iacute          Icircumflex     Idiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Eth             Ntilde          Ograve          Oacute          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Ocircumflex     Otilde          Odiaeresis      multiply        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Ooblique        Ugrave          Uacute          Ucircumflex     */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Udiaeresis      Yacute          Thorn           ssharp          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      agrave          aacute          acircumflex     atilde          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      adiaeresis      aring           ae              ccedilla        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      egrave          eacute          ecircumflex     ediaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      igrave          iacute          icircumflex     idiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      eth             ntilde          ograve          oacute          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      ocircumflex     otilde          odiaeresis      division        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      oslash          ugrave          uacute          ucircumflex     */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      udiaeresis      yacute          thorn           ydiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
};

// #pragma mark ESC [ ? - DEC table
int gDecTable[] =
{
/*	NUL		SOH		STX		ETX	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	EOT		ENQ		ACK		BEL	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_BELL,
/*	BS		HT		NL		VT	*/
CASE_BS,
CASE_TAB,
CASE_VMOT,
CASE_VMOT,
/*	NP		CR		SO		SI	*/
CASE_VMOT,
CASE_CR,
CASE_SO,
CASE_SI,
/*	DLE		DC1		DC2		DC3	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	DC4		NAK		SYN		ETB	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	CAN		EM		SUB		ESC	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_ESC,
/*	FS		GS		RS		US	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	SP		!		"		#	*/
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
/*	$		%		&		'	*/
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
/*	(		)		*		+	*/
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
/*	,		-		.		/	*/
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
/*	0		1		2		3	*/
CASE_ESC_DIGIT,
CASE_ESC_DIGIT,
CASE_ESC_DIGIT,
CASE_ESC_DIGIT,
/*	4		5		6		7	*/
CASE_ESC_DIGIT,
CASE_ESC_DIGIT,
CASE_ESC_DIGIT,
CASE_ESC_DIGIT,
/*	8		9		:		;	*/
CASE_ESC_DIGIT,
CASE_ESC_DIGIT,
CASE_IGNORE,
CASE_ESC_SEMI,
/*	<		=		>		?	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	@		A		B		C	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	D		E		F		G	*/
CASE_GROUND_STATE,
#ifdef STATUSLINE
CASE_ERASE_STATUS,
CASE_FROM_STATUS,
#else /* !STATUSLINE */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
#endif /* !STATUSLINE */
CASE_GROUND_STATE,
/*	H		I		J		K	*/
#ifdef STATUSLINE
CASE_HIDE_STATUS,
#else /* !STATUSLINE */
CASE_GROUND_STATE,
#endif /* !STATUSLINE */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	L		M		N		O	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	P		Q		R		S	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
#ifdef STATUSLINE
CASE_SHOW_STATUS,
#else /* !STATUSLINE */
CASE_GROUND_STATE,
#endif /* !STATUSLINE */
/*	T		U		V		W	*/
#ifdef STATUSLINE
CASE_TO_STATUS,
#else /* !STATUSLINE */
CASE_GROUND_STATE,
#endif /* !STATUSLINE */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	X		Y		Z		[	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	\		]		^		_	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	`		a		b		c	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	d		e		f		g	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	h		i		j		k	*/
CASE_DECSET,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	l		m		n		o	*/
CASE_DECRST,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	p		q		r		s	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	t		u		v		w	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	x		y		z		{	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	|		}		~		DEL	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      0x80            0x81            0x82            0x83    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x84            0x85            0x86            0x87    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x88            0x89            0x8a            0x8b    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x8c            0x8d            0x8e            0x8f    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x90            0x91            0x92            0x93    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x94            0x95            0x96            0x97    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x99            0x99            0x9a            0x9b    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x9c            0x9d            0x9e            0x9f    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      nobreakspace    exclamdown      cent            sterling        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      currency        yen             brokenbar       section         */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      diaeresis       copyright       ordfeminine     guillemotleft   */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      notsign         hyphen          registered      macron          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      degree          plusminus       twosuperior     threesuperior   */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      acute           mu              paragraph       periodcentered  */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      cedilla         onesuperior     masculine       guillemotright  */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      onequarter      onehalf         threequarters   questiondown    */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Agrave          Aacute          Acircumflex     Atilde          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Adiaeresis      Aring           AE              Ccedilla        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Egrave          Eacute          Ecircumflex     Ediaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Igrave          Iacute          Icircumflex     Idiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Eth             Ntilde          Ograve          Oacute          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Ocircumflex     Otilde          Odiaeresis      multiply        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Ooblique        Ugrave          Uacute          Ucircumflex     */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Udiaeresis      Yacute          Thorn           ssharp          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      agrave          aacute          acircumflex     atilde          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      adiaeresis      aring           ae              ccedilla        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      egrave          eacute          ecircumflex     ediaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      igrave          iacute          icircumflex     idiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      eth             ntilde          ograve          oacute          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      ocircumflex     otilde          odiaeresis      division        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      oslash          ugrave          uacute          ucircumflex     */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      udiaeresis      yacute          thorn           ydiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
};

// #pragma mark CASE_ESC_IGNORE table
int gEscIgnoreTable[] =
{
/*	NUL		SOH		STX		ETX	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	EOT		ENQ		ACK		BEL	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_BELL,
/*	BS		HT		NL		VT	*/
CASE_BS,
CASE_TAB,
CASE_VMOT,
CASE_VMOT,
/*	NP		CR		SO		SI	*/
CASE_VMOT,
CASE_CR,
CASE_SO,
CASE_SI,
/*	DLE		DC1		DC2		DC3	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	DC4		NAK		SYN		ETB	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	CAN		EM		SUB		ESC	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_ESC,
/*	FS		GS		RS		US	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	SP		!		"		#	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	$		%		&		'	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	(		)		*		+	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	,
	-		.		/	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	0		1		2		3	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	4		5		6		7	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	8		9		:		;	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	<		=		>		?	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	@		A		B		C	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	D		E		F		G	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	H		I		J		K	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	L		M		N		O	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	P		Q		R		S	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	T		U		V		W	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	X		Y		Z		[	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	\		]		^		_	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	`		a		b		c	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	d		e		f		g	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	h		i		j		k	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	l		m		n		o	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	p		q		r		s	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	t		u		v		w	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	x		y		z		{	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	|		}		~		DEL	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      0x80            0x81            0x82            0x83    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x84            0x85            0x86            0x87    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x88            0x89            0x8a            0x8b    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x8c            0x8d            0x8e            0x8f    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x90            0x91            0x92            0x93    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x94            0x95            0x96            0x97    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x99            0x99            0x9a            0x9b    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x9c            0x9d            0x9e            0x9f    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      nobreakspace    exclamdown      cent            sterling        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      currency        yen             brokenbar       section         */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      diaeresis       copyright       ordfeminine     guillemotleft   */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      notsign         hyphen          registered      macron          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      degree          plusminus       twosuperior     threesuperior   */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      acute           mu              paragraph       periodcentered  */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      cedilla         onesuperior     masculine       guillemotright  */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      onequarter      onehalf         threequarters   questiondown    */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Agrave          Aacute          Acircumflex     Atilde          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Adiaeresis      Aring           AE              Ccedilla        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Egrave          Eacute          Ecircumflex     Ediaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Igrave          Iacute          Icircumflex     Idiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Eth             Ntilde          Ograve          Oacute          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Ocircumflex     Otilde          Odiaeresis      multiply        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Ooblique        Ugrave          Uacute          Ucircumflex     */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Udiaeresis      Yacute          Thorn           ssharp          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      agrave          aacute          acircumflex     atilde          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      adiaeresis      aring           ae              ccedilla        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      egrave          eacute          ecircumflex     ediaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      igrave          iacute          icircumflex     idiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      eth             ntilde          ograve          oacute          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      ocircumflex     otilde          odiaeresis      division        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      oslash          ugrave          uacute          ucircumflex     */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      udiaeresis      yacute          thorn           ydiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
};

// #pragma mark ESC table
int gEscTable[] =
{
/*	NUL		SOH		STX		ETX	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	EOT		ENQ		ACK		BEL	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_BELL,
/*	BS		HT		NL		VT	*/
CASE_BS,
CASE_TAB,
CASE_LF,
CASE_LF, /*CASE_UP,*/
/*	NP		CR		SO		SI	*/
CASE_LF, /*CASE_IGNORE,*/
CASE_CR,
CASE_SO,
CASE_SI,
/*	DLE		DC1		DC2		DC3	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	DC4		NAK		SYN		ETB	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	CAN		EM		SUB		ESC	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_ESC,
/*	FS		GS		RS		US	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	SP		!		"		#	*/
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_SCR_STATE,
/*	$		%		&		'	*/
#ifdef USE_ISO2022
CASE_MBCS,
#else /* !USE_ISO2022 */
CASE_ESC_IGNORE,
#endif /* !USE_ISO2022 */
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
/*	(		)		*		+	*/
#ifdef USE_ISO2022
CASE_SCS_STATE,
CASE_SCS_STATE,
CASE_SCS_STATE,
CASE_SCS_STATE,
#else /* !USE_ISO2022 */
CASE_SCS0_STATE,
CASE_SCS1_STATE,
CASE_SCS2_STATE,
CASE_SCS3_STATE,
#endif /* !USE_ISO2022 */
/*	,		-		.		/	*/
#ifdef USE_ISO2022
CASE_SCS_STATE,	/* not defined in ISO2022 but used in Mule */
CASE_SCS_STATE,
CASE_SCS_STATE,
CASE_SCS_STATE,
#else /* !USE_ISO2022 */
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
#endif /* !USE_ISO2022 */
/*	0		1		2		3	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	4		5		6		7	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_DECSC,
/*	8		9		:		;	*/
CASE_DECRC,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	<		=		>		?	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	@		A		B		C	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	D		E		F		G	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	H		I		J		K	*/
CASE_HTS,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	L		M		N		O	*/
CASE_GROUND_STATE,
CASE_RI,
CASE_SS2,
CASE_SS3,
/*	P		Q		R		S	*/
CASE_IGNORE_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	T		U		V		W	*/
CASE_XTERM_TITLE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	X		Y		Z		[	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_DA1,
CASE_CSI_STATE,
/*	\		]		^		_	*/
CASE_GROUND_STATE,
CASE_OSC,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	`		a		b		c	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_RIS,
/*	d		e		f		g	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	h		i		j		k	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	l		m		n		o	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_LS2,
CASE_LS3,
/*	p		q		r		s	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	t		u		v		w	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	x		y		z		{	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	|		}		~		DEL	*/
CASE_LS3R,
CASE_LS2R,
CASE_LS1R,
CASE_GROUND_STATE,
/*      0x80            0x81            0x82            0x83    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x84            0x85            0x86            0x87    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x88            0x89            0x8a            0x8b    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x8c            0x8d            0x8e            0x8f    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x90            0x91            0x92            0x93    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x94            0x95            0x96            0x97    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x99            0x99            0x9a            0x9b    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x9c            0x9d            0x9e            0x9f    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      nobreakspace    exclamdown      cent            sterling        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      currency        yen             brokenbar       section         */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      diaeresis       copyright       ordfeminine     guillemotleft   */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      notsign         hyphen          registered      macron          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      degree          plusminus       twosuperior     threesuperior   */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      acute           mu              paragraph       periodcentered  */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      cedilla         onesuperior     masculine       guillemotright  */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      onequarter      onehalf         threequarters   questiondown    */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Agrave          Aacute          Acircumflex     Atilde          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Adiaeresis      Aring           AE              Ccedilla        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Egrave          Eacute          Ecircumflex     Ediaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Igrave          Iacute          Icircumflex     Idiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Eth             Ntilde          Ograve          Oacute          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Ocircumflex     Otilde          Odiaeresis      multiply        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Ooblique        Ugrave          Uacute          Ucircumflex     */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Udiaeresis      Yacute          Thorn           ssharp          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      agrave          aacute          acircumflex     atilde          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      adiaeresis      aring           ae              ccedilla        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      egrave          eacute          ecircumflex     ediaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      igrave          iacute          icircumflex     idiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      eth             ntilde          ograve          oacute          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      ocircumflex     otilde          odiaeresis      division        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      oslash          ugrave          uacute          ucircumflex     */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      udiaeresis      yacute          thorn           ydiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
};

// #pragma mark CASE_IGNORE_ESC table
int gIesTable[] =
{
/*	NUL		SOH		STX		ETX	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	EOT		ENQ		ACK		BEL	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	BS		HT		NL		VT	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	NP		CR		SO		SI	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	DLE		DC1		DC2		DC3	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	DC4		NAK		SYN		ETB	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	CAN		EM		SUB		ESC	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	FS		GS		RS		US	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	SP		!		"		#	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	$		%		&		'	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	(		)		*		+	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	,		-		.		/	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	0		1		2		3	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	4		5		6		7	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	8		9		:		;	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	<		=		>		?	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	@		A		B		C	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	D		E		F		G	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	H		I		J		K	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	L		M		N		O	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	P		Q		R		S	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	T		U		V		W	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	X		Y		Z		[	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	\		]		^		_	*/
CASE_GROUND_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	`		a		b		c	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	d		e		f		g	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	h		i		j		k	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	l		m		n		o	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	p		q		r		s	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	t		u		v		w	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	x		y		z		{	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	|		}		~		DEL	*/
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*      0x80            0x81            0x82            0x83    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x84            0x85            0x86            0x87    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x88            0x89            0x8a            0x8b    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x8c            0x8d            0x8e            0x8f    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x90            0x91            0x92            0x93    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x94            0x95            0x96            0x97    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x99            0x99            0x9a            0x9b    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x9c            0x9d            0x9e            0x9f    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      nobreakspace    exclamdown      cent            sterling        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      currency        yen             brokenbar       section         */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      diaeresis       copyright       ordfeminine     guillemotleft   */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      notsign         hyphen          registered      macron          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      degree          plusminus       twosuperior     threesuperior   */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      acute           mu              paragraph       periodcentered  */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      cedilla         onesuperior     masculine       guillemotright  */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      onequarter      onehalf         threequarters   questiondown    */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Agrave          Aacute          Acircumflex     Atilde          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Adiaeresis      Aring           AE              Ccedilla        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Egrave          Eacute          Ecircumflex     Ediaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Igrave          Iacute          Icircumflex     Idiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Eth             Ntilde          Ograve          Oacute          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Ocircumflex     Otilde          Odiaeresis      multiply        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Ooblique        Ugrave          Uacute          Ucircumflex     */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Udiaeresis      Yacute          Thorn           ssharp          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      agrave          aacute          acircumflex     atilde          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      adiaeresis      aring           ae              ccedilla        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      egrave          eacute          ecircumflex     ediaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      igrave          iacute          icircumflex     idiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      eth             ntilde          ograve          oacute          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      ocircumflex     otilde          odiaeresis      division        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      oslash          ugrave          uacute          ucircumflex     */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      udiaeresis      yacute          thorn           ydiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
};

// #pragma mark CASE_IGNORE_STATE table
int gIgnoreTable[] =
{
/*	NUL		SOH		STX		ETX	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	EOT		ENQ		ACK		BEL	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	BS		HT		NL		VT	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	NP		CR		SO		SI	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	DLE		DC1		DC2		DC3	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	DC4		NAK		SYN		ETB	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	CAN		EM		SUB		ESC	*/
CASE_GROUND_STATE, 
CASE_IGNORE,
CASE_GROUND_STATE,
CASE_IGNORE_ESC,
/*	FS		GS		RS		US	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	SP		!		"		#	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	$		%		&		'	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	(		)		*		+	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	,		-		.		/	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	0		1		2		3	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	4		5		6		7	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	8		9		:		;	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	<		=		>		?	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	@		A		B		C	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	D		E		F		G	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	H		I		J		K	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	L		M		N		O	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	P		Q		R		S	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	T		U		V		W	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	X		Y		Z		[	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	\		]		^		_	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	`		a		b		c	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	d		e		f		g	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	h		i		j		k	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	l		m		n		o	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	p		q		r		s	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	t		u		v		w	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	x		y		z		{	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	|		}		~		DEL	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x80            0x81            0x82            0x83    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x84            0x85            0x86            0x87    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x88            0x89            0x8a            0x8b    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x8c            0x8d            0x8e            0x8f    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x90            0x91            0x92            0x93    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x94            0x95            0x96            0x97    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x99            0x99            0x9a            0x9b    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x9c            0x9d            0x9e            0x9f    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      nobreakspace    exclamdown      cent            sterling        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      currency        yen             brokenbar       section         */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      diaeresis       copyright       ordfeminine     guillemotleft   */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      notsign         hyphen          registered      macron          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      degree          plusminus       twosuperior     threesuperior   */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      acute           mu              paragraph       periodcentered  */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      cedilla         onesuperior     masculine       guillemotright  */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      onequarter      onehalf         threequarters   questiondown    */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Agrave          Aacute          Acircumflex     Atilde          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Adiaeresis      Aring           AE              Ccedilla        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Egrave          Eacute          Ecircumflex     Ediaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Igrave          Iacute          Icircumflex     Idiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Eth             Ntilde          Ograve          Oacute          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Ocircumflex     Otilde          Odiaeresis      multiply        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Ooblique        Ugrave          Uacute          Ucircumflex     */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Udiaeresis      Yacute          Thorn           ssharp          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      agrave          aacute          acircumflex     atilde          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      adiaeresis      aring           ae              ccedilla        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      egrave          eacute          ecircumflex     ediaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      igrave          iacute          icircumflex     idiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      eth             ntilde          ograve          oacute          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      ocircumflex     otilde          odiaeresis      division        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      oslash          ugrave          uacute          ucircumflex     */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      udiaeresis      yacute          thorn           ydiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
};

// #pragma mark ESC # - SCR table
int gScrTable[] =
{
/*	NUL		SOH		STX		ETX	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	EOT		ENQ		ACK		BEL	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_BELL,
/*	BS		HT		NL		VT	*/
CASE_BS,
CASE_TAB,
CASE_VMOT,
CASE_VMOT,
/*	NP		CR		SO		SI	*/
CASE_VMOT,
CASE_CR,
CASE_SO,
CASE_SI,
/*	DLE		DC1		DC2		DC3	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	DC4		NAK		SYN		ETB	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	CAN		EM		SUB		ESC	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_ESC,
/*	FS		GS		RS		US	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	SP		!		"		#	*/
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
/*	$		%		&		'	*/
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
/*	(		)		*		+	*/
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
/*	,		-		.		/	*/
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
/*	0		1		2		3	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	4		5		6		7	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	8		9		:		;	*/
CASE_DECALN,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	<		=		>		?	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	@		A		B		C	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	D		E		F		G	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	H		I		J		K	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	L		M		N		O	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	P		Q		R		S	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	T		U		V		W	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	X		Y		Z		[	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	\		]		^		_	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	`		a		b		c	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	d		e		f		g	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	h		i		j		k	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	l		m		n		o	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	p		q		r		s	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	t		u		v		w	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	x		y		z		{	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	|		}		~		DEL	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      0x80            0x81            0x82            0x83    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x84            0x85            0x86            0x87    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x88            0x89            0x8a            0x8b    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x8c            0x8d            0x8e            0x8f    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x90            0x91            0x92            0x93    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x94            0x95            0x96            0x97    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x99            0x99            0x9a            0x9b    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x9c            0x9d            0x9e            0x9f    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      nobreakspace    exclamdown      cent            sterling        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      currency        yen             brokenbar       section         */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      diaeresis       copyright       ordfeminine     guillemotleft   */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      notsign         hyphen          registered      macron          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      degree          plusminus       twosuperior     threesuperior   */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      acute           mu              paragraph       periodcentered  */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      cedilla         onesuperior     masculine       guillemotright  */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      onequarter      onehalf         threequarters   questiondown    */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Agrave          Aacute          Acircumflex     Atilde          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Adiaeresis      Aring           AE              Ccedilla        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Egrave          Eacute          Ecircumflex     Ediaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Igrave          Iacute          Icircumflex     Idiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Eth             Ntilde          Ograve          Oacute          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Ocircumflex     Otilde          Odiaeresis      multiply        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Ooblique        Ugrave          Uacute          Ucircumflex     */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Udiaeresis      Yacute          Thorn           ssharp          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      agrave          aacute          acircumflex     atilde          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      adiaeresis      aring           ae              ccedilla        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      egrave          eacute          ecircumflex     ediaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      igrave          iacute          icircumflex     idiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      eth             ntilde          ograve          oacute          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      ocircumflex     otilde          odiaeresis      division        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      oslash          ugrave          uacute          ucircumflex     */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      udiaeresis      yacute          thorn           ydiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
};

// #pragma mark ESC ( - SCS table
int gScsTable[] =
{
/*	NUL		SOH		STX		ETX	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	EOT		ENQ		ACK		BEL	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_BELL,
/*	BS		HT		NL		VT	*/
CASE_BS,
CASE_TAB,
CASE_VMOT,
CASE_VMOT,
/*	NP		CR		SO		SI	*/
CASE_VMOT,
CASE_CR,
CASE_SO,
CASE_SI,
/*	DLE		DC1		DC2		DC3	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	DC4		NAK		SYN		ETB	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	CAN		EM		SUB		ESC	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_ESC,
/*	FS		GS		RS		US	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	SP		!		"		#	*/
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
/*	$		%		&		'	*/
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
/*	(		)		*		+	*/
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
/*	,		-		.		/	*/
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
#ifdef USE_ISO2022
/*	0		1		2		3	*/
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
/*	4		5		6		7	*/
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
/*	8		9		:		;	*/
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
/*	<		=		>		?	*/
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
/*	@		A		B		C	*/
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
/*	D		E		F		G	*/
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
/*	H		I		J		K	*/
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
/*	L		M		N		O	*/
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
/*	P		Q		R		S	*/
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
/*	T		U		V		W	*/
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
/*	X		Y		Z		[	*/
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
/*	\		]		^		_	*/
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
/*	`		a		b		c	*/
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
/*	d		e		f		g	*/
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
/*	h		i		j		k	*/
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
/*	l		m		n		o	*/
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
CASE_GROUND_STATE, /* GSET('p') >= 0x40 (MBCS flag) */
/*	p		q		r		s	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	t		u		v		w	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	x		y		z		{	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	|		}		~		DEL	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE, /* empty character set */
CASE_GROUND_STATE,
#else /* !USE_ISO2022 */
/*	0		1		2		3	*/
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
CASE_GROUND_STATE,
/*	4		5		6		7	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	8		9		:		;	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	<		=		>		?	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	@		A		B		C	*/
CASE_GROUND_STATE,
CASE_GSETS,
CASE_GSETS,
CASE_GROUND_STATE,
/*	D		E		F		G	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	H		I		J		K	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	L		M		N		O	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	P		Q		R		S	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	T		U		V		W	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	X		Y		Z		[	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	\		]		^		_	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	`		a		b		c	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	d		e		f		g	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	h		i		j		k	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	l		m		n		o	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	p		q		r		s	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	t		u		v		w	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	x		y		z		{	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	|		}		~		DEL	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
#endif /* !USE_ISO2022 */
/*      0x80            0x81            0x82            0x83    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x84            0x85            0x86            0x87    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x88            0x89            0x8a            0x8b    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x8c            0x8d            0x8e            0x8f    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x90            0x91            0x92            0x93    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x94            0x95            0x96            0x97    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x99            0x99            0x9a            0x9b    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x9c            0x9d            0x9e            0x9f    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      nobreakspace    exclamdown      cent            sterling        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      currency        yen             brokenbar       section         */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      diaeresis       copyright       ordfeminine     guillemotleft   */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      notsign         hyphen          registered      macron          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      degree          plusminus       twosuperior     threesuperior   */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      acute           mu              paragraph       periodcentered  */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      cedilla         onesuperior     masculine       guillemotright  */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      onequarter      onehalf         threequarters   questiondown    */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Agrave          Aacute          Acircumflex     Atilde          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Adiaeresis      Aring           AE              Ccedilla        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Egrave          Eacute          Ecircumflex     Ediaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Igrave          Iacute          Icircumflex     Idiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Eth             Ntilde          Ograve          Oacute          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Ocircumflex     Otilde          Odiaeresis      multiply        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Ooblique        Ugrave          Uacute          Ucircumflex     */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Udiaeresis      Yacute          Thorn           ssharp          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      agrave          aacute          acircumflex     atilde          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      adiaeresis      aring           ae              ccedilla        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      egrave          eacute          ecircumflex     ediaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      igrave          iacute          icircumflex     idiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      eth             ntilde          ograve          oacute          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      ocircumflex     otilde          odiaeresis      division        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      oslash          ugrave          uacute          ucircumflex     */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      udiaeresis      yacute          thorn           ydiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
};

#ifdef USE_MBCS
// #pragma mark MBCS table
int gMbcsTable[] = {
/*	NUL		SOH		STX		ETX	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	EOT		ENQ		ACK		BEL	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_BELL,
/*	BS		HT		NL		VT	*/
CASE_BS,
CASE_TAB,
CASE_VMOT,
CASE_VMOT,
/*	NP		CR		SO		SI	*/
CASE_VMOT,
CASE_CR,
CASE_SO,
CASE_SI,
/*	DLE		DC1		DC2		DC3	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	DC4		NAK		SYN		ETB	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	CAN		EM		SUB		ESC	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_ESC,
/*	FS		GS		RS		US	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	SP		!		"		#	*/
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
/*	$		%		&		'	*/
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
/*	(		)		*		+	*/
CASE_IGNORE, /*CASE_SCS_STATE,*/
CASE_SCS_STATE,
CASE_SCS_STATE,
CASE_SCS_STATE,
/*	,		-		.		/	*/
CASE_ESC_IGNORE,
CASE_SCS_STATE,
CASE_SCS_STATE,
CASE_SCS_STATE,
/*	0		1		2		3	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	4		5		6		7	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	8		9		:		;	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	<		=		>		?	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	@		A		B		C	*/
CASE_GSETS,	/* ESC-$-@ (JIS-78) */
CASE_GSETS,	/* ESC-$-A (GB) */
CASE_GSETS,	/* ESC-$-B (JIS-83) */
CASE_GROUND_STATE,
/*	D		E		F		G	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	H		I		J		K	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	L		M		N		O	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	P		Q		R		S	*/
CASE_IGNORE_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	T		U		V		W	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	X		Y		Z		[	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	\		]		^		_	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_IGNORE_STATE,
CASE_IGNORE_STATE,
/*	`		a		b		c	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	d		e		f		g	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	h		i		j		k	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	l		m		n		o	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	p		q		r		s	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	t		u		v		w	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	x		y		z		{	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	|		}		~		DEL	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      0x80            0x81            0x82            0x83    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x84            0x85            0x86            0x87    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x88            0x89            0x8a            0x8b    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x8c            0x8d            0x8e            0x8f    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x90            0x91            0x92            0x93    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x94            0x95            0x96            0x97    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x99            0x99            0x9a            0x9b    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x9c            0x9d            0x9e            0x9f    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      nobreakspace    exclamdown      cent            sterling        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      currency        yen             brokenbar       section         */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      diaeresis       copyright       ordfeminine     guillemotleft   */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      notsign         hyphen          registered      macron          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      degree          plusminus       twosuperior     threesuperior   */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      acute           mu              paragraph       periodcentered  */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      cedilla         onesuperior     masculine       guillemotright  */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      onequarter      onehalf         threequarters   questiondown    */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Agrave          Aacute          Acircumflex     Atilde          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Adiaeresis      Aring           AE              Ccedilla        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Egrave          Eacute          Ecircumflex     Ediaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Igrave          Iacute          Icircumflex     Idiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Eth             Ntilde          Ograve          Oacute          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Ocircumflex     Otilde          Odiaeresis      multiply        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Ooblique        Ugrave          Uacute          Ucircumflex     */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Udiaeresis      Yacute          Thorn           ssharp          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      agrave          aacute          acircumflex     atilde          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      adiaeresis      aring           ae              ccedilla        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      egrave          eacute          ecircumflex     ediaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      igrave          iacute          icircumflex     idiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      eth             ntilde          ograve          oacute          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      ocircumflex     otilde          odiaeresis      division        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      oslash          ugrave          uacute          ucircumflex     */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      udiaeresis      yacute          thorn           ydiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
};

// #pragma mark SMBCS table
int gSmbcsTable[] = {
/*	NUL		SOH		STX		ETX	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	EOT		ENQ		ACK		BEL	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_BELL,
/*	BS		HT		NL		VT	*/
CASE_BS,
CASE_TAB,
CASE_VMOT,
CASE_VMOT,
/*	NP		CR		SO		SI	*/
CASE_VMOT,
CASE_CR,
CASE_SO,
CASE_SI,
/*	DLE		DC1		DC2		DC3	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	DC4		NAK		SYN		ETB	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	CAN		EM		SUB		ESC	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_ESC,
/*	FS		GS		RS		US	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	SP		!		"		#	*/
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
/*	$		%		&		'	*/
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
/*	(		)		*		+	*/
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
/*	,		-		.		/	*/
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
/*	0		1		2		3	*/
CASE_GROUND_STATE, /* (2-byte or more) private character set */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	4		5		6		7	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	8		9		:		;	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	<		=		>		?	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	@		A		B		C	*/
CASE_GSETS,	/* ESC-$-I-F */
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
/*	D		E		F		G	*/
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
/*	H		I		J		K	*/
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
/*	L		M		N		O	*/
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
/*	P		Q		R		S	*/
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
/*	T		U		V		W	*/
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
/*	X		Y		Z		[	*/
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
/*	\		]		^		_	*/
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
CASE_GSETS,
/*	`		a		b		c	*/
CASE_GROUND_STATE, /* 3-byte character set */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	d		e		f		g	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	h		i		j		k	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	l		m		n		o	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	p		q		r		s	*/
CASE_GROUND_STATE, /* 4-byte character set */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	t		u		v		w	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	x		y		z		{	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	|		}		~		DEL	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      0x80            0x81            0x82            0x83    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x84            0x85            0x86            0x87    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x88            0x89            0x8a            0x8b    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x8c            0x8d            0x8e            0x8f    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x90            0x91            0x92            0x93    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x94            0x95            0x96            0x97    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x99            0x99            0x9a            0x9b    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      0x9c            0x9d            0x9e            0x9f    */
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*      nobreakspace    exclamdown      cent            sterling        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      currency        yen             brokenbar       section         */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      diaeresis       copyright       ordfeminine     guillemotleft   */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      notsign         hyphen          registered      macron          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      degree          plusminus       twosuperior     threesuperior   */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      acute           mu              paragraph       periodcentered  */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      cedilla         onesuperior     masculine       guillemotright  */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      onequarter      onehalf         threequarters   questiondown    */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Agrave          Aacute          Acircumflex     Atilde          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Adiaeresis      Aring           AE              Ccedilla        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Egrave          Eacute          Ecircumflex     Ediaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Igrave          Iacute          Icircumflex     Idiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Eth             Ntilde          Ograve          Oacute          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Ocircumflex     Otilde          Odiaeresis      multiply        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Ooblique        Ugrave          Uacute          Ucircumflex     */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      Udiaeresis      Yacute          Thorn           ssharp          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      agrave          aacute          acircumflex     atilde          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      adiaeresis      aring           ae              ccedilla        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      egrave          eacute          ecircumflex     ediaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      igrave          iacute          icircumflex     idiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      eth             ntilde          ograve          oacute          */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      ocircumflex     otilde          odiaeresis      division        */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      oslash          ugrave          uacute          ucircumflex     */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*      udiaeresis      yacute          thorn           ydiaeresis      */
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
};

#endif

// #pragma mark Shift-JIS ground table
int gSJISGroundTable[] =
{
/*	NUL		SOH		STX		ETX	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	EOT		ENQ		ACK		BEL	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_BELL,
/*	BS		HT		NL		VT	*/
CASE_BS,
CASE_TAB,
CASE_LF,
CASE_LF, /*CASE_UP,*/
/*	NP		CR		SO		SI	*/
CASE_LF, /*CASE_IGNORE,*/
CASE_CR,
CASE_SO,
CASE_SI,
/*	DLE		DC1		DC2		DC3	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	DC4		NAK		SYN		ETB	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	CAN		EM		SUB		ESC	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_ESC,
/*	FS		GS		RS		US	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	SP		!		"		#	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	$		%		&		'	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	(		)		*		+	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	,		-		.		/	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	0		1		2		3	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	4		5		6		7	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	8		9		:		;	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	<		=		>		?	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT, 
/*	@		A		B		C	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	D		E		F		G	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	H		I		J		K	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	L		M		N		O	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	P		Q		R		S	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	T		U		V		W	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	X		Y		Z		[	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	\		]		^		_	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	`		a		b		c	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	d		e		f		g	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	h		i		j		k	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	l		m		n		o	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	p		q		r		s	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	t		u		v		w	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	x		y		z		{	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	|		}		~		DEL	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*      0x80            0x81            0x82            0x83    */
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
/*      0x84            0x85            0x86            0x87    */
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
/*      0x88            0x89            0x8a            0x8b    */
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
/*      0x8c            0x8d            0x8e            0x8f    */
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
/*      0x90            0x91            0x92            0x93    */
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
/*      0x94            0x95            0x96            0x97    */
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
/*      0x99            0x99            0x9a            0x9b    */
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
/*      0x9c            0x9d            0x9e            0x9f    */
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
/*      0xa0		0xa1		0xa2		0xa3	*/
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
/*      0xa4		0xa5		0xa6		0xa7	*/
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
/*      0xa8		0xa9		0xaa		0xab	*/
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
/*      0xac		0xad		0xae		0xaf	*/
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
/*      0xb0		0xb1		0xb2		0xb3	*/
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
/*      0xb4		0xb5		0xb6		0xb7	*/
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
/*      0xb8		0xb9		0xba		0xbb	*/
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
/*      0xbc		0xbd		0xbe		0xbf	*/
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
/*      0xc0		0xc1		0xc2		0xc3	*/
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
/*      0xc4		0xc5		0xc6		0xc7	*/
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
/*      0xc8		0xc9		0xca		0xcb	*/
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
/*      0xcc		0xcd		0xce		0xcf	*/
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
/*      0xd0		0xd1		0xd2		0xd3	*/
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
/*      0xd4		0xd5		0xd6		0xd7	*/
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
/*      0xd8		0xd9		0xda		0xdb	*/
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
/*      0xdc		0xdd		0xde		0xdf	*/
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
CASE_SJIS_KANA,
/*      0xe0		0xe1		0xe2		0xe3	*/
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
/*      0xe4		0xe5		0xe6		0xe7	*/
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
/*      0xe8		0xe9		0xea		0xeb	*/
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
/*      0xec		0xed		0xee		0xef	*/
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
/*      0xf0		0xf1		0xf2		0xf3	*/
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
/*      0xf4		0xf5		0xf6		0xf7	*/
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
/*      0xf8		0xf9		0xfa		0xfb	*/
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
/*      0xfc		0xfd		0xfe		0xff	*/
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
CASE_SJIS_INSTRING,
};


// #pragma mark Line drawing table
int gLineDrawTable[] =
{
/*	NUL		SOH		STX		ETX	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	EOT		ENQ		ACK		BEL	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_BELL,
/*	BS		HT		NL		VT	*/
CASE_BS,
CASE_TAB,
CASE_LF,
CASE_LF, /* CASE_UP*/
/*	NP		CR		SO		SI	*/
CASE_LF, /* CASE_IGNORE*/
CASE_CR,
CASE_SO,
CASE_SI,
/*	DLE		DC1		DC2		DC3	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	DC4		NAK		SYN		ETB	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	CAN		EM		SUB		ESC	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_ESC,
/*	FS		GS		RS		US	*/
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
CASE_IGNORE,
/*	SP		!		"		#	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	$		%		&		'	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	(		)		*		+	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	,		-		.		/	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	0		1		2		3	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	4		5		6		7	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	8		9		:		;	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	<		=		>		?	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT, 
/*	@		A		B		C	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	D		E		F		G	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	H		I		J		K	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	L		M		N		O	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	P		Q		R		S	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	T		U		V		W	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	X		Y		Z		[	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	\		]		^		_	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	`		a		b		c	*/
CASE_PRINT,
CASE_PRINT_GRA,
CASE_PRINT,
CASE_PRINT,
/*	d		e		f		g	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	h		i		j		k	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT_GRA,
CASE_PRINT_GRA,
/*	l		m		n		o	*/
CASE_PRINT_GRA,
CASE_PRINT_GRA,
CASE_PRINT_GRA,
CASE_PRINT,
/*	p		q		r		s	*/
CASE_PRINT,
CASE_PRINT_GRA,
CASE_PRINT,
CASE_PRINT,
/*	t		u		v		w	*/
CASE_PRINT_GRA,
CASE_PRINT_GRA,
CASE_PRINT_GRA,
CASE_PRINT_GRA,
/*	x		y		z		{	*/
CASE_PRINT_GRA,
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
/*	|		}		~		DEL	*/
CASE_PRINT,
CASE_PRINT,
CASE_PRINT,
CASE_IGNORE,
/*      0x80            0x81            0x82            0x83    */
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0x84            0x85            0x86            0x87    */
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0x88            0x89            0x8a            0x8b    */
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0x8c            0x8d            0x8e            0x8f    */
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0x90            0x91            0x92            0x93    */
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0x94            0x95            0x96            0x97    */
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0x99            0x99            0x9a            0x9b    */
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0x9c            0x9d            0x9e            0x9f    */
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0xa0		0xa1		0xa2		0xa3	*/
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0xa4		0xa5		0xa6		0xa7	*/
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0xa8		0xa9		0xaa		0xab	*/
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0xac		0xad		0xae		0xaf	*/
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0xb0		0xb1		0xb2		0xb3	*/
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0xb4		0xb5		0xb6		0xb7	*/
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0xb8		0xb9		0xba		0xbb	*/
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0xbc		0xbd		0xbe		0xbf	*/
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
CASE_UTF8_INSTRING,
/*      0xc0		0xc1		0xc2		0xc3	*/
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
/*      0xc4		0xc5		0xc6		0xc7	*/
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
/*      0xc8		0xc9		0xca		0xcb	*/
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
/*      0xcc		0xcd		0xce		0xcf	*/
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
/*      0xd0		0xd1		0xd2		0xd3	*/
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
/*      0xd4		0xd5		0xd6		0xd7	*/
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
/*      0xd8		0xd9		0xda		0xdb	*/
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
/*      0xdc		0xdd		0xde		0xdf	*/
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
CASE_UTF8_2BYTE,
/*      0xe0		0xe1		0xe2		0xe3	*/
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
/*      0xe4		0xe5		0xe6		0xe7	*/
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
/*      0xe8		0xe9		0xea		0xeb	*/
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
/*      0xec		0xed		0xee		0xef	*/
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
/*      0xf0		0xf1		0xf2		0xf3	*/
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
/*      0xf4		0xf5		0xf6		0xf7	*/
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
/*      0xf8		0xf9		0xfa		0xfb	*/
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
/*      0xfc		0xfd		0xfe		0xff	*/
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
CASE_UTF8_3BYTE,
};
