/*
 * Copyright 2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#include "InitParamsPanel.h"

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



InitParamsPanel::InitParamsPanel(BWindow* window)
	: BWindow(BRect(300.0, 200.0, 600.0, 300.0), 0, B_MODAL_WINDOW_LOOK,
		B_MODAL_SUBSET_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS)
	, fEscapeFilter(new EscapeFilter(this))
	, fExitSemaphore(create_sem(0, "InitParamsPanel exit"))
	, fWindow(window)
	, fReturnValue(GO_CANCELED)
{
	AddCommonFilter(fEscapeFilter);

	fNameTC = new BTextControl("Name", NULL, NULL);

	BPopUpMenu* blocksizeMenu = new BPopUpMenu("Blocksize");
	BMessage* message = new BMessage(MSG_BLOCK_SIZE);
	message->AddString("size", "1024");
	blocksizeMenu->AddItem(new BMenuItem("1024 (Mostly small files)", message));
	message = new BMessage(MSG_BLOCK_SIZE);
	message->AddString("size", "2048");
	blocksizeMenu->AddItem(new BMenuItem("2048 (Recommended)", message));
	message = new BMessage(MSG_BLOCK_SIZE);
	message->AddString("size", "4096");
	blocksizeMenu->AddItem(new BMenuItem("4096", message));
	message = new BMessage(MSG_BLOCK_SIZE);
	message->AddString("size", "8192");
	blocksizeMenu->AddItem(new BMenuItem("8192 (Mostly large files)", message));

	fBlockSizeMF = new BMenuField("Blocksize", blocksizeMenu, NULL);

	BButton* okButton = new BButton("Initialize", new BMessage(MSG_OK));
	BButton* cancelButton = new BButton("Cancel", new BMessage(MSG_CANCEL));

	BView* rootView = BGroupLayoutBuilder(B_VERTICAL, 5)

		.Add(BSpaceLayoutItem::CreateVerticalStrut(10))

		// test views
		.Add(BGridLayoutBuilder(10, 10)
			// row 1
			.Add(BSpaceLayoutItem::CreateHorizontalStrut(5), 0, 0)

			.Add(fNameTC->CreateLabelLayoutItem(), 1, 0)
			.Add(fNameTC->CreateTextViewLayoutItem(), 2, 0)

			.Add(BSpaceLayoutItem::CreateHorizontalStrut(10), 3, 0)

			// row 2
			.Add(BSpaceLayoutItem::CreateHorizontalStrut(10), 0, 1)

			.Add(fBlockSizeMF->CreateLabelLayoutItem(), 1, 1)
			.Add(fBlockSizeMF->CreateMenuBarLayoutItem(), 2, 1)

			.Add(BSpaceLayoutItem::CreateHorizontalStrut(5), 3, 1)
		)

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

	fNameTC->SetText(name.String());
	fNameTC->MakeFocus(true);

	Show();
	Unlock();

	// block this thread now
	acquire_sem(fExitSemaphore);

	if (!Lock())
		return GO_CANCELED;

	if (fReturnValue == GO_SUCCESS) {
		name = fNameTC->Text();
		parameters = "";
		if (BMenuItem* item = fBlockSizeMF->Menu()->FindMarked()) {
			const char* size;
			BMessage* message = item->Message();
			if (!message || message->FindString("size", &size) < B_OK)
				size = "2048";
			// TODO: use libroot driver settings API
			parameters << "block_size " << size << ";\n"; 
		}
	}

	Quit();
	return fReturnValue;
}


void
InitParamsPanel::Cancel()
{
	fReturnValue = GO_CANCELED;
	release_sem(fExitSemaphore);
}






