/*
 * Copyright 2004-2006 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Sandor Vroemisse
 *		Jérôme Duval
 */
 
#ifndef KEYMAP_APPLICATION_H
#define KEYMAP_APPLICATION_H


#include "KeymapWindow.h"

#include <Application.h>
#include <Catalog.h>
#include <Entry.h>
#include <Locale.h>


class KeymapApplication : public BApplication {
	public:
		KeymapApplication();

		void MessageReceived(BMessage* message);
		bool UseKeymap(BEntry* keymap);

	private:
		KeymapWindow* fWindow;
};

#endif // KEYMAP_APPLICATION_H
