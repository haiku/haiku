/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <Window.h>


enum {
	MSG_MAIN_WINDOW_CLOSED		= 'mwcl',
};


class MainWindow : public BWindow {
public:
								MainWindow(BRect frame);
	virtual						~MainWindow();

	// BWindow interface
	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);

private:
};

#endif // MAIN_WINDOW_H
