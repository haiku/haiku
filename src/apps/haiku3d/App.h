/*
 * Copyright 2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexandre Deckner <alex@zappotek.com>
 */

#ifndef _APP_H
#define _APP_H

#include <Application.h>

class App: public BApplication {
protected:
	BWindow*		fMainWindow;
public:
					App();
					~App();

	virtual void	AboutRequested();
	virtual void	MessageReceived(BMessage *message);

};

#endif /* _APP_H */
