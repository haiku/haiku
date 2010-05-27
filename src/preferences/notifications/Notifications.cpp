/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 */

#include <Application.h>

#include "PrefletWin.h"

class PrefletApp : public BApplication {
public:
						PrefletApp();

	virtual	void		ReadyToRun();
	virtual	bool		QuitRequested();

private:
			PrefletWin*	fWindow;
};


PrefletApp::PrefletApp()
	:
	BApplication("application/x-vnd.Haiku-Notifications")
{
}


void
PrefletApp::ReadyToRun()
{
	fWindow = new PrefletWin;
}


bool
PrefletApp::QuitRequested()
{
	return true;
}


int
main(int argc, char* argv[])
{
	PrefletApp app;
	app.Run();
	return 0;
}
