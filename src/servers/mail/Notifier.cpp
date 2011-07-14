/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */

#include <Catalog.h>

#include "Notifier.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Notifier"


DefaultNotifier::DefaultNotifier(const char* accountName, bool inbound,
	ErrorLogWindow* errorWindow, MailStatusWindow* statusWindow)
	:
	fAccountName(accountName),
	fIsInbound(inbound),
	fErrorWindow(errorWindow),
	fStatusWindow(statusWindow)
{
	BString desc;
	if (fIsInbound == true)
		desc << B_TRANSLATE("Fetching mail for %name");
	else
		desc << B_TRANSLATE("Sending mail for %name");
    desc.ReplaceFirst("%name", fAccountName);

	fStatusWindow->Lock();
	fStatusView = fStatusWindow->NewStatusView(desc, fIsInbound != false);
	fStatusWindow->Unlock();
}


DefaultNotifier::~DefaultNotifier()
{
	fStatusWindow->Lock();
    if (fStatusView->Window())
		fStatusWindow->RemoveView(fStatusView);
	delete fStatusView;
	fStatusWindow->Unlock();
}


MailNotifier*
DefaultNotifier::Clone()
{
	return new DefaultNotifier(fAccountName, fIsInbound, fErrorWindow,
		fStatusWindow);
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
	fStatusView->SetTotalItems(items);
}


void
DefaultNotifier::SetTotalItemsSize(int32 size)
{
	fStatusView->SetMaximum(size);
}


void
DefaultNotifier::ReportProgress(int bytes, int messages, const char* message)
{
	if (bytes != 0)
		fStatusView->AddProgress(bytes);

	for (int i = 0; i < messages; i++)
		fStatusView->AddItem();

	if (message != NULL)
		fStatusView->SetMessage(message);

	if (fStatusView->ItemsNow() == fStatusView->CountTotalItems())
		fStatusView->Reset();
}


void
DefaultNotifier::ResetProgress(const char* message)
{
	fStatusView->Reset();
	if (message != NULL)
		fStatusView->SetMessage(message);
}
