// RosterLaunchTestApp1.cpp

#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Entry.h>
#include <File.h>
#include <Message.h>
#include <Messenger.h>
#include <OS.h>
#include <Resources.h>
#include <Roster.h>
#include <String.h>

#include "RosterTestAppDefs.h"

BMessenger unitTesterMessenger;

static const char *kUnitTesterSignature
	= "application/x-vnd.obos-roster-launch-test";

// init_unit_tester_messenger
static
status_t
init_unit_tester_messenger()
{
	// Unfortunately BeOS R5 doesn't update the roster-side information about
	// the app looper's port ID once a BApplication has been created, and
	// thus we can't construct a BMessenger targeting a BApplication created
	// after the first one using the signature/team ID constructor.
	// We need to do some hacking.
	struct fake_messenger {
		port_id	fPort;
		int32	fHandlerToken;
		team_id	fTeam;
		int32	extra0;
		int32	extra1;
		bool	fPreferredTarget;
		bool	extra2;
		bool	extra3;
		bool	extra4;
	} &fake = *(fake_messenger*)&unitTesterMessenger;
	// get team
	status_t error = B_OK;
	fake.fTeam = BRoster().TeamFor(kUnitTesterSignature);
	if (fake.fTeam < 0)
		error = fake.fTeam;
	// find app looper port
	bool found = false;
	int32 cookie = 0;
	port_info info;
	while (error == B_OK && !found) {
		error = get_next_port_info(fake.fTeam, &cookie, &info);
		found = (error == B_OK && !strcmp("AppLooperPort", info.name));
	}
	// init messenger
	if (error == B_OK) {
		fake.fPort = info.port;
		fake.fHandlerToken = 0;
		fake.fPreferredTarget = true;
	}
	return error;
}

// get_app_path
status_t
get_app_path(char *buffer)
{
	status_t error = (buffer ? B_OK : B_BAD_VALUE);
	image_info info;
	int32 cookie = 0;
	bool found = false;
	if (error == B_OK) {
		while (!found && get_next_image_info(0, &cookie, &info) == B_OK) {
			if (info.type == B_APP_IMAGE) {
				strncpy(buffer, info.name, B_PATH_NAME_LENGTH);
				buffer[B_PATH_NAME_LENGTH] = 0;
				found = true;
			}
		}
	}
	if (error == B_OK && !found)
		error = B_ENTRY_NOT_FOUND;
	return error;
}

// TestApp
class TestApp : public BApplication {
public:
	TestApp(const char *signature) : BApplication(signature)
	{
	}

	virtual void ArgvReceived(int32 argc, char **argv)
	{
		BMessage message(MSG_ARGV_RECEIVED);
		message.AddInt32("argc", argc);
		for (int32 i = 0; i < argc; i++)
			message.AddString("argv", argv[i]);
		unitTesterMessenger.SendMessage(&message);
	}

	virtual void RefsReceived(BMessage *_message)
	{
		BMessage message(*_message);
		message.what = MSG_REFS_RECEIVED;
		unitTesterMessenger.SendMessage(&message);
	}

	virtual void MessageReceived(BMessage *_message)
	{
		BMessage message(MSG_MESSAGE_RECEIVED);
		message.AddMessage("message", _message);
		message.AddInt32("sender", _message->ReturnAddress().Team());
		unitTesterMessenger.SendMessage(&message);
	}

	virtual bool QuitRequested()
	{
		unitTesterMessenger.SendMessage(MSG_QUIT_REQUESTED);
		return true;
	}

	virtual void ReadyToRun()
	{
		unitTesterMessenger.SendMessage(MSG_READY_TO_RUN);
	}
};

// main
int
main(int argc, char **argv)
{
	// find app file and get signature from resources
	char path[B_PATH_NAME_LENGTH];
	status_t error = get_app_path(path);
	char signature[B_MIME_TYPE_LENGTH];
	if (error == B_OK) {
		// init app file
		BFile file;
		error = file.SetTo(path, B_READ_ONLY);
		// get signature
		BString signatureString;
		if (error == B_OK) {
			if (file.ReadAttrString("signature", &signatureString) == B_OK
				&& signatureString.Length() > 0) {
				strcpy(signature, signatureString.String());
			} else
				strcpy(signature, kDefaultTestAppSignature);
		} else
			printf("ERROR: Couldn't init app file: %s\n", strerror(error));
	} else
		printf("ERROR: Couldn't get app ref: %s\n", strerror(error));
	// create the app
	TestApp *app = NULL;
	if (error == B_OK) {
		app = new TestApp(signature);
//		unitTesterMessenger = BMessenger(kUnitTesterSignature);
		error = init_unit_tester_messenger();
		if (error != B_OK)
			printf("ERROR: Couldn't init messenger: %s\n", strerror(error));
		// send started message
		BMessage message(MSG_STARTED);
		message.AddString("path", path);
		unitTesterMessenger.SendMessage(&message);
		// send main() args message
		BMessage argsMessage(MSG_MAIN_ARGS);
		argsMessage.AddInt32("argc", argc);
		for (int i = 0; i < argc; i++)
			argsMessage.AddString("argv", argv[i]);
		unitTesterMessenger.SendMessage(&argsMessage);
		// run the app
		app->Run();
		delete app;
		// send terminated message
		unitTesterMessenger.SendMessage(MSG_TERMINATED);
	}
	return 0;
}

