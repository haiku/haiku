/* Copyright 1992 NEC Corporation, Tokyo, Japan.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of NEC
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  NEC Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * NEC CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN 
 * NO EVENT SHALL NEC CORPORATION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF 
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
 * OTHER TORTUOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
 * PERFORMANCE OF THIS SOFTWARE. 
 */

/* @(#) 102.1 $Id: keydef.h 10527 2004-12-23 22:08:39Z korli $ */
/* normal function keys */
     
#define CANNA_KEY_Nfer		0x80
#define CANNA_KEY_Xfer		0x81
#define CANNA_KEY_Up		0x82
#define CANNA_KEY_Left		0x83
#define CANNA_KEY_Right		0x84
#define CANNA_KEY_Down		0x85
#define CANNA_KEY_Insert	0x86
#define CANNA_KEY_Rollup	0x87
#define CANNA_KEY_PageDown      CANNA_KEY_Rollup
#define CANNA_KEY_Rolldown	0x88
#define CANNA_KEY_PageUp        CANNA_KEY_Rolldown
#define CANNA_KEY_Home		0x89
#define CANNA_KEY_Help		0x8a
#define CANNA_KEY_KP_Key	0x8b /* どういう意味で入れたんだっけ？ */
#define CANNA_KEY_End		0x8c

/* shifted function keys */

#define CANNA_KEY_Shift_Nfer	0x90
#define CANNA_KEY_Shift_Xfer	0x91
#define CANNA_KEY_Shift_Up	0x92
#define CANNA_KEY_Shift_Left	0x93
#define CANNA_KEY_Shift_Right	0x94
#define CANNA_KEY_Shift_Down	0x95

/* control-shifted function keys */

#define CANNA_KEY_Cntrl_Nfer	0x96
#define CANNA_KEY_Cntrl_Xfer	0x97
#define CANNA_KEY_Cntrl_Up	0x98
#define CANNA_KEY_Cntrl_Left	0x99
#define CANNA_KEY_Cntrl_Right	0x9a
#define CANNA_KEY_Cntrl_Down	0x9b

/* The followings should have special translation rule */
#define CANNA_KEY_KP_Separator  0x9c
#define CANNA_KEY_KP_Decimal    0x9d
#define CANNA_KEY_KP_Divide     0x9e
#define CANNA_KEY_KP_Subtract	0x9f
#define CANNA_KEY_Shift_Space	0xa0

/* application keypad mode keys */

#ifdef DoNotConflictWithKatakanaKeys

/* 以下のキーはカタカナ文字とぶつかってしまうため使えない。*/

#define CANNA_KEY_KP_Up		0xc0	/* \eOA			*/
#define CANNA_KEY_KP_Left	0xc1	/* \eOB			*/
#define CANNA_KEY_KP_Right	0xc2	/* \eOC			*/
#define CANNA_KEY_KP_Down	0xc3	/* \eOD			*/
#define CANNA_KEY_KP_Tab	0xc4	/* \eOI			*/
#define CANNA_KEY_KP_Enter	0xc5	/* \eOM			*/
#define CANNA_KEY_KP_Equal	0xc6	/* \eOX			*/
#define CANNA_KEY_KP_Multiply	0xc7	/* \eOj			*/
#define CANNA_KEY_KP_Add	0xc8	/* \eOk			*/
#define CANNA_KEY_KP_Separator	0xc9	/* \eOl			*/
#define CANNA_KEY_KP_Subtract	0xca	/* \eOm			*/
#define CANNA_KEY_KP_Decimal	0xcb	/* \eOn			*/
#define CANNA_KEY_KP_Divide	0xcc	/* \eOo			*/
#define CANNA_KEY_KP_0		0xd0	/* \eOp			*/
#define CANNA_KEY_KP_1		0xd1	/* \eOq			*/
#define CANNA_KEY_KP_2		0xd2	/* \eOr			*/
#define CANNA_KEY_KP_3		0xd3	/* \eOs			*/
#define CANNA_KEY_KP_4		0xd4	/* \eOt			*/
#define CANNA_KEY_KP_5		0xd5	/* \eOu			*/
#define CANNA_KEY_KP_6		0xd6	/* \eOv			*/
#define CANNA_KEY_KP_7		0xd7	/* \eOw			*/
#define CANNA_KEY_KP_8		0xd8	/* \eOx			*/
#define CANNA_KEY_KP_9		0xd9	/* \eOy			*/

#endif

/* numeral-function keys */

#define CANNA_KEY_F1		0xe0
#define CANNA_KEY_F2		0xe1
#define CANNA_KEY_F3		0xe2
#define CANNA_KEY_F4		0xe3
#define CANNA_KEY_F5		0xe4
#define CANNA_KEY_F6		0xe5
#define CANNA_KEY_F7		0xe6
#define CANNA_KEY_F8		0xe7
#define CANNA_KEY_F9		0xe8
#define CANNA_KEY_F10		0xe9
#define CANNA_KEY_PF1		0xf0
#define CANNA_KEY_PF2		0xf1
#define CANNA_KEY_PF3		0xf2
#define CANNA_KEY_PF4		0xf3
#define CANNA_KEY_PF5		0xf4
#define CANNA_KEY_PF6		0xf5
#define CANNA_KEY_PF7		0xf6
#define CANNA_KEY_PF8		0xf7
#define CANNA_KEY_PF9		0xf8
#define CANNA_KEY_PF10		0xf9

#define CANNA_KEY_HIRAGANA	0xfa
#define CANNA_KEY_KATAKANA	0xfb
#define CANNA_KEY_HANKAKUZENKAKU 0xfc
#define CANNA_KEY_EISU		0xfd

#define CANNA_KEY_Undefine   	0xff

#ifdef IROHA_BC

#define IROHA_KEY_Nfer		CANNA_KEY_Nfer
#define IROHA_KEY_Xfer		CANNA_KEY_Xfer
#define IROHA_KEY_Up		CANNA_KEY_Up
#define IROHA_KEY_Left		CANNA_KEY_Left
#define IROHA_KEY_Right		CANNA_KEY_Right
#define IROHA_KEY_Down		CANNA_KEY_Down
#define IROHA_KEY_Insert	CANNA_KEY_Insert
#define IROHA_KEY_Rollup	CANNA_KEY_Rollup
#define IROHA_KEY_Rolldown	CANNA_KEY_Rolldown
#define IROHA_KEY_Home		CANNA_KEY_Home
#define IROHA_KEY_Help		CANNA_KEY_Help
#define IROHA_KEY_KP_Key	CANNA_KEY_KP_Key

/* shifted function keys */

#define IROHA_KEY_Shift_Nfer	CANNA_KEY_Shift_Nfer
#define IROHA_KEY_Shift_Xfer	CANNA_KEY_Shift_Xfer
#define IROHA_KEY_Shift_Up	CANNA_KEY_Shift_Up
#define IROHA_KEY_Shift_Left	CANNA_KEY_Shift_Left
#define IROHA_KEY_Shift_Right	CANNA_KEY_Shift_Right
#define IROHA_KEY_Shift_Down	CANNA_KEY_Shift_Down

/* control-shifted function keys */

#define IROHA_KEY_Cntrl_Nfer	CANNA_KEY_Cntrl_Nfer
#define IROHA_KEY_Cntrl_Xfer	CANNA_KEY_Cntrl_Xfer
#define IROHA_KEY_Cntrl_Up	CANNA_KEY_Cntrl_Up
#define IROHA_KEY_Cntrl_Left	CANNA_KEY_Cntrl_Left
#define IROHA_KEY_Cntrl_Right	CANNA_KEY_Cntrl_Right
#define IROHA_KEY_Cntrl_Down	CANNA_KEY_Cntrl_Down

/* application keypad mode keys */

#ifdef DoNotConflictWithKatakanaKeys

/* 以下のキーはカタカナ文字とぶつかってしまうため使えない。*/

#define IROHA_KEY_KP_Up		CANNA_KEY_KP_Up
#define IROHA_KEY_KP_Left	CANNA_KEY_KP_Left
#define IROHA_KEY_KP_Right	CANNA_KEY_KP_Right
#define IROHA_KEY_KP_Down	CANNA_KEY_KP_Down
#define IROHA_KEY_KP_Tab	CANNA_KEY_KP_Tab
#define IROHA_KEY_KP_Enter	CANNA_KEY_KP_Enter
#define IROHA_KEY_KP_Equal	CANNA_KEY_KP_Equal
#define IROHA_KEY_KP_Multiply	CANNA_KEY_KP_Multiply
#define IROHA_KEY_KP_Add	CANNA_KEY_KP_Add
#define IROHA_KEY_KP_Separator	CANNA_KEY_KP_Separator
#define IROHA_KEY_KP_Subtract	CANNA_KEY_KP_Subtract
#define IROHA_KEY_KP_Decimal	CANNA_KEY_KP_Decimal
#define IROHA_KEY_KP_Divide	CANNA_KEY_KP_Divide
#define IROHA_KEY_KP_0		CANNA_KEY_KP_0
#define IROHA_KEY_KP_1		CANNA_KEY_KP_1
#define IROHA_KEY_KP_2		CANNA_KEY_KP_2
#define IROHA_KEY_KP_3		CANNA_KEY_KP_3
#define IROHA_KEY_KP_4		CANNA_KEY_KP_4
#define IROHA_KEY_KP_5		CANNA_KEY_KP_5
#define IROHA_KEY_KP_6		CANNA_KEY_KP_6
#define IROHA_KEY_KP_7		CANNA_KEY_KP_7
#define IROHA_KEY_KP_8		CANNA_KEY_KP_8
#define IROHA_KEY_KP_9		CANNA_KEY_KP_9

#endif

/* numeral-function keys */

#define IROHA_KEY_F1		CANNA_KEY_F1
#define IROHA_KEY_F2		CANNA_KEY_F2
#define IROHA_KEY_F3		CANNA_KEY_F3
#define IROHA_KEY_F4		CANNA_KEY_F4
#define IROHA_KEY_F5		CANNA_KEY_F5
#define IROHA_KEY_F6		CANNA_KEY_F6
#define IROHA_KEY_F7		CANNA_KEY_F7
#define IROHA_KEY_F8		CANNA_KEY_F8
#define IROHA_KEY_F9		CANNA_KEY_F9
#define IROHA_KEY_F10		CANNA_KEY_F10
#define IROHA_KEY_PF1		CANNA_KEY_PF1
#define IROHA_KEY_PF2		CANNA_KEY_PF2
#define IROHA_KEY_PF3		CANNA_KEY_PF3
#define IROHA_KEY_PF4		CANNA_KEY_PF4
#define IROHA_KEY_PF5		CANNA_KEY_PF5
#define IROHA_KEY_PF6		CANNA_KEY_PF6
#define IROHA_KEY_PF7		CANNA_KEY_PF7
#define IROHA_KEY_PF8		CANNA_KEY_PF8
#define IROHA_KEY_PF9		CANNA_KEY_PF9
#define IROHA_KEY_PF10		CANNA_KEY_PF10
#define IROHA_KEY_Undefine   	CANNA_KEY_Undefine

#endif /* IROHA_BC */
