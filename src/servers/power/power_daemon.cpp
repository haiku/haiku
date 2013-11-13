/*
 * Copyright 2005-2013, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Nathan Whitehorn
 */


#include "power_button_monitor.h"

#include <Application.h>
#include <MessageRunner.h>


class PowerManagementDaemon : public BApplication {
	public:
		PowerManagementDaemon();
		virtual ~PowerManagementDaemon();
};


int
main(void)
{
	new PowerManagementDaemon();
	be_app->Run();
	delete be_app;
	return 0;
}


PowerManagementDaemon::PowerManagementDaemon()
	:
	BApplication("application/x-vnd.Haiku-powermanagement")
{
	PowerButtonMonitor *powerButtonMonitor = new PowerButtonMonitor();
	AddHandler(powerButtonMonitor);

	new BMessageRunner(BMessenger(powerButtonMonitor,this),
		new BMessage(POLL_POWER_BUTTON_STATUS), 5e5 /* twice a second */);
}


PowerManagementDaemon::~PowerManagementDaemon()
{
}
