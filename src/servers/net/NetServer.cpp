//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#include "NetServer.h"
#include <Application.h>
#include <Alert.h>


static const char *kSignature = NET_SERVER_SIGNATURE;


class NetServerApplication : public BApplication {
	public:
		NetServerApplication();
		
		virtual void AboutRequested();
		virtual void ReadyToRun();
			// loads add-ons
		virtual bool QuitRequested();
			// unloads add-ons
};


int main()
{
	new NetServerApplication();
	
	be_app->Run();
	
	delete be_app;
	
	return 0;
}


NetServerApplication::NetServerApplication()
	: BApplication(kSignature)
{
}


void
NetServerApplication::AboutRequested()
{
	// the alert should not block because we might get _very_ important messages
	(new BAlert("About...",
		"OpenBeOS net_server\n\n"
		"The net_server manages all userland tasks that "
		"cannot be handled by the netstack.",
		"Uhm...Cool?", NULL, NULL, B_WIDTH_AS_USUAL,
		B_INFO_ALERT))->Go(NULL);
}


void
NetServerApplication::ReadyToRun()
{
	// load integrated add-ons
	
	
	// TODO: load add-on binaries
}


bool
NetServerApplication::QuitRequested()
{
	// unload integrated add-ons
	
	
	// TODO: unload add-on binaries
	
	return true;
}
