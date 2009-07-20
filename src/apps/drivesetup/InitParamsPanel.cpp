/*
 * Copyright 2008-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "InitParamsPanel.h"

#include <driver_settings.h>
#include <stdio.h>

#include <Button.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <GroupView.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <MessageFilter.h>
#include <PopUpMenu.h>
#include <TextControl.h>
#include <SpaceLayoutItem.h>
#include <String.h>


class InitParamsPanel::EscapeFilter : public BMessageFilter {
public:
	EscapeFilter(InitParamsPanel* target)
		: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
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


enum {
	MSG_OK						= 'okok',
	MSG_CANCEL					= 'cncl',
	MSG_BLOCK_SIZE				= 'blsz'
};


InitParamsPanel::InitParamsPanel(BWindow* window, const BString& diskSystem,
	BPartition* partition)
	: BWindow(BRect(300.0, 200.0, 600.0, 300.0), 0, B_MODAL_WINDOW_LOOK,
		B_MODAL_SUBSET_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fEscapeFilter(new EscapeFilter(this)),
	fExitSemaphore(create_sem(0, "InitParamsPanel exit")),
	fWindow(window),
	fReturnValue(GO_CANCELED)
{
	AddCommonFilter(fEscapeFilter);

	BButton* okButton = new BButton("Initialize", new BMessage(MSG_OK));
	BButton* cancelButton = new BButton("Cancel", new BMessage(MSG_CANCEL));

	partition->GetInitializationParameterEditor(diskSystem.String(),
		&fEditor);

	BView* rootView = BGroupLayoutBuilder(B_VERTICAL, 5)
		.Add(BSpaceLayoutItem::CreateVerticalStrut(10))

		// test views
		.Add(fEditor->View())

		// controls
		.AddGroup(B_HORIZONTAL, 10)
			.Add(BSpaceLayoutItem::CreateHorizontalStrut(5))
			.AddGlue()
			.Add(cancelButton)
			.Add(okButton)
			.Add(BSpaceLayoutItem::CreateHorizontalStrut(5))
		.End()

		.Add(BSpaceLayoutItem::CreateVerticalStrut(5))
	;

	SetLayout(new BGroupLayout(B_HORIZONTAL));
	AddChild(rootView);
	SetDefaultButton(okButton);

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
	BRect frame = Frame();
	BRect parentFrame = fWindow->Frame();
	MoveTo((parentFrame.left + parentFrame.right - frame.Width()) / 2.0,
		(parentFrame.top + parentFrame.bottom - frame.Height()) / 2.0);

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
