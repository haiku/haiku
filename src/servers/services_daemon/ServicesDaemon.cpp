/*
 * Copyright (c) 2005-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@earthlink.net>
 */
#include "ServicesDaemon.h"
#include <String.h>
#include <Alert.h>
#include <OS.h>
#include <MediaDefs.h>
#include <be_apps/ServicesDaemon/ServicesDaemon.h>

enum {
	M_RELAUNCH_DESKBAR = 'rtsk',
	M_RELAUNCH_TRACKER,
	M_RELAUNCH_MAIL,
	M_RELAUNCH_PRINT,
	M_RELAUNCH_AUDIO,
	M_RELAUNCH_MEDIA_ADDONS,
	M_RELAUNCH_MEDIA,
	M_RELAUNCH_MIDI
};

// Time delay in microseconds - 1 second
const bigtime_t kRelaunchDelay = 1000000;

const char *gSignatures[] = {
	"application/x-vnd.Be-TSKB",
	"application/x-vnd.Be-TRAK",
	"application/x-vnd.Be-POST",
	"application/x-vnd.Be-PSRV",
	"application/x-vnd.Be-AUSV",
	"application/x-vnd.Be.addon-host",
	"application/x-vnd.Be.media-server",
	"application/x-vnd.be-midi-roster",
	NULL
};

static sem_id sRelaunchSem;
static sem_id sQuitSem;



App::App(void)
 :	BApplication("application/x-vnd.Haiku-ServicesDaemon"),
 	fStatus(B_NO_INIT)
{
	sRelaunchSem = create_sem(1,"relaunch_sem");
	sQuitSem = create_sem(1,"quit_sem");
	
	// We will release this when we want the relauncher thread to quit
	acquire_sem(sQuitSem);
	
	// We will release this semaphore when we want to activate the relauncher
	// thread
	acquire_sem(sRelaunchSem);
	
	fRelauncherID = spawn_thread(App::RelauncherThread, "relauncher_thread",
								B_NORMAL_PRIORITY,this);
	resume_thread(fRelauncherID);
	
	fStatus = be_roster->StartWatching(be_app_messenger, B_REQUEST_QUIT);
	if (fStatus != B_OK)
		PostMessage(B_QUIT_REQUESTED);
}


App::~App(void)
{
	// Release in this order so that there isn't a race condition when quitting.
	// The thread waits
	release_sem(sQuitSem);
	release_sem(sRelaunchSem);
	
	int32 dummyval;
	wait_for_thread(fRelauncherID, &dummyval);
	
	for (int32 i = 0; i < fSignatureList.CountItems(); i++) {
		BString *item = (BString*) fSignatureList.ItemAt(i);
		delete item;
	}
}


bool
App::QuitRequested(void)
{
	if (fStatus == B_OK)
		be_roster->StopWatching(be_app_messenger);
	
	return true;
}


void
App::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case B_SOME_APP_QUIT: {
			BString string;
			if (msg->FindString("be:signature",&string) != B_OK)
				return;
			
			// See if the signature belongs to an app that has requested
			// a restart
			for (int32 i = 0; i < fSignatureList.CountItems(); i++) {
				BString *item = (BString*) fSignatureList.ItemAt(i);
				if (string.Compare(*item) == 0) {
					fSignatureList.RemoveItem(item);
					delete item;
					
					be_roster->Launch(string.String());
					return;
				}
			}
			
			int i = 0;
			while (gSignatures[i]) {
				// Checking to see if it's one of the supported signatures
				// and if it is, add a relaunch message to the queue which is
				// to be posted here at the
				if (string.Compare(gSignatures[i]) == 0) {
					BMessage *launchmsg = new BMessage(M_RELAUNCH_DESKBAR + i);
					fRelaunchQueue.AddMessage(launchmsg);
					
					release_sem(sRelaunchSem);
					acquire_sem(sRelaunchSem);
				}
				i++;
			}
			break;
		}
		case B_SERVICES_DAEMON_RESTART: {
			BString signature;
			if (msg->FindString("signature",&signature) == B_OK)
				fSignatureList.AddItem(new BString(signature));
			break;
		}
		case M_RELAUNCH_DESKBAR: {
			be_roster->Launch(gSignatures[msg->what - M_RELAUNCH_DESKBAR]);
			break;
		}
		case M_RELAUNCH_TRACKER: {
			be_roster->Launch(gSignatures[msg->what - M_RELAUNCH_DESKBAR]);
			break;
		}
		case M_RELAUNCH_MAIL: {
			be_roster->Launch(gSignatures[msg->what - M_RELAUNCH_DESKBAR]);
			break;
		}
		case M_RELAUNCH_PRINT: {
			be_roster->Launch(gSignatures[msg->what - M_RELAUNCH_DESKBAR]);
			break;
		}
		case M_RELAUNCH_AUDIO: {
			be_roster->Launch(gSignatures[msg->what - M_RELAUNCH_DESKBAR]);
			break;
		}
		case M_RELAUNCH_MEDIA_ADDONS:
		case M_RELAUNCH_MEDIA: {
			shutdown_media_server();
			launch_media_server();
			break;
		}
		case M_RELAUNCH_MIDI: {
			be_roster->Launch(gSignatures[msg->what - M_RELAUNCH_DESKBAR]);
			break;
		}
		
		default:
			BApplication::MessageReceived(msg);
	}
}


int32
App::RelauncherThread(void *data)
{
	App *app = (App*) data;
	
	sem_info quitSemInfo;
	get_sem_info(sQuitSem, &quitSemInfo);
	
	while (quitSemInfo.count > -1) {
		// wait until we need to check for a relaunch
		acquire_sem(sRelaunchSem);
		
		// We can afford to do this because while we have the semaphore, the
		// app itself is locked.
		BMessage *msg = app->fRelaunchQueue.NextMessage();
		
		release_sem(sRelaunchSem);
		
		get_sem_info(sQuitSem, &quitSemInfo);
		if (quitSemInfo.count > 0) {
			if (msg)
				delete msg;
			break;
		}
		
		if (msg) {
			// Attempt to work around R5 shutdown procedures by checking for 
			// debug_server running state. If it's shut down, then chances 
			// are that the system is shutting down or has already done so.
			// We use the debug server in particular because it's one of 
			// the few services that is available in safe mode which is 
			// closed on system shutdown.
			
			if (be_roster->IsRunning("application/x-vnd.Be-DBSV")) {
				snooze(kRelaunchDelay);
				app->PostMessage(msg->what);
				delete msg;
			}
		}
		
		get_sem_info(sQuitSem, &quitSemInfo);
	}
	
	return 0;
}


int
main(void)
{
	App *app = new App();
	app->Run();
	delete app;
	return 0;
}
