// AlertManualTestApp.cpp

#include <stdio.h>
#include "AlertManualTestApp.h"
#include "AlertTestWindow.h"

int main(int argc, char **argv)
{
	AlertManualTestApp app;
	app.Run();
	
	return 0;
}

AlertManualTestApp::AlertManualTestApp()
	: BApplication("application/x-vnd.Haiku-AlertManualTest")
{
	BRect rect(150, 150, 600, 300);
	fMainWindow = new AlertTestWindow(rect);
	
	fMainWindow->Show();
}

