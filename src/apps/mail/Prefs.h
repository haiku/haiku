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
#ifndef _PREFS_H
#define _PREFS_H


#include <Font.h>
#include <Window.h>

class BButton;
class BMenu;
class BPopUpMenu;
class BTextControl;


#define ACCOUNT_USE_DEFAULT	0
#define ACCOUNT_FROM_MAIL	1

#define	PREF_WIDTH			340
#define PREF_HEIGHT			330

struct EncodingItem {
			char*				name;
			uint32				flavor;
};

extern const EncodingItem kEncodings[];


class TPrefsWindow : public BWindow {
public:
								TPrefsWindow(BRect rect, BFont* font,
									int32* level, bool* warp,
									bool* attachAttributes, bool* cquotes,
									int32* account, int32* replyTo,
									char** preamble, char** sig,
									uint32* encoding, bool* warnUnencodable,
									bool* spellCheckStartOn,
									bool* autoMarkRead, uint8* buttonBar);
	virtual						~TPrefsWindow();

	virtual	void				MessageReceived(BMessage* message);

private:
			BPopUpMenu*			_BuildFontMenu(BFont*);
			BPopUpMenu*			_BuildLevelMenu(int32);
			BPopUpMenu*			_BuildAccountMenu(int32);
			BPopUpMenu*			_BuildReplyToMenu(int32);
			BMenu*				_BuildReplyPreambleMenu();
			BPopUpMenu*			_BuildSignatureMenu(char*);
			BPopUpMenu*			_BuildSizeMenu(BFont*);
			BPopUpMenu*			_BuildWrapMenu(bool);
			BPopUpMenu*			_BuildAttachAttributesMenu(bool);
			BPopUpMenu*			_BuildColoredQuotesMenu(bool quote);
			BPopUpMenu*			_BuildEncodingMenu(uint32 encoding);
			BPopUpMenu*			_BuildWarnUnencodableMenu(
									bool warnUnencodable);
			BPopUpMenu*			_BuildSpellCheckStartOnMenu(
									bool spellCheckStartOn);
			BPopUpMenu*			_BuildAutoMarkReadMenu(
									bool autoMarkRead);
			BPopUpMenu*			_BuildButtonBarMenu(uint8 show);
			
			BPopUpMenu*			_BuildBoolMenu(uint32 msg,
									const char* boolItem, bool isTrue);

			bool*				fNewWrap;
			bool				fWrap;
			bool*				fNewAttachAttributes;
			bool				fAttachAttributes;
			uint8*				fNewButtonBar;
			uint8				fButtonBar;
			bool*				fNewColoredQuotes;
			bool				fColoredQuotes;
			int32*				fNewAccount;
			int32				fAccount;
			int32*				fNewReplyTo;
			int32				fReplyTo;

			char**				fNewPreamble;

			char**				fNewSignature;
			char*				fSignature;
			BFont*				fNewFont;
			BFont				fFont;
			uint32*				fNewEncoding;
			uint32				fEncoding;
			bool*				fNewWarnUnencodable;
			bool				fWarnUnencodable;
			bool*				fNewSpellCheckStartOn;
			bool				fSpellCheckStartOn;
			bool*				fNewAutoMarkRead;
			bool				fAutoMarkRead;

			BButton*			fRevert;

			BPopUpMenu*			fFontMenu;
			BPopUpMenu*			fSizeMenu;
			BPopUpMenu*			fWrapMenu;
			BPopUpMenu*			fColoredQuotesMenu;
			BPopUpMenu*			fAttachAttributesMenu;
			BPopUpMenu*			fAccountMenu;
			BPopUpMenu*			fReplyToMenu;
			BMenu*				fReplyPreambleMenu;
			BTextControl*		fReplyPreamble;
			BPopUpMenu*			fSignatureMenu;
			BPopUpMenu*			fEncodingMenu;
			BPopUpMenu*			fWarnUnencodableMenu;
			BPopUpMenu*			fSpellCheckStartOnMenu;
			BPopUpMenu*			fButtonBarMenu;
			BPopUpMenu*			fAutoMarkReadMenu;
};

#endif	/* _PREFS_H */

