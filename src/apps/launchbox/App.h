/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef APP_H
#define APP_H

#include <Application.h>
#include <List.h>

class MainWindow;

class App : public BApplication {
 public:
								App();
	virtual						~App();

	virtual	bool				QuitRequested();
	virtual	void				ReadyToRun();
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				AboutRequested();

 private:
};

#endif // APP_H
