/*
 * Copyright 2001-2007, Haiku, Inc.
 * Copyright 2003-2004 Kian Duffy, myob@users.sourceforge.net
 * Parts Copyright 1998-1999 Kazuho Okui and Takashi Murai. 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef APPEARANCE_PREF_VIEW_H
#define APPEARANCE_PREF_VIEW_H


#include <Messenger.h>
#include <String.h>
#include <View.h>


const ulong MSG_HALF_FONT_CHANGED	= 'mchf';
const ulong MSG_HALF_SIZE_CHANGED	= 'mchs';
const ulong MSG_FULL_FONT_CHANGED	= 'mcff';
const ulong MSG_FULL_SIZE_CHANGED	= 'mcfs';
const ulong MSG_COLOR_FIELD_CHANGED	= 'mccf';
const ulong MSG_COLOR_CHANGED		= 'mcbc';

const ulong MSG_COLS_CHANGED            = 'mccl';
const ulong MSG_ROWS_CHANGED            = 'mcrw';
const ulong MSG_HISTORY_CHANGED         = 'mhst';

const ulong MSG_PREF_MODIFIED		= 'mpmo';


class BColorControl;
class BMenu;
class BMenuField;
class BPopUpMenu;

class AppearancePrefView : public BView {
public:
								AppearancePrefView(const char *name,
									const BMessenger &messenger);

	virtual	void				Revert();
	virtual void				MessageReceived(BMessage *message);
	virtual void				AttachedToWindow();

	virtual	void				GetPreferredSize(float *_width,
									float *_height);

private:
	static	BMenu*				_MakeFontMenu(uint32 command,
									const char *defaultFamily,
									const char *defaultStyle);
	static	BMenu*				_MakeSizeMenu(uint32 command,
									uint8 defaultSize);
	
	static	BPopUpMenu*			_MakeMenu(uint32 msg, const char **items,
									const char *defaultItemName);
			
			BMenuField			*fFont;
			BMenuField			*fFontSize;

			BMenuField			*fColorField;
			BColorControl		*fColorControl;

			BMessenger			fTerminalMessenger;
};

#endif	// APPEARANCE_PREF_VIEW_H
