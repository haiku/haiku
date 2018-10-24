/*
 * Copyright 2015, Dario Casalinuovo
 * Copyright 2004, 2006, Jérôme Duval.
 * Copyright 2003-2004, Andrew Bachmann.
 * Copyright 2002-2004, 2006 Marcus Overhagen.
 * Copyright 2002, Eric Jaessler.
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include <MediaDefs.h>

#include <Application.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <IconUtils.h>
#include <LaunchRoster.h>
#include <Locale.h>
#include <MediaRoster.h>
#include <Notification.h>
#include <Roster.h>

#include "MediaRosterEx.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MediaDefs"

// final & verified
const char* B_MEDIA_SERVER_SIGNATURE = "application/x-vnd.Be.media-server";
const char* B_MEDIA_ADDON_SERVER_SIGNATURE = "application/x-vnd.Be.addon-host";

const type_code B_CODEC_TYPE_INFO = 0x040807b2;

// shutdown_media_server() and launch_media_server()
// are provided by libbe.so in BeOS R5

#define MEDIA_SERVICE_NOTIFICATION_ID "MediaServiceNotificationID"


void
notify_system(float progress, const char* message)
{
	BNotification notification(B_PROGRESS_NOTIFICATION);
	notification.SetMessageID(MEDIA_SERVICE_NOTIFICATION_ID);
	notification.SetProgress(progress);
	notification.SetGroup(B_TRANSLATE("Media Service"));
	notification.SetContent(message);

	app_info info;
	be_app->GetAppInfo(&info);
	BBitmap icon(BRect(0, 0, 32, 32), B_RGBA32);
	BNode node(&info.ref);
	BIconUtils::GetVectorIcon(&node, "BEOS:ICON", &icon);
	notification.SetIcon(&icon);

	notification.Send();
}


void
progress_shutdown(int stage,
	bool (*progress)(int stage, const char* message, void* cookie),
	void* cookie)
{
	// parameter "message" is no longer used. It is kept for compatibility with
	// BeOS as this is used as a shutdown_media_server callback.

	TRACE("stage: %i\n", stage);
	const char* string = "Unknown stage";
	switch (stage) {
		case 10:
			string = B_TRANSLATE("Stopping media server" B_UTF8_ELLIPSIS);
			break;
		case 20:
			string = B_TRANSLATE("Waiting for media_server to quit.");
			break;
		case 40:
			string = B_TRANSLATE("Telling media_addon_server to quit.");
			break;
		case 50:
			string = B_TRANSLATE("Waiting for media_addon_server to quit.");
			break;
		case 70:
			string = B_TRANSLATE("Cleaning up.");
			break;
		case 100:
			string = B_TRANSLATE("Done shutting down.");
			break;
	}

	if (progress == NULL)
		notify_system(stage / 100.0f, string);
	else
		progress(stage, string, cookie);
}


status_t
shutdown_media_server(bigtime_t timeout,
	bool (*progress)(int stage, const char* message, void* cookie),
	void* cookie)
{
	BMessage msg(B_QUIT_REQUESTED);
	status_t err = B_MEDIA_SYSTEM_FAILURE;
	bool shutdown = false;

	BMediaRoster* roster = BMediaRoster::Roster(&err);
	if (roster == NULL || err != B_OK)
		return err;

	if (progress == NULL && roster->Lock()) {
		MediaRosterEx(roster)->EnableLaunchNotification(true, true);
		roster->Unlock();
	}

	if ((err = msg.AddBool("be:_user_request", true)) != B_OK)
		return err;

	team_id mediaServer = be_roster->TeamFor(B_MEDIA_SERVER_SIGNATURE);
	team_id addOnServer = be_roster->TeamFor(B_MEDIA_ADDON_SERVER_SIGNATURE);

	if (mediaServer != B_ERROR) {
		BMessage reply;
		BMessenger messenger(B_MEDIA_SERVER_SIGNATURE, mediaServer);
		progress_shutdown(10, progress, cookie);

		err = messenger.SendMessage(&msg, &reply, 2000000, 2000000);
		reply.FindBool("_shutdown", &shutdown);
		if (err == B_TIMED_OUT || shutdown == false) {
			if (messenger.IsValid())
				kill_team(mediaServer);
		} else if (err != B_OK)
			return err;

		progress_shutdown(20, progress, cookie);

		int32 rv;
		if (reply.FindInt32("error", &rv) == B_OK && rv != B_OK)
			return rv;
	}

	if (addOnServer != B_ERROR) {
		shutdown = false;
		BMessage reply;
		BMessenger messenger(B_MEDIA_ADDON_SERVER_SIGNATURE, addOnServer);
		progress_shutdown(40, progress, cookie);

		// The media_server usually shutdown the media_addon_server,
		// if not let's do something.
		if (messenger.IsValid()) {
			err = messenger.SendMessage(&msg, &reply, 2000000, 2000000);
			reply.FindBool("_shutdown", &shutdown);
			if (err == B_TIMED_OUT || shutdown == false) {
				if (messenger.IsValid())
					kill_team(addOnServer);
			} else if (err != B_OK)
				return err;

			progress_shutdown(50, progress, cookie);

			int32 rv;
			if (reply.FindInt32("error", &rv) == B_OK && rv != B_OK)
				return rv;
		}
	}

	progress_shutdown(100, progress, cookie);
	return B_OK;
}


void
progress_startup(int stage,
	bool (*progress)(int stage, const char* message, void* cookie),
	void* cookie)
{
	// parameter "message" is no longer used. It is kept for compatibility with
	// BeOS as this is used as a shutdown_media_server callback.

	TRACE("stage: %i\n", stage);
	const char* string = "Unknown stage";
	switch (stage) {
		case 10:
			string = B_TRANSLATE("Stopping media server" B_UTF8_ELLIPSIS);
			break;
		case 20:
			string = B_TRANSLATE("Stopping media_addon_server.");
			break;
		case 50:
			string = B_TRANSLATE("Starting media_services.");
			break;
		case 90:
			string = B_TRANSLATE("Error occurred starting media services.");
			break;
		case 100:
			string = B_TRANSLATE("Ready for use.");
			break;
	}

	if (progress == NULL)
		notify_system(stage / 100.0f, string);
	else
		progress(stage, string, cookie);
}


status_t
launch_media_server(bigtime_t timeout,
	bool (*progress)(int stage, const char* message, void* cookie),
	void* cookie, uint32 flags)
{
	if (BMediaRoster::IsRunning())
		return B_ALREADY_RUNNING;

	status_t err = B_MEDIA_SYSTEM_FAILURE;
	BMediaRoster* roster = BMediaRoster::Roster(&err);
	if (roster == NULL || err != B_OK)
		return err;

	if (progress == NULL && roster->Lock()) {
		MediaRosterEx(roster)->EnableLaunchNotification(true, true);
		roster->Unlock();
	}

	// The media_server crashed
	if (be_roster->IsRunning(B_MEDIA_ADDON_SERVER_SIGNATURE)) {
		progress_startup(10, progress, cookie);
		kill_team(be_roster->TeamFor(B_MEDIA_ADDON_SERVER_SIGNATURE));
	}

	// The media_addon_server crashed
	if (be_roster->IsRunning(B_MEDIA_SERVER_SIGNATURE)) {
		progress_startup(20, progress, cookie);
		kill_team(be_roster->TeamFor(B_MEDIA_SERVER_SIGNATURE));
	}

	progress_startup(50, progress, cookie);

	err = BLaunchRoster().Start(B_MEDIA_SERVER_SIGNATURE);

	if (err != B_OK)
		progress_startup(90, progress, cookie);
	else if (progress != NULL) {
		progress_startup(100, progress, cookie);
		err = B_OK;
	}

	return err;
}
