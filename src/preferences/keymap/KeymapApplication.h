/*
 * Copyright 2004-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		John Scipione, jscipione@gmail.com
 *		Sandor Vroemisse
 */
#ifndef KEYMAP_APPLICATION_H
#define KEYMAP_APPLICATION_H


#include "KeymapWindow.h"

#include <Application.h>
#include <Catalog.h>
#include <Entry.h>
#include <Locale.h>

#include "ModifierKeysWindow.h"


static const uint32 kMsgShowModifierKeysWindow = 'smkw';
static const uint32 kMsgCloseModifierKeysWindow = 'hmkw';
static const uint32 kMsgUpdateModifierKeys = 'umod';
static const uint32 kMsgUpdateNormalKeys = 'ukey';


class KeymapApplication : public BApplication {
public:
		KeymapApplication();

		void					MessageReceived(BMessage* message);
		bool					UseKeymap(BEntry* keymap);

protected:
		void					_ShowModifierKeysWindow();

private:
		KeymapWindow*			fWindow;
		ModifierKeysWindow*		fModifierKeysWindow;
};


#endif	// KEYMAP_APPLICATION_H
