/*
 * Copyright 2003-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jonas Sundström, jonas@kirilla.com
 */


#include "ZipOMatic.h"

#include <Alert.h>
#include <Roster.h>
#include <TrackerAddOnAppLaunch.h>

#include "ZipOMaticMisc.h"
#include "ZipOMaticWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "file:ZipOMatic.cpp"


int
main()
{
	ZipOMatic app;
	app.Run();

	return 0;
}


ZipOMatic::ZipOMatic()
	:
	BApplication(ZIPOMATIC_APP_SIG),
	fGotRefs(false),
	fInvoker(new BInvoker(new BMessage(ZIPPO_QUIT_OR_CONTINUE), NULL, this))
{
}


ZipOMatic::~ZipOMatic()
{
}


void
ZipOMatic::RefsReceived(BMessage* message)
{
	entry_ref ref;
	if (message->FindRef("refs", &ref) == B_OK) {
		_UseExistingOrCreateNewWindow(message);
		fGotRefs = true;
	} else if (!IsLaunching()) {
		PostMessage(B_SILENT_RELAUNCH);
	}
}


void
ZipOMatic::ReadyToRun()
{
	if (!fGotRefs)
		_UseExistingOrCreateNewWindow();
}


void
ZipOMatic::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case ZIPPO_WINDOW_QUIT:
		{
			snooze(200000);
			if (CountWindows() == 0)
				Quit();
			break;
		}
		case B_SILENT_RELAUNCH:
			_SilentRelaunch();
			break;

		case ZIPPO_QUIT_OR_CONTINUE:
		{
			int32 button;
			if (message->FindInt32("which", &button) == B_OK) {
				if (button == 0) {
					_StopZipping();
				} else {
					if (CountWindows() == 0)
						Quit();
				}
			}
			break;
		}

		default:
			BApplication::MessageReceived(message);
			break;
	}
}


bool
ZipOMatic::QuitRequested(void)
{
	if (CountWindows() <= 0)
		return true;

	BWindow* window;
	ZippoWindow* zippo;
	ZippoWindow* lastFoundZippo = NULL;
	int32 zippoCount = 0;

	for (int32 i = 0;; i++) {
		window = WindowAt(i);
		if (window == NULL)
			break;

		zippo = dynamic_cast<ZippoWindow*>(window);
		if (zippo == NULL)
			continue;

		lastFoundZippo = zippo;

		if (zippo->Lock()) {
			if (zippo->IsZipping())
				zippoCount++;
			else
				zippo->PostMessage(B_QUIT_REQUESTED);

			zippo->Unlock();
		}
	}

	if (zippoCount == 1) {
		// This is likely the most frequent case - a single zipper.
		// We post a message to the window so it can put up its own
		// BAlert instead of the app-wide BAlert. This avoids making
		// a difference between having pressed Commmand-W or Command-Q.
		// Closing or quitting, it doesn't matter for a single window.

		if (lastFoundZippo->Lock()) {
			lastFoundZippo->Activate();
			lastFoundZippo->PostMessage(B_QUIT_REQUESTED);
			lastFoundZippo->Unlock();
		}
		return false;
	}

	if (zippoCount > 0) {
		// The multi-zipper case differs from the single-zipper case
		// in that zippers are not paused while the BAlert is up.

		BString question;
		question << B_TRANSLATE("You have %ld Zip-O-Matic running.\n\n");
		question << B_TRANSLATE("Do you want to stop them?");

		BString temp;
		temp << zippoCount;
		question.ReplaceFirst("%ld", temp.String());

		BAlert* alert = new BAlert(NULL, question.String(),
			B_TRANSLATE("Stop them"), B_TRANSLATE("Let them continue"), NULL,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go(fInvoker);
		alert->Activate();
			// BAlert, being modal, does not show on the current workspace
			// if the application has no window there. Activate() triggers
			// a switch to a workspace where it does have a window.

			// TODO: See if AS_ACTIVATE_WINDOW should be handled differently
			// in src/servers/app/Desktop.cpp Desktop::ActivateWindow()
			// or if maybe BAlert should (and does not?) activate itself.

		return false;
	}

	if (CountWindows() <= 0)
		return true;

	return false;
}


void
ZipOMatic::_SilentRelaunch()
{
	_UseExistingOrCreateNewWindow();
}


void
ZipOMatic::_UseExistingOrCreateNewWindow(BMessage* message)
{
	int32 windowCount = 0;
	BWindow* bWindow;
	ZippoWindow* zWindow;
	BList list;

	while (1) {
		bWindow = WindowAt(windowCount++);
		if (bWindow == NULL)
			break;

		zWindow = dynamic_cast<ZippoWindow*>(bWindow);
		if (zWindow == NULL)
			continue;

		list.AddItem(zWindow);

		if (zWindow->Lock()) {
			if (!zWindow->IsZipping()) {
				if (message != NULL)
					zWindow->PostMessage(message);
				zWindow->SetWorkspaces(B_CURRENT_WORKSPACE);
				zWindow->Activate();
				zWindow->Unlock();
				return;
			}
			zWindow->Unlock();
		}
	}

	if (message) {
		zWindow = new ZippoWindow(list);
		zWindow->PostMessage(message);
	} else {
		zWindow = new ZippoWindow(list, true);
	}

	zWindow->Show();
}


void
ZipOMatic::_StopZipping()
{
	BWindow* window;
	ZippoWindow* zippo;
	BList list;

	for (int32 i = 0;; i++) {
		window = WindowAt(i);
		if (window == NULL)
			break;

		zippo = dynamic_cast<ZippoWindow*>(window);
		if (zippo == NULL)
			continue;

		list.AddItem(zippo);
	}

	for (int32 i = 0;; i++) {
		zippo = static_cast<ZippoWindow*>(list.ItemAt(i));
		if (zippo == NULL)
			break;

		if (zippo->Lock()) {
			if (zippo->IsZipping())
				zippo->StopZipping();

			zippo->PostMessage(B_QUIT_REQUESTED);
			zippo->Unlock();
		}
	}
}

