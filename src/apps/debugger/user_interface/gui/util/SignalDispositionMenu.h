/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef SIGNAL_DISPOSITION_MENU_H
#define SIGNAL_DISPOSITION_MENU_H


#include <Menu.h>


class SignalDispositionMenu : public BMenu {
public:
								SignalDispositionMenu(const char* label,
									BMessage* baseMessage = NULL);
	virtual						~SignalDispositionMenu();
};


#endif // SIGNAL_DISPOSITION_MENU_H
