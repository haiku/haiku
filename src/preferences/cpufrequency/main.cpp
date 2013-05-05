/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */


#include <Application.h>
#include <Catalog.h>
#include <Locale.h>

#include "CPUFrequencyView.h"
#include "PreferencesWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Main window"


int
main(int argc, char* argv[])
{
	BApplication	*app = new BApplication(kPrefSignature);

	PreferencesWindow<freq_preferences> *window;
	window = new PreferencesWindow<freq_preferences>(
		B_TRANSLATE_SYSTEM_NAME("CPUFrequency"), kPreferencesFileName,
		kDefaultPreferences);
	CPUFrequencyView* prefView
		= new CPUFrequencyView(BRect(0, 0, 400, 350), window);
	window->SetPreferencesView(prefView);
	window->Show();
	app->Run();

	delete app;
	return 0;
}
