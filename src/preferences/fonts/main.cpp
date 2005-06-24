/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 */
#include "MainWindow.h"
#include "main.h"
#include "FontsSettings.h"

int main(int, char**)
{
	FontApp fontApp;
	fontApp.Run();
	
	return(0);
}

FontApp::FontApp()
 : BApplication("application/x-vnd.Haiku-FNPL")
{
	MainWindow *window = new MainWindow();
	window->Show();
}
