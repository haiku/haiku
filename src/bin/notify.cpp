/*
 * Copyright 2010, Haiku, Inc. All rights reserved.
 * Copyright 2008, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *              Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
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

			bool				GoodArguments() const { return fOk; }

private:
			bool				fOk;
			notification_type	fType;
			const char*			fAppName;
			const char*			fTitle;
			const char*			fMsgId;
			float				fProgress;
			int32				fTimeout;
			const char*			fIconFile;
			entry_ref			fFileRef;
			const char*			fMessage;
			const char*			fApp;
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
	fOk(false),
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
	if (fAppName)
		free((void*)fAppName);
	if (fTitle)
		free((void*)fTitle);
	if (fMsgId)
		free((void*)fMsgId);
	if (fIconFile)
		free((void*)fIconFile);
	if (fMessage)
		free((void*)fMessage);
	if (fApp)
		free((void*)fApp);

	int32 i;
	void* item;

	for (i = 0; item = fRefs->ItemAt(i); i++)
		delete item;
	delete fRefs;

	for (i = 0; item = fArgv->ItemAt(i); i++) {
		if (item != NULL)
			free(item);
	}
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
			} else if (strcmp(option, "app") == 0) {
				fAppName = strdup(argument);
			} else if (strcmp(option, "title") == 0) {
				fTitle = strdup(argument);
			} else if (strcmp(option, "messageID") == 0) {
				fMsgId = strdup(argument);
			} else if (strcmp(option, "progress") == 0) {
				fProgress = atof(argument);
			} else if (strcmp(option, "timeout") == 0) {
				fTimeout = atol(argument);
			} else if (strcmp(option, "icon") == 0) {
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

				fRefs->AddItem((void*)new BEntry(&ref));
			} else if (strcmp(option, "onClickArgv") == 0)
				fArgv->AddItem((void*)strdup(argument));
			else {
				// Unrecognized option
				fprintf(stderr, "Unrecognized option --%s\n\n", option);
				return;
			}
		} else
			// Option doesn't start with '--'
			break;

		if (index == kArgCount) {
			// No text argument provided, only '--' arguments
			fprintf(stderr, "Missing message argument!\n\n");
			return;
		}
	}

	// Check for missing arguments
	if (!fAppName) {
		fprintf(stderr, "Missing --app argument!\n\n");
		return;
	}
	if (!fTitle) {
		fprintf(stderr, "Missing --title argument!\n\n");
		return;
	}

	fMessage = strdup(argv[index]);
	fOk = true;
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
	if (GoodArguments()) {
		BNotification* msg = new BNotification(fType);
		msg->SetApplication(fAppName);
		msg->SetTitle(fTitle);
		msg->SetContent(fMessage);

		if (fMsgId)
			msg->SetMessageID(fMsgId);

		if (fType == B_PROGRESS_NOTIFICATION)
			msg->SetProgress(fProgress);

		if (fIconFile) {
			BBitmap* bitmap = _GetBitmap(&fFileRef);
			if (bitmap)
				msg->SetIcon(bitmap);
		}

		if (fApp)
			msg->SetOnClickApp(fApp);

		if (fHasFile)
			msg->SetOnClickFile(&fFile);

		int32 i;
		void* item;

		for (i = 0; item = fRefs->ItemAt(i); i++) {
			BEntry* entry = (BEntry*)item;

			entry_ref ref;
			if (entry->GetRef(&ref) == B_OK)
				msg->AddOnClickRef(&ref);
		}

		for (i = 0; item = fArgv->ItemAt(i); i++) {
			const char* arg = (const char*)item;
			msg->AddOnClickArg(arg);
		}

		be_roster->Notify(msg, fTimeout);
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
	if (!app.GoodArguments())
		return kErrorArgumentsFail;

	return 0;
}
