/*
 * Copyright 2002-2025, Haiku Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		François Revol (mmu_man)
 *		Humdinger, humdinger@mailbox.org
 */


#include <Application.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <NetworkInterface.h>
#include <NetworkRoster.h>
#include <String.h>

#include <cstdio>
#include <cstdlib>


static int sExitValue = EXIT_FAILURE;
static const char* kProgramName = "waitfor";
static const char* kProgramSig = "application/x-vnd.haiku-waitfor";


#define INTERVAL 100000 // 0.1sec
#define CHECK 'chck'


enum command {
	NONE,
	APP_RUNNING,
	NETWORK,
	THREAD_STARTED,
	THREAD_ENDED
};


class WaitforApp : public BApplication {
public:
	WaitforApp();
	virtual ~WaitforApp();

	virtual void ReadyToRun();
	virtual void ArgvReceived(int32 argc, char** argv);
	virtual void MessageReceived(BMessage* message);

private:
	void _Usage();
	bool _CheckNetworkConnection();

	BMessageRunner* fRunner;
	BString fThreadName;
	BString fAppSig;
	command fOption;
};


WaitforApp::WaitforApp()
	:
	BApplication(kProgramSig),
	fThreadName(NULL),
	fAppSig(NULL),
	fOption(NONE)
{
	BMessage message(CHECK);
	fRunner = new BMessageRunner(this, &message, INTERVAL);
}


WaitforApp::~WaitforApp()
{
	stop_watching_network(this);
	delete fRunner;
}


void
WaitforApp::ArgvReceived(int32 argc, char** argv)
{
	if (argc == 2 && strcmp(argv[1], "-n") == 0) {
		fOption = NETWORK;
	} else if (argc == 2) {
		fOption = THREAD_STARTED;
		fThreadName = argv[1];
	} else if (argc == 3 && strcmp(argv[1], "-e") == 0) {
		fOption = THREAD_ENDED;
		fThreadName = argv[2];
	} else if (argc == 3 && strcmp(argv[1], "-m") == 0) {
		fOption = APP_RUNNING;
		fAppSig = argv[2];
	} else
		fOption = NONE;
}


void
WaitforApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case CHECK:
		{
			switch (fOption) {
				case APP_RUNNING:
				{
					BMessenger messenger(fAppSig);
					if (messenger.IsValid()) {
						sExitValue = EXIT_SUCCESS;
						PostMessage(B_QUIT_REQUESTED);
					}
					break;
				}
				case THREAD_STARTED:
				{
					if (find_thread(fThreadName) >= 0) {
						sExitValue = EXIT_SUCCESS;
						PostMessage(B_QUIT_REQUESTED);
					}
					break;
				}
				case THREAD_ENDED:
				{
					if (find_thread(fThreadName) < 0) {
						sExitValue = EXIT_SUCCESS;
						PostMessage(B_QUIT_REQUESTED);
					}
					break;
				}
				case NONE:
				{
					_Usage();
					PostMessage(B_QUIT_REQUESTED);
					break;
				}
				default:
					break;
			}
			break;
		}
		case B_NETWORK_MONITOR:
		{
			if (_CheckNetworkConnection())
				PostMessage(B_QUIT_REQUESTED);
			break;
		}
		default:
		{
			BApplication::MessageReceived(message);
			break;
		}
	}
}


void
WaitforApp::ReadyToRun()
{
	if (fOption == NETWORK) {
		if (_CheckNetworkConnection())
			PostMessage(B_QUIT_REQUESTED);
		else {
			start_watching_network(
				B_WATCH_NETWORK_INTERFACE_CHANGES | B_WATCH_NETWORK_LINK_CHANGES, this);
		}
	}
}


bool
WaitforApp::_CheckNetworkConnection()
{
	BNetworkRoster& roster = BNetworkRoster::Default();
	BNetworkInterface interface;
	uint32 cookie = 0;
	while (roster.GetNextInterface(&cookie, interface) == B_OK) {
		uint32 flags = interface.Flags();
		if ((flags & IFF_LOOPBACK) == 0 && (flags & (IFF_UP | IFF_LINK)) == (IFF_UP | IFF_LINK)) {
			sExitValue = EXIT_SUCCESS;
			return true;
		}
	}
	// No network connection detected
	return false;
}


void
WaitforApp::_Usage()
{
	BString usageText("Usage:\n"
					  "  %s <thread_name>\n"
					  "      wait until a thread with 'thread_name' has been started.\n\n"
					  "  %s -e <thread_name>\n"
					  "      wait until all threads with thread_name have ended.\n\n"
					  "  %s -m <app_signature>\n"
					  "      wait until the application specified by 'app_signature' is "
					  "ready to receive messages.\n\n"
					  "  %s -n\n"
					  "      wait until the network connection is up.\n");
	usageText.ReplaceAll("%s", kProgramName);
	fprintf(stderr, usageText);
}


int
main(int argc, char** argv)
{
	WaitforApp waitforApp;
	waitforApp.Run();

	return sExitValue;
}
