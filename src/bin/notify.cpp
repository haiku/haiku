/*
 * Copyright 2010, Haiku, Inc. All rights reserved.
 * Copyright 2008, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Application.h>
#include <Bitmap.h>
#include <IconUtils.h>
#include <List.h>
#include <Mime.h>
#include <Notification.h>
#include <Path.h>
#include <Roster.h>
#include <TranslationUtils.h>

const char* kSignature			= "application/x-vnd.Haiku-notify";
const char* kSmallIconAttribute = "BEOS:M:STD_ICON";
const char* kLargeIconAttribute = "BEOS:L:STD_ICON";
const char* kIconAttribute		= "BEOS:ICON";

const char *kTypeNames[] = {
	"information",
	"important",
	"error",
	"progress",
	NULL
};

const int32 kErrorInitFail		= 127;
const int32 kErrorArgumentsFail	= 126;

class NotifyApp : public BApplication {
public:
								NotifyApp();
	virtual						~NotifyApp();

	virtual void				ReadyToRun();
	virtual void				ArgvReceived(int32 argc, char** argv);

			bool				HasGoodArguments() const
									{ return fHasGoodArguments; }

private:
			bool				fHasGoodArguments;
			notification_type	fType;
			char*				fAppName;
			char*				fTitle;
			char*				fMsgId;
			float				fProgress;
			bigtime_t			fTimeout;
			char*				fIconFile;
			entry_ref			fFileRef;
			char*				fMessage;
			char*				fApp;
			bool				fHasFile;
			entry_ref			fFile;
			BList*				fRefs;
			BList*				fArgv;

			void				_Usage() const;
			BBitmap*			_GetBitmap(const entry_ref* ref) const;
};


NotifyApp::NotifyApp()
	:
	BApplication(kSignature),
	fHasGoodArguments(false),
	fType(B_INFORMATION_NOTIFICATION),
	fAppName(NULL),
	fTitle(NULL),
	fMsgId(NULL),
	fProgress(0.0f),
	fTimeout(0),
	fIconFile(NULL),
	fMessage(NULL),
	fApp(NULL),
	fHasFile(false)
{
	fRefs = new BList();
	fArgv = new BList();
}


NotifyApp::~NotifyApp()
{
	free(fAppName);
	free(fTitle);
	free(fMsgId);
	free(fIconFile);
	free(fMessage);
	free(fApp);

	for (int32 i = 0; void* item = fRefs->ItemAt(i); i++)
		delete (BEntry*)item;
	delete fRefs;

	for (int32 i = 0; void* item = fArgv->ItemAt(i); i++)
		free(item);
	delete fArgv;
}


void
NotifyApp::ArgvReceived(int32 argc, char** argv)
{
	const uint32 kArgCount = argc - 1;
	uint32 index = 1;

	// Look for valid options
	for (; index <= kArgCount; ++index) {
		if (argv[index][0] == '-' && argv[index][1] == '-') {
			const char* option = argv[index] + 2;

			if (++index > kArgCount) {
				// No argument to option
				fprintf(stderr, "Missing argument to option --%s\n\n", option);
				return;
			}

			const char* argument = argv[index];

			if (strcmp(option, "type") == 0) {
				for (int32 i = 0; kTypeNames[i]; i++) {
					if (strncmp(kTypeNames[i], argument, strlen(argument)) == 0)
						fType = (notification_type)i;
				}
			} else if (strcmp(option, "app") == 0)
				fAppName = strdup(argument);
			else if (strcmp(option, "title") == 0)
				fTitle = strdup(argument);
			else if (strcmp(option, "messageID") == 0)
				fMsgId = strdup(argument);
			else if (strcmp(option, "progress") == 0)
				fProgress = atof(argument);
			else if (strcmp(option, "timeout") == 0)
				fTimeout = atol(argument) * 1000000;
			else if (strcmp(option, "icon") == 0) {
				fIconFile = strdup(argument);

				if (get_ref_for_path(fIconFile, &fFileRef) < B_OK) {
					fprintf(stderr, "Bad icon path!\n\n");
					return;
				}
			} else if (strcmp(option, "onClickApp") == 0)
				fApp = strdup(argument);
			else if (strcmp(option, "onClickFile") == 0) {
				if (get_ref_for_path(argument, &fFile) != B_OK) {
					fprintf(stderr, "Bad path for --onClickFile!\n\n");
					return;
				}

				fHasFile = true;
			} else if (strcmp(option, "onClickRef") == 0) {
				entry_ref ref;

				if (get_ref_for_path(argument, &ref) != B_OK) {
					fprintf(stderr, "Bad path for --onClickRef!\n\n");
					return;
				}

				fRefs->AddItem(new BEntry(&ref));
			} else if (strcmp(option, "onClickArgv") == 0)
				fArgv->AddItem(strdup(argument));
			else {
				// Unrecognized option
				fprintf(stderr, "Unrecognized option --%s\n\n", option);
				return;
			}
		} else {
			// Option doesn't start with '--'
			break;
		}

		if (index == kArgCount) {
			// No text argument provided, only '--' arguments
			fprintf(stderr, "Missing message argument!\n\n");
			return;
		}
	}

	// Check for missing arguments
	if (fAppName == NULL) {
		fprintf(stderr, "Missing --app argument!\n\n");
		return;
	}
	if (fTitle == NULL) {
		fprintf(stderr, "Missing --title argument!\n\n");
		return;
	}

	fMessage = strdup(argv[index]);
	fHasGoodArguments = true;
}

void
NotifyApp::_Usage() const
{
	fprintf(stderr, "Usage: notify [OPTION]... [MESSAGE]\n"
		"Send notifications to notification_server.\n"
		"  --type <type>\tNotification type,\n"
		"               \t      <type>: ");

	for (int32 i = 0; kTypeNames[i]; i++)
		fprintf(stderr, kTypeNames[i + 1] ? "%s|" : "%s\n", kTypeNames[i]);

	fprintf(stderr,
		"  --app <app name>\tApplication name\n"
		"  --title <title>\tMessage title\n"
		"  --messageID <msg id>\tMessage ID\n"
		"  --progress <float>\tProgress, value between 0.0 and 1.0  - if type is set to progress\n"
		"  --timeout <secs>\tSpecify timeout\n"
		"  --onClickApp <signature>\tApplication to open when notification is clicked\n"
		"  --onClickFile <fullpath>\tFile to open when notification is clicked\n"
		"  --onClickRef <fullpath>\tFile to open with the application when notification is clicked\n"
		"  --onClickArgv <arg>\tArgument to the application when notification is clicked\n"
		"  --icon <icon file> Icon\n");
}


BBitmap*
NotifyApp::_GetBitmap(const entry_ref* ref) const
{
	BBitmap* bitmap = NULL;

	// First try by contents
	bitmap = BTranslationUtils::GetBitmap(ref);
	if (bitmap)
		return bitmap;

	// Then, try reading its attribute
	BNode node(BPath(ref).Path());
	bitmap = new BBitmap(BRect(0, 0, (float)B_LARGE_ICON - 1,
		(float)B_LARGE_ICON - 1), B_RGBA32);
	if (BIconUtils::GetIcon(&node, kIconAttribute, kSmallIconAttribute,
		kLargeIconAttribute, B_LARGE_ICON, bitmap) != B_OK) {
		delete bitmap;
		bitmap = NULL;
	}

	return bitmap;
}


void
NotifyApp::ReadyToRun()
{
	if (HasGoodArguments()) {
		BNotification notification(fType);
		notification.SetApplication(fAppName);
		notification.SetTitle(fTitle);
		notification.SetContent(fMessage);

		if (fMsgId != NULL)
			notification.SetMessageID(fMsgId);

		if (fType == B_PROGRESS_NOTIFICATION)
			notification.SetProgress(fProgress);

		if (fIconFile != NULL) {
			BBitmap* bitmap = _GetBitmap(&fFileRef);
			if (bitmap) {
				notification.SetIcon(bitmap);
				delete bitmap;
			}
		}

		if (fApp != NULL)
			notification.SetOnClickApp(fApp);

		if (fHasFile)
			notification.SetOnClickFile(&fFile);

		for (int32 i = 0; void* item = fRefs->ItemAt(i); i++) {
			BEntry* entry = (BEntry*)item;

			entry_ref ref;
			if (entry->GetRef(&ref) == B_OK)
				notification.AddOnClickRef(&ref);
		}

		for (int32 i = 0; void* item = fArgv->ItemAt(i); i++) {
			const char* arg = (const char*)item;
			notification.AddOnClickArg(arg);
		}

		status_t ret = be_roster->Notify(notification, fTimeout);
		if (ret != B_OK) {
			fprintf(stderr, "Failed to deliver notification: %s\n",
				strerror(ret));
		}
	} else
		_Usage();

	Quit();
}


int
main(int argc, char** argv)
{
	NotifyApp app;
	if (app.InitCheck() != B_OK)
		return kErrorInitFail;

	app.Run();
	if (!app.HasGoodArguments())
		return kErrorArgumentsFail;

	return 0;
}
