/*
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT/X11 license.
 *
 * Copyright (c) 1999 Mike Steed. You are free to use and distribute this software
 * as long as it is accompanied by it's documentation and this copyright notice.
 * The software comes with no warranty, etc.
 */
#ifndef APP_H
#define APP_H


#include <Application.h>


class MainWindow;

class App: public BApplication {
public:
								App();
	virtual						~App();

	virtual	void				ArgvReceived(int32 argc, char** argv);
	virtual	void				RefsReceived(BMessage* message);

	virtual	void				ReadyToRun();
	virtual	bool				QuitRequested();

private:
			MainWindow*			fMainWindow;
			BMessage*			fSavedRefsReceived;
};

#endif // APP_H
