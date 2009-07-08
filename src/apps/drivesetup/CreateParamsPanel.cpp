/*
 * Copyright 2008-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Bryce Groff	  <bgroff@hawaii.edu>
 */

#include "CreateParamsPanel.h"
#include "Support.h"

#include <Button.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <GroupView.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <MessageFilter.h>
#include <PopUpMenu.h>
#include <Slider.h>
#include <SpaceLayoutItem.h>
#include <String.h>


class CreateParamsPanel::EscapeFilter : public BMessageFilter {
public:
	EscapeFilter(CreateParamsPanel* target)
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
 	CreateParamsPanel*		fPanel;
};

// #pragma mark -


enum {
	MSG_OK						= 'okok',
	MSG_CANCEL					= 'cncl',
	MSG_PARTITION_TYPE			= 'prty'
};


CreateParamsPanel::CreateParamsPanel(BWindow* window, off_t offset, off_t size)
	: BWindow(BRect(300.0, 200.0, 600.0, 300.0), 0, B_MODAL_WINDOW_LOOK,
		B_MODAL_SUBSET_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fEscapeFilter(new EscapeFilter(this)),
	fExitSemaphore(create_sem(0, "CreateParamsPanel exit")),
	fWindow(window),
	fReturnValue(GO_CANCELED)
{
	AddCommonFilter(fEscapeFilter);

	// Scale offset, and size from bytes to megabytes (2^20)
	// so that we do not run over a signed int32.
	offset /= kMegaByte;
	size /= kMegaByte;
	_CreateViewControls(offset, size);
}


CreateParamsPanel::~CreateParamsPanel()
{
	RemoveCommonFilter(fEscapeFilter);
	delete fEscapeFilter;

	delete_sem(fExitSemaphore);
}


bool
CreateParamsPanel::QuitRequested()
{
	release_sem(fExitSemaphore);
	return false;
}


void
CreateParamsPanel::MessageReceived(BMessage* message)
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
CreateParamsPanel::Go(off_t& offset, off_t& size, BString& type)
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
		if (BMenuItem* item = fTypeMenuField->Menu()->FindMarked()) {
			const char* _type;
			BMessage* message = item->Message();
			if (!message || message->FindString("type", &_type) < B_OK)
				_type = "BFS Filesystem";
			type << _type;
		}
		// Return the value back as bytes.
		size = (off_t)fSizeSlider->Size() * kMegaByte;
		offset = (off_t)fSizeSlider->Offset() * kMegaByte;
	}

	int32 value = fReturnValue;

	Quit();
		// NOTE: this object is toast now!

	return value;
}


void
CreateParamsPanel::Cancel()
{
	fReturnValue = GO_CANCELED;
	release_sem(fExitSemaphore);
}


void
CreateParamsPanel::_CreateViewControls(off_t offset, off_t size)
{
	// Setup the controls
	//TODO Add all the partition types that we want/can use
	fTypePopUpMenu = new BPopUpMenu("Partition Type");
	BMessage* message = new BMessage(MSG_PARTITION_TYPE);
	message->AddString("type", "BFS Filesystem");
	BMenuItem *item = new BMenuItem("BFS Filesystem", message);
	fTypePopUpMenu->AddItem(item);
	item->SetMarked(true);
	fTypeMenuField = new BMenuField("Partition Type", fTypePopUpMenu, NULL);

	fSizeSlider = new SizeSlider("Slider", "Partition Size", NULL, offset,
		offset + size);
	fSizeSlider->SetPosition(1.0);

	fOKButton = new BButton("Create", new BMessage(MSG_OK));
	fCancelButton = new BButton("Cancel", new BMessage(MSG_CANCEL));

	BView* rootView = BGroupLayoutBuilder(B_VERTICAL, 4)

		.Add(BSpaceLayoutItem::CreateVerticalStrut(10))

		// test views
		.Add(BGridLayoutBuilder(10, 10)
			// row 1
			.Add(BSpaceLayoutItem::CreateHorizontalStrut(5), 0, 0)

			.Add(fSizeSlider, 1, 0)
			.Add(BSpaceLayoutItem::CreateVerticalStrut(5), 1, 0)
			.Add(fTypeMenuField, 1, 1)
		)

		// controls
		.AddGroup(B_HORIZONTAL, 10)
			.Add(BSpaceLayoutItem::CreateHorizontalStrut(5))
			.AddGlue()
			.Add(fCancelButton)
			.Add(fOKButton)
			.Add(BSpaceLayoutItem::CreateHorizontalStrut(5))
		.End()

		.Add(BSpaceLayoutItem::CreateVerticalStrut(5))
	;

	SetLayout(new BGroupLayout(B_HORIZONTAL));
	AddChild(rootView);
	SetDefaultButton(fOKButton);

	AddToSubset(fWindow);
}
