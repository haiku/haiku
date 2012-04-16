/*
 * Copyright 2008-20011 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Bryce Groff	  <bgroff@hawaii.edu>
 *		Karsten Heimrich. <host.haiku@gmx.de>
 */

#include "CreateParamsPanel.h"
#include "Support.h"

#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <DiskDeviceTypes.h>
#include <GridLayoutBuilder.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Locale.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <MessageFilter.h>
#include <PopUpMenu.h>
#include <PartitionParameterEditor.h>
#include <Partition.h>
#include <String.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "CreateParamsPanel"


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
	MSG_PARTITION_TYPE			= 'type',
	MSG_SIZE_SLIDER				= 'ssld',
	MSG_SIZE_TEXTCONTROL		= 'stct'
};


CreateParamsPanel::CreateParamsPanel(BWindow* window, BPartition* partition,
	off_t offset, off_t size)
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
	_CreateViewControls(partition, offset, size);
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

		case MSG_PARTITION_TYPE:
			if (fEditor != NULL) {
				const char* type;
				message->FindString("type", &type);
				fEditor->PartitionTypeChanged(type);
			}
			break;
		
		case MSG_SIZE_SLIDER:
			_UpdateTextControl();
			break;
			
		case MSG_SIZE_TEXTCONTROL:
		{
			BString sizeString;
			sizeString = fSizeTextControl->Text();
			int32 sizeInt = atoi(sizeString.String());
			if (sizeInt >= 0 && sizeInt <= fSizeSlider->MaxPartitionSize())
				fSizeSlider->SetValue(sizeInt);
			else
				_UpdateTextControl();
			break;
		}

		default:
			BWindow::MessageReceived(message);
	}
}


int32
CreateParamsPanel::Go(off_t& offset, off_t& size, BString& name,
	BString& type, BString& parameters)
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

	if (fReturnValue == GO_SUCCESS) {
		// Return the value back as bytes.
		size = (off_t)fSizeSlider->Size() * kMegaByte;
		offset = (off_t)fSizeSlider->Offset() * kMegaByte;

		// get name
		name.SetTo(fNameTextControl->Text());

		// get type
		if (BMenuItem* item = fTypeMenuField->Menu()->FindMarked()) {
			const char* _type;
			BMessage* message = item->Message();
			if (!message || message->FindString("type", &_type) < B_OK)
				_type = kPartitionTypeBFS;
			type << _type;
		}

		// get editors parameters
		if (fEditor != NULL) {
			if (fEditor->FinishedEditing()) {
				status_t status = fEditor->GetParameters(&parameters);
				if (status != B_OK)
					fReturnValue = status;
			}
		}
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
CreateParamsPanel::_CreateViewControls(BPartition* parent, off_t offset,
	off_t size)
{
	// Setup the controls
	fSizeSlider = new SizeSlider("Slider", B_TRANSLATE("Partition size"), NULL,
		offset, offset + size);
	fSizeSlider->SetPosition(1.0);
	fSizeSlider->SetModificationMessage(new BMessage(MSG_SIZE_SLIDER));
	
	BString sizeText;
	sizeText << fSizeSlider->Value();
	fSizeTextControl = new BTextControl("Size Control",
		"", sizeText.String(), NULL);
	for(int32 i = 0; i < 256; i++)
		fSizeTextControl->TextView()->DisallowChar(i);
	for(int32 i = '0'; i <= '9'; i++)
		fSizeTextControl->TextView()->AllowChar(i);
	fSizeTextControl->SetModificationMessage(new BMessage(MSG_SIZE_TEXTCONTROL));

	fNameTextControl = new BTextControl("Name Control",
		B_TRANSLATE("Partition name:"),	"", NULL);
	if (!parent->SupportsChildName())
		fNameTextControl->SetEnabled(false);

	fTypePopUpMenu = new BPopUpMenu("Partition Type");

	int32 cookie = 0;
	BString supportedType;
	while (parent->GetNextSupportedChildType(&cookie, &supportedType)
			== B_OK) {
		BMessage* message = new BMessage(MSG_PARTITION_TYPE);
		message->AddString("type", supportedType);
		BMenuItem* item = new BMenuItem(supportedType, message);
		fTypePopUpMenu->AddItem(item);

		if (strcmp(supportedType, kPartitionTypeBFS) == 0)
			item->SetMarked(true);
	}

	fTypeMenuField = new BMenuField(B_TRANSLATE("Partition type:"),
		fTypePopUpMenu);

	const float spacing = be_control_look->DefaultItemSpacing();
	BGroupLayout* layout = new BGroupLayout(B_VERTICAL, spacing);
	layout->SetInsets(spacing, spacing, spacing, spacing);

	SetLayout(layout);

	AddChild(BGroupLayoutBuilder(B_VERTICAL, spacing)
		.Add(fSizeSlider)
		.Add(fSizeTextControl)
		.Add(BGridLayoutBuilder(0.0, 5.0)
			.Add(fNameTextControl->CreateLabelLayoutItem(), 0, 0)
			.Add(fNameTextControl->CreateTextViewLayoutItem(), 1, 0)
			.Add(fTypeMenuField->CreateLabelLayoutItem(), 0, 1)
			.Add(fTypeMenuField->CreateMenuBarLayoutItem(), 1, 1)
		)
	);

	status_t err = parent->GetParameterEditor(B_CREATE_PARAMETER_EDITOR, &fEditor);
	if (err == B_OK && fEditor != NULL)
		AddChild(fEditor->View());
	else
		fEditor = NULL;

	BButton* okButton = new BButton(B_TRANSLATE("Create"),
		new BMessage(MSG_OK));
	AddChild(BGroupLayoutBuilder(B_HORIZONTAL, spacing)
		.AddGlue()
		.Add(new BButton(B_TRANSLATE("Cancel"), new BMessage(MSG_CANCEL)))
		.Add(okButton)
	);
	SetDefaultButton(okButton);

	AddToSubset(fWindow);
	layout->View()->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void
CreateParamsPanel::_UpdateTextControl()
{
	BString sizeString;
	sizeString << fSizeSlider->Value();
	fSizeTextControl->SetText(sizeString.String());
}
