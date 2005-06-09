/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Inspired by SoundCapture from Be newsletter (Media Kit Basics: Consumers and Producers)
 */

#include "RecorderApp.h"
#include "RecorderWindow.h"


RecorderApp::RecorderApp(const char * signature) :
	BApplication(signature), fRecorderWin(NULL)
{
}

RecorderApp::~RecorderApp()
{
}


void
RecorderApp::ReadyToRun()
{
	BApplication::ReadyToRun();
	fRecorderWin = new RecorderWindow();
	fRecorderWin->Show();
}


int
main()
{
	RecorderApp app("application/x-vnd.Haiku-SoundRecorder");
	app.Run();
	return 0;
}