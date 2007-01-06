#ifndef SERVICES_DAEMON_H
#define SERVICES_DAEMON_H

#include <Application.h>
#include <Roster.h>
#include <MessageQueue.h>

class App : public BApplication
{
public:
						App(void);
						~App(void);
			bool		QuitRequested(void);
			void		MessageReceived(BMessage *msg);
	
private:
	static	int32		RelauncherThread(void *data);
	
			thread_id		fRelauncherID;
			status_t		fStatus;
			BMessageQueue	fRelaunchQueue;
			
			BList			fSignatureList;
};


#endif
