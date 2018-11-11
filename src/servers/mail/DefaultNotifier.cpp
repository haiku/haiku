/*
 * Copyright 2011-2012, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 *
 * Distributed under the terms of the MIT License.
 */


#include "DefaultNotifier.h"

#include <Catalog.h>
#include <IconUtils.h>
#include <MailDaemon.h>
#include <Roster.h>

#include <MailPrivate.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Notifier"


DefaultNotifier::DefaultNotifier(const char* accountName, bool inbound,
	ErrorLogWindow* errorWindow, uint32 showMode)
	:
	fAccountName(accountName),
	fIsInbound(inbound),
	fErrorWindow(errorWindow),
	fNotification(B_PROGRESS_NOTIFICATION),
	fShowMode(showMode),
	fTotalItems(0),
	fItemsDone(0),
	fTotalSize(0),
	fSizeDone(0)
{
	BString desc(fIsInbound ? B_TRANSLATE("Fetching mail for %name")
		: B_TRANSLATE("Sending mail for %name"));
	desc.ReplaceFirst("%name", fAccountName);

	BString identifier;
	identifier << accountName << inbound;
		// Two windows for each acocunt : one for sending and the other for
		// receiving mails
	fNotification.SetMessageID(identifier);
	fNotification.SetGroup(B_TRANSLATE("Mail status"));
	fNotification.SetTitle(desc);

	app_info info;
	be_roster->GetAppInfo(B_MAIL_DAEMON_SIGNATURE, &info);
	BBitmap icon(BRect(0, 0, 32, 32), B_RGBA32);
	BNode node(&info.ref);
	BIconUtils::GetVectorIcon(&node, "BEOS:ICON", &icon);
	fNotification.SetIcon(&icon);
}


DefaultNotifier::~DefaultNotifier()
{
}


BMailNotifier*
DefaultNotifier::Clone()
{
	return new DefaultNotifier(fAccountName, fIsInbound, fErrorWindow,
		fShowMode);
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
DefaultNotifier::SetTotalItems(uint32 items)
{
	fTotalItems = items;
	BString progress;
	progress << fItemsDone << "/" << fTotalItems;
	fNotification.SetContent(progress);
}


void
DefaultNotifier::SetTotalItemsSize(uint64 size)
{
	fTotalSize = size;
	fNotification.SetProgress(fSizeDone / (float)fTotalSize);
}


void
DefaultNotifier::ReportProgress(uint32 messages, uint64 bytes,
	const char* message)
{
	fSizeDone += bytes;
	fItemsDone += messages;

	if (fTotalSize > 0)
		fNotification.SetProgress(fSizeDone / (float)fTotalSize);
	else if (fTotalItems > 0) {
		// No size information available
		// Report progress in terms of message count instead
		fNotification.SetProgress(fItemsDone / (float)fTotalItems);
	} else {
		// No message count information either
		// TODO: we should use a B_INFORMATION_NOTIFICATION here, but it is not
		// possible to change the BNotification type after creating it...
		fNotification.SetProgress(0);
	}

	BString progress;
	progress << message << "\t";

	if (fTotalItems > 0)
		progress << fItemsDone << "/" << fTotalItems;

	fNotification.SetContent(progress);

	int timeout = 0; // Default timeout
	if (fItemsDone >= fTotalItems && fTotalItems != 0)
		timeout = 1; // We're done, make the window go away faster

	_NotifyIfAllowed(timeout);
}


void
DefaultNotifier::ResetProgress(const char* message)
{
	fSizeDone = 0;
	fItemsDone = 0;
	fNotification.SetProgress(0);
	if (message != NULL)
		fNotification.SetTitle(message);
	_NotifyIfAllowed(1); // go away faster
}


void
DefaultNotifier::_NotifyIfAllowed(int timeout)
{
	int32 flag;
	if (fIsInbound)
		flag = B_MAIL_SHOW_STATUS_WINDOW_WHEN_ACTIVE;
	else
		flag = B_MAIL_SHOW_STATUS_WINDOW_WHEN_SENDING;

	if ((fShowMode & flag) != 0)
		fNotification.Send(timeout);
}
