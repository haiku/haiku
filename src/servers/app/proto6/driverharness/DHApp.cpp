#include "DHApp.h"
#include "DisplayDriver.h"
#include "ViewDriver.h"

DHApp::DHApp(void) : BApplication("application/x-vnd.obos-DDriverHarness")
{
	driver=new ViewDriver;
	driver->Initialize();
}

DHApp::~DHApp(void)
{
	driver->Shutdown();
	delete driver;
}

int main(void)
{
	DHApp *app=new DHApp;
	app->Run();
	delete app;
	return 0;
}