/*
 * Copyright 2004-2015 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <Locale.h>
#include <Window.h>

#include "NetworkSetupWindow.h"


static const char* kSignature = "application/x-vnd.Haiku-Network";


class Application : public BApplication {
public:
								Application();

public:
	virtual	void				ReadyToRun();
};


Application::Application()
	:
	BApplication(kSignature)
{
}


void
Application::ReadyToRun()
{
	NetworkSetupWindow* window = new NetworkSetupWindow();
	window->Show();
}


// #pragma mark -


int
main()
{
	Application* app = new Application();
	app->Run();
	delete app;
	return 0;
}
