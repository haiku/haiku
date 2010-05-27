/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 */
#ifndef _GENERAL_VIEW_H
#define _GENERAL_VIEW_H

#include "SettingsPane.h"

class BCheckBox;
class BButton;
class BStringView;
class BTextControl;

class GeneralView : public SettingsPane {
public:
							GeneralView(SettingsHost* host);

	virtual	void			AttachedToWindow();
	virtual	void			MessageReceived(BMessage* msg);

			// SettingsPane hooks
			status_t		Load();
			status_t		Save();
			status_t		Revert();

private:
		BButton*			fServerButton;
		BStringView*		fStatusLabel;
		BCheckBox*			fAutoStart;
		BTextControl*		fTimeout;
		BCheckBox*			fHideAll;

		bool				_CanFindServer(entry_ref* ref);
		bool				_IsServerRunning();
};

#endif // _GENERAL_VIEW_H
