/*
 * Copyright 2005, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#include "PPPUpApplication.h"
#include "ConnectionWindow.h"

#include <KPPPUtils.h>
#include <PPPReportDefs.h>


int
main(int argc, const char *argv[])
{
	if(argc != 2)
		return -1;
	
	const char *interfaceName = argv[1];
	
	new PPPUpApplication(interfaceName);
	be_app->Run();
	delete be_app;
	
	return 0;
}


PPPUpApplication::PPPUpApplication(const char *interfaceName)
	: BApplication(APP_SIGNATURE),
	fInterfaceName(interfaceName)
{
}


void
PPPUpApplication::ReadyToRun()
{
	BRect rect(150, 50, 450, 435);
		// TODO: center rect on screen
	(new ConnectionWindow(rect, fInterfaceName))->Run();
}
