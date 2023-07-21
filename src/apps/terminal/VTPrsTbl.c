/*
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Kian Duffy, myob@users.sourceforge.net
 *		Siarzhuk Zharski, zharik@gmx.li
 */


#include <SupportDefs.h>

#include "VTparse.h"


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
CASE_LS1,
CASE_LS0,
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
CASE_LS1,
CASE_LS0,
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

// #pragma mark WinCP table (ISO8859 + C1)
// This one defines both C1 control and GR characters
// as CASE_PRINT_GR to let process set of encodings
// using this areas: cp1252, cp1251, koi-8r, cp866, gb18030
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
CASE_LS1,
CASE_LS0,
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
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
/*	NP		CR		SO		SI	*/
CASE_ESC_IGNORE,
CASE_CR,
CASE_LS1,
CASE_LS0,
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
CASE_CSI_SP,
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
CASE_ESC_SEMI,
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
CASE_CNL,
CASE_CPL,
CASE_HPA,
/*	H		I		J		K	*/
CASE_CUP,
CASE_CFT,
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
CASE_CBT,
CASE_GROUND_STATE,
/*	\		]		^		_	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	`		a		b		c	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_REP,
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
CASE_DECSCUSR_ETC,
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
CASE_LS1,
CASE_LS0,
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
CASE_LS1,
CASE_LS0,
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
CASE_LS1,
CASE_LS0,
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
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
CASE_ESC_IGNORE,
/*	(		)		*		+	*/
CASE_SCS_STATE,
CASE_SCS_STATE,
CASE_SCS_STATE,
CASE_SCS_STATE,
/*	,		-		.		/	*/
CASE_SCS_STATE,	/* not defined in ISO2022 but used in Mule */
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
CASE_INDEX,
CASE_NEXT_LINE,
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
CASE_LS1,
CASE_LS0,
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
CASE_LS1,
CASE_LS0,
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

/*
 * 94/96 alternative character sets for G0-G3
 *
 * - characters to replace are UTF-8 literals
 * - NULL mean falling through to corresponding ASCII chars
 *
 */

/* DEC Special Graphic Character Set - mix of xterm
   definitions and ncurses extended characters (ACS_)  */
const char* gLineDrawGraphSet[96] =
{
//	SP		!		"		#
NULL,
NULL,
NULL,
NULL,

//	$		%		&		'
NULL,
NULL,
NULL,
NULL,

//	(		)		*		+
NULL,
NULL,
NULL,
"\xE2\x86\x92", // ACS_RARROW

//	,		-		.		/
"\xE2\x86\x90", // ACS_LARROW
"\xE2\x86\x91", // ACS_UARROW
"\xE2\x86\x93", // ACS_DARROW
NULL,

//	0		1		2		3
"\xE2\x96\xAE", // ACS_BLOCK
NULL,
NULL,
NULL,

//	4		5		6		7
NULL,
NULL,
NULL,
NULL,

//	8		9		:		;
NULL,
NULL,
NULL,
NULL,

//	<		=		>		?
NULL,
NULL,
NULL,
NULL,

//	@		A		B		C
NULL,
NULL,
NULL,
NULL,

//	D		E		F		G
NULL,
NULL,
NULL,
NULL,

//	H		I		J		K
NULL,
NULL,
NULL,
NULL,

//	L		M		N		O
NULL,
NULL,
NULL,
NULL,

//	P		Q		R		S
NULL,
NULL,
NULL,
NULL,

//	T		U		V		W
NULL,
NULL,
NULL,
NULL,

//	X		Y		Z		[
NULL,
NULL,
NULL,
NULL,
//	\		]		^		_
NULL,
NULL,
NULL,
"\xE2\x96\xAE", // xterm: black vertical rectangle

//	`		a		b		c
"\xE2\x97\x86", // ACS_DIAMOND
"\xE2\x96\x92", // ACS_CKBOARD
"\xE2\x90\x89", // xterm:symbol for horizontal tabulation
"\xE2\x90\x8C", // xterm:symbol for form feed

//	d		e		f		g
"\xE2\x90\x8D", // xterm:symbol for carriage return
"\xE2\x90\x8A", // xterm:symbol for line feed
"\xC2\xB0", // ACS_DEGREE
"\xC2\xB1", // ACS_PLMINUS

//	h		i		j		k
"\xE2\x90\xA4", // xterm:symbol for newline (ACS_BOARD)
"\xE2\x98\x83", // ACS_LANTERN (xterm:symbol for vert.tab: \xE2\x90\x8B)
"\xE2\x94\x98", // ACS_LRCORNER
"\xE2\x94\x90", // ACS_URCORNER

//	l		m		n		o
"\xE2\x94\x8C", // ACS_ULCORNER
"\xE2\x94\x94", // ACS_LLCORNER
"\xE2\x94\xBC", // ACS_PLUS
"\xE2\x8E\xBA", // ACS_S1

//	p		q		r		s
"\xE2\x8E\xBB", // ACS_S3
"\xE2\x94\x80", // ACS_HLINE
"\xE2\x8E\xBC", // ACS_S7
"\xE2\x8E\xBD", // ACS_S9

//	t		u		v		w
"\xE2\x94\x9C", // ACS_LTEE
"\xE2\x94\xA4", // ACS_RTEE
"\xE2\x94\xB4", // ACS_BTEE
"\xE2\x94\xAC", // ACS_TTEE

//	x		y		z		{
"\xE2\x94\x82", // ACS_VLINE
"\xE2\x89\xA4", // ACS_LEQUAL
"\xE2\x89\xA5", // ACS_GEQUAL
"\xCF\x80", // ACS_PI

//	|		}		~		DEL
"\xE2\x89\xA0", // ACS_NEQUAL
"\xC2\xA3", // ACS_STERLING
"\xC2\xB7", // ACS_BULLET
NULL
};
