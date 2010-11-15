/*
 * Copyright 2003-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors in chronological order:
 *		Sikosis
 *		Jérôme Duval
 */
#ifndef MEDIA_H
#define MEDIA_H


#include "MediaWindow.h"

#include <Application.h>
#include <Catalog.h>
#include <Locale.h>


class Media : public BApplication {
public:
								Media();
	virtual						~Media();

	virtual	void				MessageReceived(BMessage* message);
			status_t			InitCheck();

private:
			MediaIcons			fIcons;
			MediaWindow*		fWindow;
};

#endif	// MEDIA_H
