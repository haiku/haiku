/*
 * Copyright 2004-2006, the Haiku project. All rights reserved.
 * Distributed under the terms of the Haiku License.
 *
 * Authors in chronological order:
 *  mccall@digitalparadise.co.uk
 *  Jérôme Duval
 *  Marcus Overhagen
*/
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <Application.h>
#include <Catalog.h>
#include <Locale.h>

class KeyboardApplication : public BApplication 
{
public:
	KeyboardApplication();
	
	void	MessageReceived(BMessage* message);
	void	AboutRequested(void);
};

#endif
