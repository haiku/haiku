/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2001, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

BeMail(TM), Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/
#ifndef _MAIL_SUPPORT_H
#define _MAIL_SUPPORT_H


#include <SupportDefs.h>


#define MAX_DICTIONARIES	8
#define	TITLE_BAR_HEIGHT	25
#define	WIND_WIDTH			457
#define WIND_HEIGHT			400
#define RIGHT_BOUNDARY 		8191
#define SEPARATOR_MARGIN	7
#define QUOTE				"> "


enum USER_LEVEL {
	L_BEGINNER = 0,
	L_EXPERT
};

enum WINDOW_TYPES {
	MAIL_WINDOW = 0,
	PREFS_WINDOW,
	SIG_WINDOW
};


class BFile;
class BMenu;
class Words;


int32 header_len(BFile*);
int32 add_query_menu_items(BMenu* menu, const char* attribute, uint32 what,
	const char* format, bool popup = false);


extern Words* gWords[MAX_DICTIONARIES];
extern Words* gExactWords[MAX_DICTIONARIES];
extern int32 gUserDict;
extern BFile* gUserDictFile;
extern int32 gDictCount;

extern const char* kSpamServerSignature;
extern const char* kDraftPath;
extern const char* kDraftType;
extern const char* kMailFolder;
extern const char* kMailboxSymlink;


#endif // _MAIL_SUPPORT_H

