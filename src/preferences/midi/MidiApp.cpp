/*
 * Copyright 2014, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Application.h>

#include "MidiWindow.h"

class MidiApp : public BApplication {
public:
	MidiApp();
	virtual void ReadyToRun();
};


int main()
{
	MidiApp app;
	app.Run();

	return 0;
}


MidiApp::MidiApp()
	:
	BApplication("application/x-vnd.MidiPreflet")
{

}


/* virtual */
void
MidiApp::ReadyToRun()
{
	(new MidiWindow())->Show();
}
