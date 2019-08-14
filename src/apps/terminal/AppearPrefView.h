/*
 * Copyright 2001-2010, Haiku, Inc.
 * Copyright 2003-2004 Kian Duffy, myob@users.sourceforge.net
 * Parts Copyright 1998-1999 Kazuho Okui and Takashi Murai.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef APPEARANCE_PREF_VIEW_H
#define APPEARANCE_PREF_VIEW_H


#include <GroupView.h>
#include <Messenger.h>
#include <String.h>


static const uint32 MSG_HALF_FONT_CHANGED				= 'mchf';
static const uint32 MSG_HALF_SIZE_CHANGED				= 'mchs';
static const uint32 MSG_FULL_FONT_CHANGED				= 'mcff';
static const uint32 MSG_FULL_SIZE_CHANGED				= 'mcfs';
static const uint32 MSG_COLOR_FIELD_CHANGED				= 'mccf';
static const uint32 MSG_COLOR_CHANGED					= 'mcbc';
static const uint32 MSG_COLOR_SCHEME_CHANGED			= 'mccs';

static const uint32 MSG_TAB_TITLE_SETTING_CHANGED		= 'mtts';
static const uint32 MSG_WINDOW_TITLE_SETTING_CHANGED	= 'mwts';
static const uint32 MSG_BLINK_CURSOR_CHANGED			= 'mbcc';
static const uint32 MSG_ALLOW_BOLD_CHANGED				= 'mabc';
static const uint32 MSG_USE_OPTION_AS_META_CHANGED		= 'momc';
static const uint32 MSG_WARN_ON_EXIT_CHANGED			= 'mwec';
static const uint32 MSG_COLS_CHANGED					= 'mccl';
static const uint32 MSG_HISTORY_CHANGED					= 'mhst';
// TODO MSG_ENCODING_CHANGED?

static const uint32 MSG_PREF_MODIFIED					= 'mpmo';


struct color_scheme;
class BCheckBox;
class BColorControl;
class BMenu;
class BMenuField;
class BPopUpMenu;
class BTextControl;


class AppearancePrefView : public BGroupView {
public:
								AppearancePrefView(const char* name,
									const BMessenger &messenger);

	virtual	void				Revert();
	virtual void				MessageReceived(BMessage* message);
	virtual void				AttachedToWindow();

private:
			void				_ChangeColorScheme(color_scheme* scheme);
			void				_SetCurrentColorScheme();
			void				_SetEncoding(const char* encoding);
			void				_SetWindowSize(int rows, int cols);

	static	BMenu*				_MakeFontMenu(uint32 command,
									const char* defaultFamily,
									const char* defaultStyle);
	static	BMenu*				_MakeFontSizeMenu(const char* label,
									uint32 command,
									const char* family,
									const char* style,
									const char* size);

	static	BPopUpMenu*			_MakeMenu(uint32 msg, const char** items,
										const char* defaultItem);

	static	BPopUpMenu*			_MakeColorSchemeMenu(uint32 msg,
									const color_scheme** schemes,
									const color_scheme* defaultItemName);

			void				_MarkSelectedFont(const char* family,
									const char* style, const char* size);

			BCheckBox*			fBlinkCursor;
			BCheckBox*			fAllowBold;
			BCheckBox*			fUseOptionAsMetaKey;
			BCheckBox*			fWarnOnExit;
			BMenuField*			fFontField;

			BMenuField*			fWindowSizeField;
			BMenuField*			fEncodingField;

			BMenuField*			fColorSchemeField;
			BMenuField*			fColorField;
			BColorControl*		fColorControl;

			BTextControl*		fTabTitle;
			BTextControl*		fWindowTitle;

			BMessenger			fTerminalMessenger;
};


#endif	// APPEARANCE_PREF_VIEW_H
