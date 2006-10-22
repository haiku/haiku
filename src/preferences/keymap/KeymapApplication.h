/*
 * Copyright 2004-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors in chronological order:
 *		Sandor Vroemisse
 *		Jérôme Duval
 */
#ifndef KEYMAP_APPLICATION_H
#define KEYMAP_APPLICATION_H


#include "KeymapWindow.h"

#include <Application.h>
#include <Entry.h>


class KeymapApplication : public BApplication {
	public:
		KeymapApplication();

		void MessageReceived(BMessage* message);
		bool UseKeymap(BEntry* keymap);

	private:
		KeymapWindow* fWindow;
};

#endif // KEYMAP_APPLICATION_H
