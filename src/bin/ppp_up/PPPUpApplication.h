/*
 * Copyright 2005, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#ifndef PPP_UP_APPLICATION__H
#define PPP_UP_APPLICATION__H

#include <Application.h>
#include <String.h>
#include <PPPInterfaceListener.h>

class ConnectionWindow;

#define APP_SIGNATURE "application/x-vnd.haiku.ppp_up"


class PPPUpApplication : public BApplication {
	public:
		PPPUpApplication(const char *interfaceName);
		
		virtual void ReadyToRun();

	private:
		BString fInterfaceName;
};


#endif
