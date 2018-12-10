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
#ifndef _MESSAGES_H
#define _MESSAGES_H

enum MESSAGES {
	REFS_RECEIVED = 64,
	LIST_INVOKED,
	WINDOW_CLOSED,
	CHANGE_FONT,
	RESET_BUTTONS,
	PREFS_CHANGED,
	CHARSET_CHOICE_MADE
};

enum TEXT {
	SUBJECT_FIELD = REFS_RECEIVED + 64,
	TO_FIELD,
	ENCLOSE_FIELD,
	CC_FIELD,
	BCC_FIELD,
	NAME_FIELD
};

enum MENUS {
	// app
	M_NEW = SUBJECT_FIELD + 64,
	M_PREFS,
	M_EDIT_SIGNATURE,
	M_FONT,
	M_STYLE,
	M_SIZE,
	M_BEGINNER,
	M_EXPERT,

	// file
	M_REPLY,
	M_REPLY_TO_SENDER,
	M_REPLY_ALL,
	M_FORWARD,
	M_FORWARD_WITHOUT_ATTACHMENTS,
	M_RESEND,
	M_COPY_TO_NEW,
	M_HEADER,
	M_RAW,
	M_SEND_NOW,
	M_SAVE_AS_DRAFT,
	M_SAVE,
	M_PRINT_SETUP,
	M_PRINT,
	M_DELETE,
	M_DELETE_PREV,
	M_DELETE_NEXT,
	M_CLOSE_READ,
	M_CLOSE_SAVED,
	M_CLOSE_CUSTOM,
	M_STATUS,
	M_READ_POS,

	// edit
	M_SELECT,
	M_QUOTE,
	M_REMOVE_QUOTE,
	M_CHECK_SPELLING,
	M_SIGNATURE,
	M_RANDOM_SIG,
	M_SIG_MENU,
	M_FIND,
	M_FIND_AGAIN,
	M_ACCOUNTS,

	// queries
	M_EDIT_QUERIES,
	M_EXECUTE_QUERY,
	M_QUERY_RECIPIENT,
	M_QUERY_SENDER,
	M_QUERY_SUBJECT,

	// encls
	M_ADD,
	M_REMOVE,
	M_OPEN,
	M_COPY,

	// nav
	M_READ,
	M_UNREAD,
	M_NEXTMSG,
	M_PREVMSG,
	M_SAVE_POSITION,

	// Spam GUI button and menu items.  Order is important.
	M_SPAM_BUTTON,
	M_TRAIN_SPAM_AND_DELETE,
	M_TRAIN_SPAM,
	M_UNTRAIN,
	M_TRAIN_GENUINE,

	M_REDO
};

#endif // _MESSAGES_H

