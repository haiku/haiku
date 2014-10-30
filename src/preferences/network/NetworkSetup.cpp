/*
 * Copyright 2004-2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <Locale.h>
#include <Window.h>

#include "NetworkSetupWindow.h"


#define SOFTWARE_EDITOR			"Haiku"
#define NAME					"NetworkSetup"
#define SOFTWARE_VERSION_LABEL	"0.1.0 alpha"

#define APPLICATION_SIGNATURE	"application/x-vnd." SOFTWARE_EDITOR "-" NAME


class Application : public BApplication
{
	public:
		Application();

	public:
		void			ReadyToRun(void);
};


int main()
{
	Application* app = new Application();
	app->Run();
	delete app;
	return 0;
}


Application::Application()
	: BApplication(APPLICATION_SIGNATURE)
{
}


void
Application::ReadyToRun(void)
{
	NetworkSetupWindow* window = new NetworkSetupWindow(NAME);
	window->Show();
}
