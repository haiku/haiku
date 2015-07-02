/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "SignalDispositionMenu.h"

#include <new>

#include <MenuItem.h>

#include "SignalDispositionTypes.h"
#include "UiUtils.h"


SignalDispositionMenu::SignalDispositionMenu(const char* label,
	BMessage* baseMessage)
	:
	BMenu(label)
{
	for (int i = 0; i < SIGNAL_DISPOSITION_MAX; i++) {
		BMessage* message = NULL;
		if (baseMessage != NULL) {
			message = new BMessage(*baseMessage);
			message->AddInt32("disposition", i);
		}

		AddItem(new BMenuItem(UiUtils::SignalDispositionToString(i), message));
	}
}


SignalDispositionMenu::~SignalDispositionMenu()
{
}
