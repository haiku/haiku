/*
 * Copyright 2001-2007, Haiku, Inc.
 * Copyright 2003-2004 Kian Duffy, myob@users.sourceforge.net
 * Parts Copyright 1998-1999 Kazuho Okui and Takashi Murai. 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef APPEARANCE_PREF_VIEW_H
#define APPEARANCE_PREF_VIEW_H


#include "PrefView.h"

class BColorControl;
class BMenu;
class BMenuField;

class TTextControl;


class AppearancePrefView : public PrefView {
	public:
		AppearancePrefView(BRect frame, const char *name, 
			BMessenger messenger);

		virtual	void	Revert();
		virtual void	MessageReceived(BMessage *message);
		virtual void	AttachedToWindow();

		virtual void	GetPreferredSize(float *_width, float *_height);

	private:
		BMenu*			_MakeFontMenu(uint32 command, const char *defaultFont);
		BMenu*			_MakeSizeMenu(uint32 command, uint8 defaultSize);

		BMenuField		*fFont;
		BMenuField		*fFontSize;

		BMenuField		*fColorField;
		BColorControl	*fColorControl;

		BMessenger fAppearancePrefViewMessenger;
};

#endif	// APPEARANCE_PREF_VIEW_H
