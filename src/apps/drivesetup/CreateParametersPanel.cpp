/*
 * Copyright 2008-2013 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de.
 *		Bryce Groff	<bgroff@hawaii.edu>
 *		Karsten Heimrich <host.haiku@gmx.de>
 */


#include "CreateParametersPanel.h"

#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <DiskDeviceTypes.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <MessageFilter.h>
#include <PopUpMenu.h>
#include <String.h>
#include <TextControl.h>
#include <Variant.h>

#include "Support.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "CreateParametersPanel"


enum {
	MSG_PARTITION_TYPE			= 'type',
	MSG_SIZE_SLIDER				= 'ssld',
	MSG_SIZE_TEXTCONTROL		= 'stct'
};


CreateParametersPanel::CreateParametersPanel(BWindow* window,
	BPartition* partition, off_t offset, off_t size)
	:
	AbstractParametersPanel(window)
{
	Init(B_CREATE_PARAMETER_EDITOR, "", partition);

	// Scale offset, and size from bytes to megabytes (2^20)
	// so that we do not run over a signed int32.
	offset /= kMegaByte;
	size /= kMegaByte;
	_CreateViewControls(partition, offset, size);
}


CreateParametersPanel::~CreateParametersPanel()
{
}


status_t
CreateParametersPanel::Go(off_t& offset, off_t& size, BString& name,
	BString& type, BString& parameters)
{
	// The object will be deleted in Go(), so we need to get the values before

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

	return AbstractParametersPanel::Go(parameters);
}


void
CreateParametersPanel::AddControls(BLayoutBuilder::Group<>& builder,
	BView* editorView)
{
	builder
		.Add(fSizeSlider)
		.Add(fSizeTextControl)
		.AddGrid(0.0, 5.0)
			.Add(fNameTextControl->CreateLabelLayoutItem(), 0, 0)
			.Add(fNameTextControl->CreateTextViewLayoutItem(), 1, 0)
			.Add(fTypeMenuField->CreateLabelLayoutItem(), 0, 1)
			.Add(fTypeMenuField->CreateMenuBarLayoutItem(), 1, 1)
		.End()
		.Add(editorView);
}


void
CreateParametersPanel::_CreateViewControls(BPartition* parent, off_t offset,
	off_t size)
{
	// Setup the controls
	fSizeSlider = new SizeSlider("Slider", B_TRANSLATE("Partition size"), NULL,
		offset, offset + size);
	fSizeSlider->SetPosition(1.0);
	fSizeSlider->SetModificationMessage(new BMessage(MSG_SIZE_SLIDER));

	fSizeTextControl = new BTextControl("Size Control", "", "", NULL);
	for(int32 i = 0; i < 256; i++)
		fSizeTextControl->TextView()->DisallowChar(i);
	for(int32 i = '0'; i <= '9'; i++)
		fSizeTextControl->TextView()->AllowChar(i);
	_UpdateSizeTextControl();
	fSizeTextControl->SetModificationMessage(
		new BMessage(MSG_SIZE_TEXTCONTROL));

	fNameTextControl = new BTextControl("Name Control",
		B_TRANSLATE("Partition name:"),	"", NULL);
	if (!parent->SupportsChildName())
		fNameTextControl->SetEnabled(false);

	fTypePopUpMenu = new BPopUpMenu("Partition Type");

	int32 cookie = 0;
	BString supportedType;
	while (parent->GetNextSupportedChildType(&cookie, &supportedType) == B_OK) {
		BMessage* message = new BMessage(MSG_PARTITION_TYPE);
		message->AddString("type", supportedType);
		BMenuItem* item = new BMenuItem(supportedType, message);
		fTypePopUpMenu->AddItem(item);

		if (strcmp(supportedType, kPartitionTypeBFS) == 0)
			item->SetMarked(true);
	}

	fTypeMenuField = new BMenuField(B_TRANSLATE("Partition type:"),
		fTypePopUpMenu);

	fOkButton->SetLabel(B_TRANSLATE("Create"));
}


void
CreateParametersPanel::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_PARTITION_TYPE:
			if (fEditor != NULL) {
				const char* type;
				if (message->FindString("type", &type) == B_OK)
					fEditor->ParameterChanged("type", BVariant(type));
			}
			break;

		case MSG_SIZE_SLIDER:
			_UpdateSizeTextControl();
			break;

		case MSG_SIZE_TEXTCONTROL:
		{
			int32 size = atoi(fSizeTextControl->Text());
			if (size >= 0 && size <= fSizeSlider->MaxPartitionSize())
				fSizeSlider->SetValue(size + fSizeSlider->Offset());
			else
				_UpdateSizeTextControl();
			break;
		}

		default:
			AbstractParametersPanel::MessageReceived(message);
	}
}


void
CreateParametersPanel::_UpdateSizeTextControl()
{
	BString sizeString;
	sizeString << fSizeSlider->Value() - fSizeSlider->Offset();
	fSizeTextControl->SetText(sizeString.String());
}
