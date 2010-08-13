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


#define CASE_GROUND_STATE 0
#define CASE_IGNORE_STATE 1
#define CASE_IGNORE_ESC 2
#define CASE_IGNORE 3
#define CASE_BELL 4
#define CASE_BS 5
#define CASE_CR 6
#define CASE_ESC 7
#define CASE_VMOT 8
#define CASE_TAB 9
#define CASE_LF 10
#define CASE_SI 11
#define CASE_SO 12
#define CASE_SP 13
#define CASE_SCR_STATE 14
#define CASE_SCS0_STATE 15
#define CASE_SCS1_STATE 16
#define CASE_SCS2_STATE 17
#define CASE_SCS3_STATE 18
#define CASE_ESC_IGNORE 19
#define CASE_ESC_DIGIT 20
#define CASE_ESC_SEMI 21
#define CASE_DEC_STATE 22
#define CASE_ICH 23
#define CASE_CUU 24
#define CASE_CUD 25
#define CASE_CUF 26
#define CASE_CUB 27
#define CASE_CUP 28
#define CASE_ED 29
#define CASE_EL 30
#define CASE_IL 31
#define CASE_DL 32
#define CASE_DCH 33
#define CASE_DA1 34
#define CASE_TRACK_MOUSE 35
#define CASE_TBC 36
#define CASE_SET 37
#define CASE_RST 38
#define CASE_SGR 39
#define CASE_CPR 40
#define CASE_DECSTBM 41
#define CASE_DECREQTPARM 42
#define CASE_DECSET 43
#define CASE_DECRST 44
#define CASE_DECALN 45
#define CASE_GSETS 46
#define CASE_DECSC 47
#define CASE_DECRC 48
#define CASE_DECKPAM 49
#define CASE_DECKPNM 50
#define CASE_IND 51
#define CASE_NEL 52
#define CASE_HTS 53
#define CASE_RI 54
#define CASE_SS2 55
#define CASE_SS3 56
#define CASE_CSI_STATE 57
#define CASE_OSC 58
#define CASE_RIS 59
#define CASE_LS2 60
#define CASE_LS3 61
#define CASE_LS3R 62
#define CASE_LS2R 63
#define CASE_LS1R 64
#define CASE_PRINT 65
#define CASE_XTERM_SAVE 66
#define CASE_XTERM_RESTORE 67
#define CASE_XTERM_TITLE 68
#define CASE_DECID 69
#define CASE_HP_MEM_LOCK 70
#define CASE_HP_MEM_UNLOCK 71
#define CASE_HP_BUGGY_LL 72
#define CASE_TO_STATUS 73
#define CASE_FROM_STATUS 74
#define CASE_SHOW_STATUS 75
#define CASE_HIDE_STATUS 76
#define CASE_ERASE_STATUS 77
#define CASE_MBCS 78
#define CASE_SCS_STATE 79
#define CASE_UTF8_2BYTE 80
#define CASE_UTF8_3BYTE 81
#define CASE_UTF8_INSTRING 82
#define CASE_SJIS_INSTRING 83
#define CASE_SJIS_KANA 84
#define CASE_PRINT_GR 85
#define CASE_PRINT_CS96 86
// additions, maybe reorder/reuse older ones ?
#define CASE_VPA 87
#define CASE_HPA 88

#define CASE_SU		89	/* scroll screen up */
#define CASE_SD		90	/* scroll screen down */
#define CASE_ECH 	91	/* erase characters */

#define CASE_PRINT_GRA 92
