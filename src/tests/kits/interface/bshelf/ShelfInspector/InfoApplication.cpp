// InfoApplication.cpp



#include "InfoApplication.h"

const char *XCONTAINER_APP		= "application/x-vnd.reh-XContainer";


#define	CMD_UPDATE_CONTAINER_ITEM  'updC'


InfoApplication :: InfoApplication()
		  		  		 : IEApplication("application/x-vnd.reh-XShelfInspector")
{
	// instantiate a new window
	fLaunched = true;
	fInfoWindow = new InfoWindow();
}

InfoApplication :: ~InfoApplication()
{
	be_roster -> StopWatching(fMsgnr);	
}


void InfoApplication :: MessageReceived(BMessage *msg)
{
	switch (msg->what) 
	{			
		case B_SOME_APP_QUIT:
		case B_SOME_APP_ACTIVATED:
		case B_SOME_APP_LAUNCHED:
			{
				if (  (be_roster -> IsRunning(XCONTAINER_APP)) == false )
				{
					be_roster->Launch(XCONTAINER_APP);				// Launch the XContainer 
				
					if (fLaunched == true)
					{
					 	fInfoWindow -> GetPrefs();
						fInfoWindow -> Show();
				 		fLaunched = false;
					 	break;
					}
					BMessenger	messenger;
					BMessage	msg(CMD_UPDATE_CONTAINER_ITEM);	// Update XContainer
					messenger = BMessenger(fInfoWindow);					
					messenger.SendMessage(&msg);					
				}
			}
			break;

	default:
		BApplication :: MessageReceived(msg);
	break;
	}
}

void InfoApplication::ReadyToRun()
{		
	fMsgnr = BMessenger(this);
	be_roster -> StartWatching(fMsgnr);
	PostMessage(B_SOME_APP_LAUNCHED);
}


int main()
{	
	InfoApplication	infoApplication;
	infoApplication.Run();
	return(0);
}
