/*
 * Copyright 2008-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de.
 *		Karsten Heimrich. <host.haiku@gmx.de>
 */


#include "AbstractParametersPanel.h"

#include <driver_settings.h>
#include <stdio.h>

#include <Button.h>
#include <Catalog.h>
#include <DiskSystemAddOn.h>
#include <DiskSystemAddOnManager.h>
#include <GroupLayout.h>
#include <MessageFilter.h>
#include <String.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AbstractParametersPanel"


static const uint32 kMsgOk = 'okok';
static const uint32 kParameterChanged = 'pmch';


class AbstractParametersPanel::EscapeFilter : public BMessageFilter {
public:
	EscapeFilter(AbstractParametersPanel* target)
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
 	AbstractParametersPanel*	fPanel;
};


// #pragma mark -


AbstractParametersPanel::AbstractParametersPanel(BWindow* window)
	:
	BWindow(BRect(300.0, 200.0, 600.0, 300.0), 0, B_MODAL_WINDOW_LOOK,
		B_MODAL_SUBSET_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fOkButton(new BButton(B_TRANSLATE("OK"), new BMessage(kMsgOk))),
	fReturnStatus(B_CANCELED),
	fEditor(NULL),
	fEscapeFilter(new EscapeFilter(this)),
	fExitSemaphore(create_sem(0, "AbstractParametersPanel exit")),
	fWindow(window)
{
	AddCommonFilter(fEscapeFilter);
	AddToSubset(fWindow);
}


AbstractParametersPanel::~AbstractParametersPanel()
{
	RemoveCommonFilter(fEscapeFilter);
	delete fEscapeFilter;

	delete_sem(fExitSemaphore);

	if (fEditor == NULL)
		delete fOkButton;
}


bool
AbstractParametersPanel::QuitRequested()
{
	release_sem(fExitSemaphore);
	return false;
}


void
AbstractParametersPanel::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_CANCEL:
			Cancel();
			break;

		case kMsgOk:
			fReturnStatus = B_OK;
			release_sem(fExitSemaphore);
			break;

		case kParameterChanged:
			fOkButton->SetEnabled(fEditor->ValidateParameters());
			break;

		default:
			BWindow::MessageReceived(message);
	}
}


status_t
AbstractParametersPanel::Go(BString& parameters)
{
	BMessage storage;
	return Go(parameters, storage);
}


void
AbstractParametersPanel::Cancel()
{
	fReturnStatus = B_CANCELED;
	release_sem(fExitSemaphore);
}


void
AbstractParametersPanel::Init(B_PARAMETER_EDITOR_TYPE type,
	const BString& diskSystem, BPartition* partition)
{
	// Create partition parameter editor

	status_t status = B_ERROR;
	if (diskSystem.IsEmpty()) {
		status = partition->GetParameterEditor(type, &fEditor);
	} else {
		DiskSystemAddOnManager* manager = DiskSystemAddOnManager::Default();
		BDiskSystemAddOn* addOn = manager->GetAddOn(diskSystem);
		if (addOn != NULL) {
			// put the add-on
			manager->PutAddOn(addOn);

			status = addOn->GetParameterEditor(type, &fEditor);
		}
	}
	if (status != B_OK && status != B_NOT_SUPPORTED)
		fReturnStatus = status;

	if (fEditor == NULL)
		return;

	// Create controls

	BLayoutBuilder::Group<> builder = BLayoutBuilder::Group<>(this,
		B_VERTICAL);
	AddControls(builder, fEditor->View());

	builder.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.AddGlue()
			.Add(new BButton(B_TRANSLATE("Cancel"), new BMessage(B_CANCEL)))
			.Add(fOkButton)
		.End()
		.SetInsets(B_USE_DEFAULT_SPACING);

	SetDefaultButton(fOkButton);

	fEditor->SetTo(partition);
	fEditor->SetModificationMessage(new BMessage(kParameterChanged));
}


status_t
AbstractParametersPanel::Go(BString& parameters, BMessage& storage)
{
	// Without an editor, we cannot change anything, anyway
	if (fEditor == NULL && NeedsEditor()) {
		parameters = "";
		if (ValidWithoutEditor() && fReturnStatus == B_CANCELED)
			fReturnStatus = B_OK;

		if (!Lock())
			return B_ERROR;
	} else {
		// run the window thread, to get an initial layout of the controls
		Hide();
		Show();
		if (!Lock())
			return B_CANCELED;

		// center the panel above the parent window
		CenterIn(fWindow->Frame());

		Show();
		Unlock();

		// block this thread now, but keep the window repainting
		while (true) {
			status_t status = acquire_sem_etc(fExitSemaphore, 1,
				B_CAN_INTERRUPT | B_RELATIVE_TIMEOUT, 50000);
			if (status != B_TIMED_OUT && status != B_INTERRUPTED)
				break;
			fWindow->UpdateIfNeeded();
		}

		if (!Lock())
			return B_CANCELED;

		if (fReturnStatus == B_OK) {
			if (fEditor != NULL && fEditor->ValidateParameters()) {
				status_t status = fEditor->GetParameters(parameters);
				if (status != B_OK)
					fReturnStatus = status;
			}
			if (fReturnStatus == B_OK)
				fReturnStatus = ParametersReceived(parameters, storage);
		}
	}

	status_t status = fReturnStatus;

	Quit();
		// NOTE: this object is toast now!

	return status;
}


bool
AbstractParametersPanel::NeedsEditor() const
{
	return true;
}


bool
AbstractParametersPanel::ValidWithoutEditor() const
{
	return true;
}

status_t
AbstractParametersPanel::ParametersReceived(const BString& parameters,
	BMessage& storage)
{
	return B_OK;
}


void
AbstractParametersPanel::AddControls(BLayoutBuilder::Group<>& builder,
	BView* editorView)
{
	builder.Add(editorView);
}
