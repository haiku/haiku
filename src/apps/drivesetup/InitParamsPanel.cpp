/*
 * Copyright 2008-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Karsten Heimrich. <host.haiku@gmx.de>
 */


#include "InitParamsPanel.h"

#include <driver_settings.h>
#include <stdio.h>

#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <DiskSystemAddOn.h>
#include <DiskSystemAddOnManager.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Locale.h>
#include <Message.h>
#include <MessageFilter.h>
#include <String.h>
#include <TextControl.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InitParamsPanel"


class InitParamsPanel::EscapeFilter : public BMessageFilter {
public:
	EscapeFilter(InitParamsPanel* target)
		:
		BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
		fPanel(target)
	{
	}

	virtual	~EscapeFilter()
	{
	}

	virtual filter_result Filter(BMessage* message, BHandler** target)
	{
		filter_result result = B_DISPATCH_MESSAGE;
		switch (message->what) {
			case B_KEY_DOWN:
			case B_UNMAPPED_KEY_DOWN: {
				uint32 key;
				if (message->FindInt32("raw_char", (int32*)&key) >= B_OK) {
					if (key == B_ESCAPE) {
						result = B_SKIP_MESSAGE;
						fPanel->Cancel();
					}
				}
				break;
			}
			default:
				break;
		}
		return result;
	}

private:
 	InitParamsPanel*		fPanel;
};


// #pragma mark -


// TODO: MSG_NAME_CHANGED is shared with the disk system add-ons, so it should
// be in some private shared header.
// TODO: there is already B_CANCEL, why not use that one?
enum {
	MSG_OK						= 'okok',
	MSG_CANCEL					= 'cncl',
	MSG_NAME_CHANGED			= 'nmch'
};


InitParamsPanel::InitParamsPanel(BWindow* window, const BString& diskSystem,
		BPartition* partition)
	:
	BWindow(BRect(300.0, 200.0, 600.0, 300.0), 0, B_MODAL_WINDOW_LOOK,
		B_MODAL_SUBSET_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fEscapeFilter(new EscapeFilter(this)),
	fExitSemaphore(create_sem(0, "InitParamsPanel exit")),
	fWindow(window),
	fReturnValue(GO_CANCELED)
{
	AddCommonFilter(fEscapeFilter);

	fOkButton = new BButton(B_TRANSLATE("Initialize"), new BMessage(MSG_OK));

	DiskSystemAddOnManager* manager = DiskSystemAddOnManager::Default();
	BDiskSystemAddOn* addOn = manager->GetAddOn(diskSystem);
	if (addOn) {
		// put the add-on
		manager->PutAddOn(addOn);

		status_t err = addOn->GetParameterEditor(B_INITIALIZE_PARAMETER_EDITOR, &fEditor);
		if (err != B_OK) {
			fEditor = NULL;
		}
	} else {
		fEditor = NULL;
	}

	// TODO: fEditor should be checked for NULL before adding.
	SetLayout(new BGroupLayout(B_HORIZONTAL));
	const float spacing = be_control_look->DefaultItemSpacing();
	AddChild(BGroupLayoutBuilder(B_VERTICAL, spacing)
		.Add(fEditor->View())
		.AddGroup(B_HORIZONTAL, spacing)
			.AddGlue()
			.Add(new BButton(B_TRANSLATE("Cancel"), new BMessage(MSG_CANCEL)))
			.Add(fOkButton)
		.End()
		.SetInsets(spacing, spacing, spacing, spacing)
	);

	SetDefaultButton(fOkButton);

	// If the partition had a previous name, set to that name.
	BString name = partition->ContentName();
	if (name.Length() > 0)
		fEditor->PartitionNameChanged(name.String());

	AddToSubset(fWindow);
}


InitParamsPanel::~InitParamsPanel()
{
	RemoveCommonFilter(fEscapeFilter);
	delete fEscapeFilter;

	delete_sem(fExitSemaphore);
}


bool
InitParamsPanel::QuitRequested()
{
	release_sem(fExitSemaphore);
	return false;
}


void
InitParamsPanel::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_CANCEL:
			Cancel();
			break;

		case MSG_OK:
			fReturnValue = GO_SUCCESS;
			release_sem(fExitSemaphore);
			break;

		case MSG_NAME_CHANGED:
			// message comes from fEditor's BTextControl
			BTextControl* control;
			if (message->FindPointer("source", (void**)&control) != B_OK)
				break;
			if (control->TextView()->TextLength() == 0
				&& fOkButton->IsEnabled())
				fOkButton->SetEnabled(false);
			else if (control->TextView()->TextLength() > 0
				&& !fOkButton->IsEnabled())
				fOkButton->SetEnabled(true);
			break;

		default:
			BWindow::MessageReceived(message);
	}
}


int32
InitParamsPanel::Go(BString& name, BString& parameters)
{
	// run the window thread, to get an initial layout of the controls
	Hide();
	Show();
	if (!Lock())
		return GO_CANCELED;

	// center the panel above the parent window
	CenterIn(fWindow->Frame());

	Show();
	Unlock();

	// block this thread now, but keep the window repainting
	while (true) {
		status_t err = acquire_sem_etc(fExitSemaphore, 1,
			B_CAN_INTERRUPT | B_RELATIVE_TIMEOUT, 50000);
		if (err != B_TIMED_OUT && err != B_INTERRUPTED)
			break;
		fWindow->UpdateIfNeeded();
	}

	if (!Lock())
		return GO_CANCELED;

	if (fEditor == NULL)
		fReturnValue = B_BAD_VALUE;

	if (fReturnValue == GO_SUCCESS) {
		if (fEditor->FinishedEditing()) {
			status_t err = fEditor->GetParameters(&parameters);
			if (err == B_OK) {
				void* handle = parse_driver_settings_string(
					parameters.String());
				if (handle != NULL) {
					const char* string = get_driver_parameter(handle, "name",
						NULL, NULL);
					name.SetTo(string);
					delete_driver_settings(handle);
				}
			} else
				fReturnValue = err;
		}
	}

	int32 value = fReturnValue;

	Quit();
		// NOTE: this object is toast now!

	return value;
}


void
InitParamsPanel::Cancel()
{
	fReturnValue = GO_CANCELED;
	release_sem(fExitSemaphore);
}
