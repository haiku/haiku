/* 
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

// Small app for testing the registrar's MessageDeliverer.

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <Application.h>
#include <Clipboard.h>
#include <Locker.h>
#include <Message.h>
#include <MessageRunner.h>
#include <OS.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>

// message what codes
enum {
	MSG_FILL_PORT	= 'fill',
	MSG_RUNNER		= 'runn',
	MSG_BROADCAST	= 'brod',
};

static const uint32 sExpectedMessages[3] = {
	B_CLIPBOARD_CHANGED,
	MSG_RUNNER,
	MSG_BROADCAST,
};
static const int32 sExpectedMessageCount = 3;
static int32 sExpectedMessageIndex = 0;

// check_expected_message
static
bool
check_expected_message(uint32 what)
{
	if (sExpectedMessageIndex >= sExpectedMessageCount) {
		printf("Unexpected message '%.4s'\n", (char*)&what);
		exit(1);
	}

	uint32 expected = sExpectedMessages[sExpectedMessageIndex++];
	if (what != expected) {
		printf("Unexpected message '%.4s', expected was: '%.4s'\n",
			(char*)&what, (char*)&expected);
		exit(1);
	}

	return (sExpectedMessageIndex == sExpectedMessageCount);
}


// TestApp
class TestApp : public BApplication {
public:
	TestApp()
		: BApplication("application/x-vnd.haiku.message-deliverer-test")
	{
	}

	virtual void MessageReceived(BMessage *message)
	{
		switch (message->what) {
			case MSG_FILL_PORT:
				return;
			case MSG_RUNNER:
				printf("MSG_RUNNER\n");
				break;
			case B_CLIPBOARD_CHANGED:
				printf("B_CLIPBOARD_CHANGED\n");
				break;
			case MSG_BROADCAST:
				printf("MSG_BROADCAST\n");
				break;
			default:
				BApplication::MessageReceived(message);
				return;
		}

		if (check_expected_message(message->what))
			PostMessage(B_QUIT_REQUESTED);
	}
};

// main
int
main(int argc, const char *const *argv)
{
	// if started with parameter "broadcast", we broadcast a message and exit
	if (argc > 1 && strcmp(argv[1], "broadcast") == 0) {
		BMessage message(MSG_BROADCAST);
		status_t error = be_roster->Broadcast(&message);
		if (error != B_OK) {
			printf("Broadcast failed: %s\n", strerror(error));
			exit(1);
		}
		exit(0);
	}

	TestApp app;

	// fill the looper port
	status_t error;
	do {
		BMessage message(MSG_FILL_PORT);
		error = be_app_messenger.SendMessage(&message, (BHandler*)NULL, 0LL);
	} while (error == B_OK);

	if (error != B_WOULD_BLOCK) {
		printf("sending fill message didn't fail with B_WOULD_BLOCK: %s\n",
			strerror(error));
		exit(1);
	}

	// register looper for clipboard events
	BClipboard clipboard("message deliverer test");
	error = clipboard.StartWatching(be_app_messenger);
	if (error != B_OK) {
		printf("Failed to start clipboard watching: %s\n", strerror(error));
		exit(1);
	}

	// generate a clipboard changed modification
	if (!clipboard.Lock()) {
		printf("Failed to lock clipboard.\n");
		exit(1);
	}
	clipboard.Clear();
	clipboard.Data()->AddString("test", "test data");
	error = clipboard.Commit();
	if (error != B_OK) {
		printf("Failed to commit clipboard data: %s\n", strerror(error));
		exit(1);
	}
	clipboard.Unlock();

	// create a message runner
	BMessage message(MSG_RUNNER);
	bigtime_t interval = 100000LL;
	BMessageRunner runner(be_app_messenger, &message, interval, 3);
	error = runner.InitCheck();
	if (error != B_OK) {
		printf("Message runner initialization failed: %s\n", strerror(error));
		exit(1);
	}
	snooze(5 * interval);

	// broadcast a message: We do that be starting another instance of this
	// app with parameter "broadcast". We can't do the broadcast ourselves,
	// since the message is sent only to the other applications.

	// get our app info
	app_info appInfo;
	error = app.GetAppInfo(&appInfo);
	if (error != B_OK) {
		printf("GetAppInfo() failed: %s\n", strerror(error));
		exit(1);
	}

	// get the app path
	BPath path;
	error = path.SetTo(&appInfo.ref);
	if (error != B_OK) {
		printf("Failed to get app path: %s\n", strerror(error));
		exit(1);
	}
	
	// launch...
	BString commandLine(path.Path());
	commandLine << " broadcast";
	if (system(commandLine.String()) < 0) {
		printf("Failed re-launch app for broadcasting: %s\n", strerror(errno));
		exit(1);
	}

	// wait a bit...
	snooze(100000);

	// now start the message processing loop
	app.Run();

	return 0;
}
