/*
 * Copyright 2004, pinc Software. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */


#include "NetworkTime.h"
#include "UpdateWindow.h"
#include "EditServerListWindow.h"
#include "ntp.h"

#include <Application.h>
#include <Path.h>
#include <File.h>
#include <FindDirectory.h>
#include <Alert.h>
#include <TextView.h>

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>


status_t update_time(const BMessage &settings, Monitor *monitor);
status_t update_time(const BMessage &server, Monitor *monitor, thread_id *_asyncThread);


static const uint32 kMsgNetworkTimeSettings = 'NtpS';

extern const char *__progname;
static const char *sProgramName = __progname;


class Settings {
	public:
		Settings();
		~Settings();

		void CopyMessage(BMessage &message) const;
		const BMessage &Message() const;
		void UpdateFrom(BMessage &message);

		void ResetServersToDefaults();
		void ResetToDefaults();
		status_t Load();
		status_t Save();

	private:
		status_t GetPath(BPath &path);

		BMessage	fMessage;
		bool		fWasUpdated;
};


class PrintMonitor : public Monitor {
	private:
		virtual void
		update(float progress, const char *text, va_list args)
		{
			if (progress != -1.f)
				printf("%3g%% ", progress);
			else
				printf("     ");

			if (text != NULL)
				vprintf(text, args);

			putchar('\n');
		}
};


class MessengerMonitor : public Monitor {
	public:
		MessengerMonitor(BMessenger messenger)
			:
			fMessenger(messenger)
		{
		}

		virtual void
		Done(status_t status)
		{
			BMessage message(kMsgStatusUpdate);
			message.AddInt32("status", status);

			fMessenger.SendMessage(&message);
			fMessenger.SendMessage(kMsgStopTimeUpdate);

			// This deadlocks Dano
			//be_app_messenger.SendMessage(kMsgStopTimeUpdate);
		}

	private:
		virtual void
		update(float progress, const char *text, va_list args)
		{
			BMessage message(kMsgStatusUpdate);
			if (progress != -1.f)
				message.AddFloat("progress", progress);

			if (text != NULL) {
				char buffer[2048];
				vsprintf(buffer, text, args);
				message.AddString("message", buffer);
			}

			fMessenger.SendMessage(&message);
		}

		BMessenger fMessenger;
};


class NetworkTimeApplication : public BApplication {
	public:
		NetworkTimeApplication(Settings &settings);
		virtual ~NetworkTimeApplication();

		virtual void ReadyToRun();
		virtual void MessageReceived(BMessage *message);
		virtual void AboutRequested();

		void BroadcastSettingsChanges(BMessage &message);

	private:
		Settings	&fSettings;
		thread_id	fUpdateThread;
		BWindow		*fServerListWindow;
};


status_t
adopt_int32(const BMessage &from, BMessage &to, const char *name, bool &updated)
{
	int32 value;
	if (from.FindInt32(name, &value) != B_OK)
		return B_ENTRY_NOT_FOUND;

	int32 original;
	if (to.FindInt32(name, &original) == B_OK) {
		if (value == original)
			return B_OK;

		updated = true;
		return to.ReplaceInt32(name, value);
	}

	updated = true;
	return to.AddInt32(name, value);
}


status_t
adopt_bool(const BMessage &from, BMessage &to, const char *name, bool &updated)
{
	bool value;
	if (from.FindBool(name, &value) != B_OK)
		return B_ENTRY_NOT_FOUND;

	bool original;
	if (to.FindBool(name, &original) == B_OK) {
		if (value == original)
			return B_OK;

		updated = true;
		return to.ReplaceBool(name, value);
	}

	updated = true;
	return to.AddBool(name, value);
}


status_t
adopt_rect(const BMessage &from, BMessage &to, const char *name, bool &updated)
{
	BRect rect;
	if (from.FindRect(name, &rect) != B_OK)
		return B_ENTRY_NOT_FOUND;

	BRect original;
	if (to.FindRect(name, &original) == B_OK) {
		if (rect == original)
			return B_OK;

		updated = true;
		return to.ReplaceRect(name, rect);
	}

	updated = true;
	return to.AddRect(name, rect);
}


//	#pragma mark -


class UpdateLooper : public BLooper {
	public:
		UpdateLooper(const BMessage &settings, Monitor *monitor);

		void MessageReceived(BMessage *message);

	private:
		const BMessage	&fSettings;
		Monitor			*fMonitor;
};


UpdateLooper::UpdateLooper(const BMessage &settings, Monitor *monitor)
	: BLooper("update looper"),
	fSettings(settings),
	fMonitor(monitor)
{
	PostMessage(kMsgUpdateTime);
}


void
UpdateLooper::MessageReceived(BMessage *message)
{
	if (message->what != kMsgUpdateTime)
		return;

	update_time(fSettings, fMonitor);
}


//	#pragma mark -


NetworkTimeApplication::NetworkTimeApplication(Settings &settings)
	: BApplication("application/x-vnd.Haiku-NetworkTime"),
	fSettings(settings),
	fUpdateThread(-1),
	fServerListWindow(NULL)
{
}


NetworkTimeApplication::~NetworkTimeApplication()
{
}


void
NetworkTimeApplication::ReadyToRun()
{
	BRect rect;
	if (fSettings.Message().FindRect("status frame", &rect) != B_OK)
		rect.Set(0, 0, 300, 150);

	UpdateWindow *window = new UpdateWindow(rect, fSettings.Message());
	window->Show();
}


void
NetworkTimeApplication::BroadcastSettingsChanges(BMessage &message)
{
	BWindow *window;
	int32 index = 0;
	while ((window = WindowAt(index++)) != NULL)
		window->PostMessage(&message);
}


void
NetworkTimeApplication::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgUpdateSettings:
		{
			fSettings.UpdateFrom(*message);

			message->what = kMsgSettingsUpdated;
			BroadcastSettingsChanges(*message);
			break;
		}

		case kMsgResetServerList:
		{
			fSettings.ResetServersToDefaults();

			BMessage updated = fSettings.Message();
			updated.what = kMsgSettingsUpdated;

			BroadcastSettingsChanges(updated);
			break;
		}

		case kMsgStopTimeUpdate:
			if (fUpdateThread >= B_OK)
				kill_thread(fUpdateThread);

			fUpdateThread = -1;
			break;

		case kMsgUpdateTime:
		{
			if (fUpdateThread >= B_OK)
				break;

			MessengerMonitor *monitor = NULL;
			BMessenger messenger;
			if (message->FindMessenger("monitor", &messenger) == B_OK)
				monitor = new MessengerMonitor(messenger);

			update_time(fSettings.Message(), monitor, &fUpdateThread);
			break;
		}

		case kMsgEditServerList:
			if (fServerListWindow == NULL) {
				BRect rect;
				if (fSettings.Message().FindRect("edit servers frame", &rect) != B_OK)
					rect.Set(0, 0, 250, 250);
				fServerListWindow = new EditServerListWindow(rect, fSettings.Message());
			}

			fServerListWindow->Show();
			break;

		case kMsgEditServerListWindowClosed:
			fServerListWindow = NULL;
			break;
	}
}


void
NetworkTimeApplication::AboutRequested()
{
	BAlert *alert = new BAlert("about", "Network Time\n"
		"\twritten by Axel Dörfler\n\n"
		"\tCopyright " B_UTF8_COPYRIGHT "2004, pinc Software.\n"
		"\tAll Rights Reserved.\n", "Ok");
	BTextView *view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetSize(18);
	font.SetFace(B_BOLD_FACE); 			
	view->SetFontAndColor(0, 12, &font);

	alert->Go();
}


//	#pragma mark -


Settings::Settings()
	:
	fMessage(kMsgNetworkTimeSettings),
	fWasUpdated(false)
{
	ResetToDefaults();
	Load();
}


Settings::~Settings()
{
	if (fWasUpdated)
		Save();
}


void
Settings::CopyMessage(BMessage &message) const
{
	message = fMessage;
}


const BMessage &
Settings::Message() const
{
	return fMessage;
}


void
Settings::UpdateFrom(BMessage &message)
{
	if (message.HasBool("reset servers")) {
		fMessage.RemoveName("server");
		fWasUpdated = true;
	}

	if (message.HasString("server")) {
		// remove old servers
		fMessage.RemoveName("server");

		const char *server;
		int32 index = 0;
		while (message.FindString("server", index++, &server) == B_OK) {
			fMessage.AddString("server", server);
		}

		fWasUpdated = true;
	}

	adopt_int32(message, fMessage, "default server", fWasUpdated);

	adopt_bool(message, fMessage, "auto mode", fWasUpdated);
	adopt_bool(message, fMessage, "knows auto mode", fWasUpdated);
	adopt_bool(message, fMessage, "try all servers", fWasUpdated);
	adopt_bool(message, fMessage, "choose default server", fWasUpdated);

	adopt_rect(message, fMessage, "status frame", fWasUpdated);
	adopt_rect(message, fMessage, "edit servers frame", fWasUpdated);
}


void
Settings::ResetServersToDefaults()
{
	fMessage.RemoveName("server");

	fMessage.AddString("server", "pool.ntp.org");
	fMessage.AddString("server", "de.pool.ntp.org");
	fMessage.AddString("server", "time.nist.gov");

	if (fMessage.ReplaceInt32("default server", 0) != B_OK)
		fMessage.AddInt32("default server", 0);
}


void
Settings::ResetToDefaults()
{
	fMessage.MakeEmpty();
	ResetServersToDefaults();

	fMessage.AddBool("knows auto mode", false);
	fMessage.AddBool("auto mode", false);
	fMessage.AddBool("try all servers", true);
	fMessage.AddBool("choose default server", true);

	fMessage.AddRect("status frame", BRect(0, 0, 300, 150));
	fMessage.AddRect("edit servers frame", BRect(0, 0, 250, 250));
}


status_t
Settings::Load()
{
	status_t status;

	BPath path;
	if ((status = GetPath(path)) != B_OK)
		return status;

	BFile file(path.Path(), B_READ_ONLY);
	if ((status = file.InitCheck()) != B_OK)
		return status;

	BMessage load;
	if ((status = load.Unflatten(&file)) != B_OK)
		return status;

	if (load.what != kMsgNetworkTimeSettings)
		return B_BAD_TYPE;

	fMessage = load;
	return B_OK;
}


status_t
Settings::Save()
{
	status_t status;

	BPath path;
	if ((status = GetPath(path)) != B_OK)
		return status;

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if ((status = file.InitCheck()) != B_OK)
		return status;

	file.SetSize(0);

	return fMessage.Flatten(&file);
}


status_t
Settings::GetPath(BPath &path)
{
	status_t status;
	if ((status = find_directory(B_USER_SETTINGS_DIRECTORY, &path)) != B_OK)
		return status;

	path.Append("pinc.networktime settings");
	return B_OK;
}


//	#pragma mark -


status_t
update_time(const BMessage &settings, Monitor *monitor)
{
	int32 defaultServer;
	if (settings.FindInt32("default server", &defaultServer) != B_OK)
		defaultServer = 0;

	status_t status = B_ENTRY_NOT_FOUND;
	const char *server;
	if (settings.FindString("server", defaultServer, &server) == B_OK)
		status = ntp_update_time(server, monitor);

	// try other servers if we should
	if (status != B_OK && settings.FindBool("try all servers")) {
		for (int32 index = 0; ; index++) {
			if (index == defaultServer)
				index++;
			if (settings.FindString("server", index, &server) != B_OK)
				break;

			status = ntp_update_time(server, monitor);
			if (status == B_OK) {
				if (be_app != NULL && settings.FindBool("choose default server")) {
					BMessage update(kMsgUpdateSettings);
					update.AddInt32("default server", index);
					be_app->PostMessage(&update);
				}
				break;
			}
		}
	}

	if (monitor != NULL)
		monitor->Done(status);

	delete monitor;
	return status;
}


status_t
update_time(const BMessage &settings, Monitor *monitor, thread_id *_asyncThread)
{
	if (_asyncThread != NULL) {
		BLooper *looper = new UpdateLooper(settings, monitor);
		*_asyncThread = looper->Run();
		return B_OK;
	}

	return update_time(settings, monitor);
}


bool
parse_time(char *arg, int32 *_seconds)
{
	char *unit;

	if (isdigit(arg[0]))
		*_seconds = strtoul(arg, &unit, 10);
	else
		return false;

	if (unit[0] == '\0' || !strcmp(unit, "m")) {
		*_seconds *= 60;
		return true;
	}
	if (!strcmp(unit, "h")) {
		*_seconds *= 60 * 60;
		return true;
	}
	if (!strcmp(unit, "s"))
		return true;

	return false;
}


void
usage(void)
{
	fprintf(stderr, "usage: %s [-s|--server <host>] [-p|--periodic <interval>] [-n|--silent] [-g|--nogui]\n"
		"  -s, --server\t\tContact \"host\" instead of saved list\n"
		"  -p, --periodic\tPeriodically query for the time, interval specified in minutes\n"
		"  -n, --silent\t\tDo not print progress\n"
		"  -g, --nogui\t\tDo not show graphical user interface (implied by other options)\n",
		sProgramName);
	exit(0);
}


int
main(int argc, char **argv)
{
	Settings settings;

	if (argc > 1) {
		// when there are any command line options, we will switch to command line mode

		BMessage message;
		settings.CopyMessage(message);
			// we may want to override the global settings with command line options

		static struct option const longopts[] = {
			{"server", required_argument, NULL, 's'},
			{"periodic", required_argument, NULL, 'p'},
			{"silent", no_argument, NULL, 'n'},
			{"nogui", no_argument, NULL, 'g'},
			{"help", no_argument, NULL, 'h'},
			{NULL, 0, NULL, 0}
		};

		int32 interval = 0;
		bool silent = false;

		char option;
		while ((option = getopt_long(argc, argv, "s:p:h", longopts, NULL)) != -1) {
			switch (option) {
				case 's':
					message.RemoveName("server");
					message.AddString("server", optarg);
					message.ReplaceInt32("default server", 0);
					break;
				case 'n':
					silent = true;
					break;
				case 'g':
					// default in command line mode
					break;
				case 'p':
					if (parse_time(optarg, &interval))
						break;
					// supposed to fall through
				case 'h':
				default:
					usage();
			}
		}

		while (true) {
			Monitor *monitor = NULL;
			if (!silent)
				monitor = new PrintMonitor();

			update_time(message, monitor, NULL);
			
			if (interval == 0)
				break;

			snooze(interval * 1000000LL);
		}
	} else {
		NetworkTimeApplication app(settings);
		app.Run();
	}

	return 0;
}
