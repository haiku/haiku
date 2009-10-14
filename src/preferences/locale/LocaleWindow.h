/*
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LOCALE_WINDOW_H
#define LOCALE_WINDOW_H


#include <Window.h>

class BButton;
class BListView;
class FormatView;


class LocaleWindow : public BWindow {
public:
							LocaleWindow();

	virtual	bool			QuitRequested();
	virtual	void			MessageReceived(BMessage* message);

private:
			BButton*		fRevertButton;
			BListView*		fPreferredListView;
			FormatView*		fFormatView;
};

#endif	// LOCALE_WINDOW_H

