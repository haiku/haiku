/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT license.
 */

#include "HyperTextActions.h"

#include <Entry.h>
#include <Message.h>
#include <Roster.h>


// #pragma mark - URLAction


URLAction::URLAction(const BString& url)
	:
	fURL(url)
{
}


URLAction::~URLAction()
{
}


void
URLAction::Clicked(HyperTextView* view, BPoint where, BMessage* message)
{
	// be lazy and let /bin/open open the URL
	entry_ref ref;
	if (get_ref_for_path("/bin/open", &ref))
		return;

	const char* args[] = { fURL.String(), NULL };
	be_roster->Launch(&ref, 1, args);
}


// #pragma mark - OpenFileAction


OpenFileAction::OpenFileAction(const BString& file)
	:
	fFile(file)
{
}


OpenFileAction::~OpenFileAction()
{
}


void
OpenFileAction::Clicked(HyperTextView* view, BPoint where, BMessage* message)
{
	// get the entry ref and let Tracker open the file
	entry_ref ref;
	if (get_ref_for_path(fFile.String(), &ref) != B_OK
		|| !BEntry(&ref).Exists()) {
		return;
	}

	BMessenger tracker("application/x-vnd.Be-TRAK");
	if (tracker.IsValid()) {
		BMessage message(B_REFS_RECEIVED);
		message.AddRef("refs", &ref);
		tracker.SendMessage(&message);
	} else
		be_roster->Launch(&ref);
}
