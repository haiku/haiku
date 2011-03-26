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


class App : public BApplication {
public:
							App();
	virtual					~App();

	virtual	void			ReadyToRun();
	virtual	bool			QuitRequested();

protected:
			BWindow*		fMainWindow;
};

#endif /* _APP_H */
