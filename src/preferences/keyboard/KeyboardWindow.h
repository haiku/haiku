/*
 * Copyright 2004-2006, the Haiku project. All rights reserved.
 * Distributed under the terms of the Haiku License.
 *
 * Authors in chronological order:
 *  mccall@digitalparadise.co.uk
 *  Jérôme Duval
 *  Marcus Overhagen
*/
#ifndef KEYBOARD_WINDOW_H
#define KEYBOARD_WINDOW_H

#include <Button.h>
#include <Window.h>

#include "KeyboardSettings.h"
#include "KeyboardView.h"

class KeyboardWindow : public BWindow 
{
public:
			KeyboardWindow();
	
	bool	QuitRequested();
	void	MessageReceived(BMessage* message);
	
private:
	KeyboardView	*fSettingsView;
	KeyboardSettings	fSettings;
	BButton		*fDefaultsButton;
	BButton		*fRevertButton;
};

#endif
