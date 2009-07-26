/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Inspired by SoundCapture from Be newsletter (Media Kit Basics: Consumers and Producers)
 */

#include "RecorderApp.h"
#include "RecorderWindow.h"


RecorderApp::RecorderApp(const char* signature)
	: BApplication(signature), fRecorderWin(NULL)
{
	fRecorderWin = new RecorderWindow();
}


RecorderApp::~RecorderApp()
{
}


status_t
RecorderApp::InitCheck()
{
	if (fRecorderWin)
		return fRecorderWin->InitCheck();
	return B_OK;
}


int
main()
{
	RecorderApp app("application/x-vnd.Haiku-SoundRecorder");
	if (app.InitCheck() == B_OK)
		app.Run();
	return 0;
}
