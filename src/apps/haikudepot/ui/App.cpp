/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2017-2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "App.h"

#include <stdio.h>

#include <Alert.h>
#include <Catalog.h>
#include <Entry.h>
#include <Message.h>
#include <package/PackageInfo.h>
#include <Path.h>
#include <Roster.h>
#include <Screen.h>
#include <String.h>

#include "support.h"

#include "FeaturedPackagesView.h"
#include "Logger.h"
#include "MainWindow.h"
#include "ServerHelper.h"
#include "ServerSettings.h"
#include "ScreenshotWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "App"


App::App()
	:
	BApplication("application/x-vnd.Haiku-HaikuDepot"),
	fMainWindow(NULL),
	fWindowCount(0),
	fSettingsRead(false)
{
	_CheckPackageDaemonRuns();
}


App::~App()
{
	// We cannot let global destructors cleanup static BitmapRef objects,
	// since calling BBitmap destructors needs a valid BApplication still
	// around. That's why we do it here.
	PackageInfo::CleanupDefaultIcon();
	FeaturedPackagesView::CleanupIcons();
	ScreenshotWindow::CleanupIcons();
}


bool
App::QuitRequested()
{
	if (fMainWindow != NULL
		&& fMainWindow->LockLooperWithTimeout(1500000) == B_OK) {
		BMessage windowSettings;
		fMainWindow->StoreSettings(windowSettings);

		fMainWindow->UnlockLooper();

		_StoreSettings(windowSettings);
	}

	return BApplication::QuitRequested();
}


void
App::ReadyToRun()
{
	if (fWindowCount > 0)
		return;

	BMessage settings;
	_LoadSettings(settings);

	fMainWindow = new MainWindow(settings);
	_ShowWindow(fMainWindow);
}


void
App::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_MAIN_WINDOW_CLOSED:
		{
			BMessage windowSettings;
			if (message->FindMessage(KEY_WINDOW_SETTINGS,
					&windowSettings) == B_OK) {
				_StoreSettings(windowSettings);
			}

			fWindowCount--;
			if (fWindowCount == 0)
				Quit();
			break;
		}

		case MSG_CLIENT_TOO_OLD:
			ServerHelper::AlertClientTooOld(message);
			break;

		case MSG_NETWORK_TRANSPORT_ERROR:
			ServerHelper::AlertTransportError(message);
			break;

		case MSG_SERVER_ERROR:
			ServerHelper::AlertServerJsonRpcError(message);
			break;

		case MSG_ALERT_SIMPLE_ERROR:
			_AlertSimpleError(message);
			break;

		case MSG_SERVER_DATA_CHANGED:
			fMainWindow->PostMessage(message);
			break;

		default:
			BApplication::MessageReceived(message);
			break;
	}
}


void
App::RefsReceived(BMessage* message)
{
	entry_ref ref;
	int32 index = 0;
	while (message->FindRef("refs", index++, &ref) == B_OK) {
		BEntry entry(&ref, true);
		_Open(entry);
	}
}


enum arg_switch {
	UNKNOWN_SWITCH,
	NOT_SWITCH,
	HELP_SWITCH,
	WEB_APP_BASE_URL_SWITCH,
	VERBOSITY_SWITCH,
	FORCE_NO_NETWORKING_SWITCH,
	PREFER_CACHE_SWITCH,
	DROP_CACHE_SWITCH
};


static void
app_print_help()
{
	fprintf(stdout, "HaikuDepot ");
	fprintf(stdout, "[-u|--webappbaseurl <web-app-base-url>]\n");
	fprintf(stdout, "[-v|--verbosity [off|info|debug|trace]\n");
	fprintf(stdout, "[--nonetworking]\n");
	fprintf(stdout, "[--prefercache]\n");
	fprintf(stdout, "[--dropcache]\n");
	fprintf(stdout, "[-h|--help]\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "'-h' : causes this help text to be printed out.\n");
	fprintf(stdout, "'-v' : allows for the verbosity level to be set.\n");
	fprintf(stdout, "'-u' : allows for the haiku depot server url to be\n");
	fprintf(stdout, "   configured.\n");
	fprintf(stdout, "'--nonetworking' : prevents network access.\n");
	fprintf(stdout, "'--prefercache' : prefer to get data from cache rather\n");
	fprintf(stdout, "  then obtain data from the network.**\n");
	fprintf(stdout, "'--dropcache' : drop cached data before performing\n");
	fprintf(stdout, "  bulk operations.**\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "** = only applies to bulk operations.\n");
}


static arg_switch
app_resolve_switch(char *arg)
{
	int arglen = strlen(arg);

	if (arglen > 0 && arg[0] == '-') {

		if (arglen > 3 && arg[1] == '-') { // long form
			if (0 == strcmp(&arg[2], "webappbaseurl"))
				return WEB_APP_BASE_URL_SWITCH;

			if (0 == strcmp(&arg[2], "help"))
				return HELP_SWITCH;

			if (0 == strcmp(&arg[2], "verbosity"))
				return VERBOSITY_SWITCH;

			if (0 == strcmp(&arg[2], "nonetworking"))
				return FORCE_NO_NETWORKING_SWITCH;

			if (0 == strcmp(&arg[2], "prefercache"))
				return PREFER_CACHE_SWITCH;

			if (0 == strcmp(&arg[2], "dropcache"))
				return DROP_CACHE_SWITCH;
		} else {
			if (arglen == 2) { // short form
				switch (arg[1]) {
					case 'u':
						return WEB_APP_BASE_URL_SWITCH;

					case 'h':
						return HELP_SWITCH;

					case 'v':
						return VERBOSITY_SWITCH;
				}
			}
		}

		return UNKNOWN_SWITCH;
	}

	return NOT_SWITCH;
}


void
App::ArgvReceived(int32 argc, char* argv[])
{
	for (int i = 1; i < argc;) {

			// check to make sure that if there is a value for the switch,
			// that the value is in fact supplied.

		switch (app_resolve_switch(argv[i])) {
			case VERBOSITY_SWITCH:
			case WEB_APP_BASE_URL_SWITCH:
				if (i == argc-1) {
					fprintf(stdout, "unexpected end of arguments; missing "
						"value for switch [%s]\n", argv[i]);
					Quit();
					return;
				}
				break;

			default:
				break;
		}

			// now process each switch.

		switch (app_resolve_switch(argv[i])) {

			case VERBOSITY_SWITCH:
				if (!Logger::SetLevelByName(argv[i+1])) {
					fprintf(stdout, "unknown log level [%s]\n", argv[i + 1]);
					Quit();
				}
				i++; // also move past the log level value
				break;

			case HELP_SWITCH:
				app_print_help();
				Quit();
				break;

			case WEB_APP_BASE_URL_SWITCH:
				if (ServerSettings::SetBaseUrl(BUrl(argv[i + 1])) != B_OK) {
					fprintf(stdout, "malformed web app base url; %s\n",
						argv[i + 1]);
					Quit();
				}
				else {
					fprintf(stdout, "did configure the web base url; %s\n",
						argv[i + 1]);
				}

				i++; // also move past the url value

				break;

			case FORCE_NO_NETWORKING_SWITCH:
				ServerSettings::SetForceNoNetwork(true);
				break;

			case PREFER_CACHE_SWITCH:
				ServerSettings::SetPreferCache(true);
				break;

			case DROP_CACHE_SWITCH:
				ServerSettings::SetDropCache(true);
				break;

			case NOT_SWITCH:
			{
				BEntry entry(argv[i], true);
				_Open(entry);
				break;
			}

			case UNKNOWN_SWITCH:
				fprintf(stdout, "unknown switch; %s\n", argv[i]);
				Quit();
				break;
		}

		i++; // move on at least one arg
	}
}


/*! This method will display an alert based on a message.  This message arrives
    from a number of possible background threads / processes in the application.
*/

void
App::_AlertSimpleError(BMessage* message)
{
	BString alertTitle;
	BString alertText;

	if (message->FindString(KEY_ALERT_TEXT, &alertText) != B_OK)
		alertText = "?";

	if (message->FindString(KEY_ALERT_TITLE, &alertTitle) != B_OK)
		alertTitle = B_TRANSLATE("Error");

	BAlert* alert = new BAlert(alertTitle, alertText, B_TRANSLATE("OK"));

	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();
}


// #pragma mark - private


void
App::_Open(const BEntry& entry)
{
	BPath path;
	if (!entry.Exists() || entry.GetPath(&path) != B_OK) {
		fprintf(stderr, "Package file not found: %s\n", path.Path());
		return;
	}

	// Try to parse package file via Package Kit
	BPackageKit::BPackageInfo info;
	status_t status = info.ReadFromPackageFile(path.Path());
	if (status != B_OK) {
		fprintf(stderr, "Failed to parse package file: %s\n",
			strerror(status));
		return;
	}

	// Transfer information into PackageInfo
	PackageInfoRef package(new(std::nothrow) PackageInfo(info), true);
	if (package.Get() == NULL) {
		fprintf(stderr, "Could not allocate PackageInfo\n");
		return;
	}

	package->SetLocalFilePath(path.Path());

	BMessage settings;
	_LoadSettings(settings);

	MainWindow* window = new MainWindow(settings, package);
	_ShowWindow(window);
}


void
App::_ShowWindow(MainWindow* window)
{
	window->Show();
	fWindowCount++;
}


bool
App::_LoadSettings(BMessage& settings)
{
	if (!fSettingsRead) {
		fSettingsRead = true;
		if (load_settings(&fSettings, KEY_MAIN_SETTINGS, "HaikuDepot") != B_OK)
			fSettings.MakeEmpty();
	}
	settings = fSettings;
	return !fSettings.IsEmpty();
}


void
App::_StoreSettings(const BMessage& settings)
{
	// Take what is in settings and replace data under the same name in
	// fSettings, leaving anything in fSettings that is not contained in
	// settings.
	int32 i = 0;

	char* name;
	type_code type;
	int32 count;

	while (settings.GetInfo(B_ANY_TYPE, i++, &name, &type, &count) == B_OK) {
		fSettings.RemoveName(name);
		for (int32 j = 0; j < count; j++) {
			const void* data;
			ssize_t size;
			if (settings.FindData(name, type, j, &data, &size) != B_OK)
				break;
			fSettings.AddData(name, type, data, size);
		}
	}

	save_settings(&fSettings, KEY_MAIN_SETTINGS, "HaikuDepot");
}


// #pragma mark -


static const char* kPackageDaemonSignature
	= "application/x-vnd.haiku-package_daemon";

void
App::_CheckPackageDaemonRuns()
{
	while (!be_roster->IsRunning(kPackageDaemonSignature)) {
		BAlert* alert = new BAlert(
			B_TRANSLATE("Start package daemon"),
			B_TRANSLATE("HaikuDepot needs the package daemon to function, "
				"and it appears to be not running.\n"
				"Would you like to start it now?"),
			B_TRANSLATE("No, quit HaikuDepot"),
			B_TRANSLATE("Start package daemon"), NULL, B_WIDTH_AS_USUAL,
			B_WARNING_ALERT);
		alert->SetShortcut(0, B_ESCAPE);

		if (alert->Go() == 0)
			exit(1);

		if (!_LaunchPackageDaemon())
			break;
	}
}


bool
App::_LaunchPackageDaemon()
{
	status_t ret = be_roster->Launch(kPackageDaemonSignature);
	if (ret != B_OK) {
		BString errorMessage
			= B_TRANSLATE("Starting the package daemon failed:\n\n%Error%");
		errorMessage.ReplaceAll("%Error%", strerror(ret));

		BAlert* alert = new BAlert(
			B_TRANSLATE("Package daemon problem"), errorMessage,
			B_TRANSLATE("Quit HaikuDepot"),
			B_TRANSLATE("Try again"), NULL, B_WIDTH_AS_USUAL,
			B_WARNING_ALERT);
		alert->SetShortcut(0, B_ESCAPE);

		if (alert->Go() == 0)
			return false;
	}
	// TODO: Would be nice to send a message to the package daemon instead
	// and get a reply once it is ready.
	snooze(2000000);
	return true;
}

