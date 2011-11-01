/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */

#include <Catalog.h>
#include <Roster.h>

#include "Notifier.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Notifier"


DefaultNotifier::DefaultNotifier(const char* accountName, bool inbound,
	ErrorLogWindow* errorWindow)
	:
	fAccountName(accountName),
	fIsInbound(inbound),
	fErrorWindow(errorWindow),
	fNotification(B_PROGRESS_NOTIFICATION),
	fTotalItems(0),
	fItemsDone(0),
	fTotalSize(0),
	fSizeDone(0)
{
	BString desc;
	if (fIsInbound == true)
		desc << B_TRANSLATE("Fetching mail for %name");
	else
		desc << B_TRANSLATE("Sending mail for %name");
    desc.ReplaceFirst("%name", fAccountName);

	BString identifier;
	identifier << (int)this;
		// This should get us an unique value for each notifier running
	fNotification.SetMessageID(identifier);
	fNotification.SetApplication("Mail daemon");
}


DefaultNotifier::~DefaultNotifier()
{
}


MailNotifier*
DefaultNotifier::Clone()
{
	return new DefaultNotifier(fAccountName, fIsInbound, fErrorWindow);
}


void
DefaultNotifier::ShowError(const char* error)
{
	fErrorWindow->AddError(B_WARNING_ALERT, error, fAccountName);
}


void
DefaultNotifier::ShowMessage(const char* message)
{
	fErrorWindow->AddError(B_INFO_ALERT, message, fAccountName);
}


void
DefaultNotifier::SetTotalItems(int32 items)
{
	fTotalItems = items;
	BString progress;
	progress << fItemsDone << "/" << fTotalItems;
	fNotification.SetContent(progress);
}


void
DefaultNotifier::SetTotalItemsSize(int32 size)
{
	fTotalSize = size;
	fNotification.SetProgress(fSizeDone / (float)fTotalSize);
}


void
DefaultNotifier::ReportProgress(int bytes, int messages, const char* message)
{
	fSizeDone += bytes;
	if (fTotalSize > 0)
		fNotification.SetProgress(fSizeDone / (float)fTotalSize);
	else {
		// Likely we should set it as an INFORMATION_NOTIFICATION in that case,
		// but this can't be done after object creation...
		fNotification.SetProgress(0);
	}

	fItemsDone += messages;
	BString progress;
	if (fTotalItems > 0)
		progress << fItemsDone << "/" << fTotalItems;

	fNotification.SetContent(progress);

	if (message != NULL)
		fNotification.SetTitle(message);

	int timeout = 0; // Default timeout
	if (fItemsDone == fTotalItems && fTotalItems != 0)
		timeout = 1; // We're done, make the window go away faster
	be_roster->Notify(fNotification, timeout);
}


void
DefaultNotifier::ResetProgress(const char* message)
{
	fNotification.SetProgress(0);
	if (message != NULL)
		fNotification.SetContent(message);
	be_roster->Notify(fNotification, 0);
}
