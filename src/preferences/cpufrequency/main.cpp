/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */

#include "PreferencesWindow.h"
#include "CPUFrequencyView.h"
#include <Application.h>


int 
main(int argc, char* argv[])
{
	BApplication	*app = new BApplication(kPrefSignature);
	PreferencesWindow<freq_preferences> *window;
	window = new PreferencesWindow<freq_preferences>("CPU Frequency",
														kPreferencesFileName,
														kDefaultPreferences);
	CPUFrequencyView* prefView = new CPUFrequencyView(BRect(0, 0, 400, 350),
														window);
	window->SetPreferencesView(prefView);
	window->Show();
	app->Run();
	
	delete app;
	return 0;
}
