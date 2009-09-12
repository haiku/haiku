/*
 * Copyright 2001-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mark Hogben
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H


#include "FontsSettings.h"

#include <Window.h>

class BButton;
class BMessageRunner;
class FontView;


class MainWindow : public BWindow {
public:
							MainWindow();
	virtual					~MainWindow();

	virtual	bool			QuitRequested();
	virtual	void			MessageReceived(BMessage *message);


private:
		void				_Center();

		BMessageRunner*		fRunner;
		FontView*			fFontsView;
		BButton*			fDefaultsButton;
		BButton*			fRevertButton;

		FontsSettings		fSettings;
};

static const int32 kMsgUpdate = 'updt';

#endif	// MAIN_WINDOW_H
