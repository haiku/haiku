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

#define CASE_IGNORE 0
#define CASE_FUNCTION 1
#define CASE_UP_ARROW 2
#define CASE_DOWN_ARROW 3
#define CASE_RIGHT_ARROW 4
#define CASE_LEFT_ARROW 5
#define CASE_RET_ENTR 6

#define F1_KEY 0x02
#define F2_KEY 0x03
#define F3_KEY 0x04
#define F4_KEY 0x05
#define F5_KEY 0x06
#define F6_KEY 0x07
#define F7_KEY 0x08
#define F8_KEY 0x09
#define F9_KEY 0x0a
#define F10_KEY 0x0b
#define F11_KEY 0x0c
#define F12_KEY 0x0d

#define RETURN_KEY 0x47
#define ENTER_KEY 0x5b

#define LEFT_ARROW_KEY 0x61
#define RIGHT_ARROW_KEY 0x63
#define UP_ARROW_KEY 0x57
#define DOWN_ARROW_KEY 0x62

#define HOME_KEY 0x20
#define INSERT_KEY 0x1f
#define END_KEY 0x35
#define PAGE_UP_KEY 0x21
#define PAGE_DOWN_KEY 0x36


#define LEFT_ARROW_KEY_CODE "\033OD"
#define RIGHT_ARROW_KEY_CODE "\033OC"
#define UP_ARROW_KEY_CODE "\033OA"
#define DOWN_ARROW_KEY_CODE "\033OB"

#define CTRL_LEFT_ARROW_KEY_CODE "\033O5D"
#define CTRL_RIGHT_ARROW_KEY_CODE "\033O5C"
#define CTRL_UP_ARROW_KEY_CODE "\033O5A"
#define CTRL_DOWN_ARROW_KEY_CODE "\033O5B"

#define DELETE_KEY_CODE		"\033[3~"
#define BACKSPACE_KEY_CODE	"\177"

#define HOME_KEY_CODE "\033OH"
#define INSERT_KEY_CODE "\033[2~"
#define END_KEY_CODE "\033OF"
#define PAGE_UP_KEY_CODE "\033[5~"
#define PAGE_DOWN_KEY_CODE "\033[6~"

//#define IS_DOWN_KEY(x) (info.key_states[(x) / 8] & key_state_table[(x) % 8])
#define IS_DOWN_KEY(x) \
(info.key_states[(x) >> 3] & (1 << (7 - ((x) % 8))))
