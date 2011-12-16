/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DISPLAY_VIEW_H
#define _DISPLAY_VIEW_H

#include "SettingsPane.h"

class BTextControl;
class BMenu;
class BMenuField;

class DisplayView : public SettingsPane {
public:
							DisplayView(SettingsHost* host);

	virtual	void			AttachedToWindow();
	virtual	void			MessageReceived(BMessage* msg);

			// SettingsPane hooks
			status_t		Load();
			status_t		Save();
			status_t		Revert();

private:
			BTextControl*	fWindowWidth;
			BMenu*			fIconSize;
			BMenuField*		fIconSizeField;

};

#endif // _DISPLAY_VIEW_H
