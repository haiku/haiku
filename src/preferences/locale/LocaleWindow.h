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

	virtual	void			MessageReceived(BMessage* message);
	virtual	void			FrameMoved(BPoint newPosition);

private:
			BButton*		fRevertButton;
			BListView*		fLanguageListView;
			BListView*		fPreferredListView;
			FormatView*		fFormatView;
};

#endif	// LOCALE_WINDOW_H

