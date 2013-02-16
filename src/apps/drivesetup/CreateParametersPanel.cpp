/*
 * Copyright 2008-2013 Haiku, Inc. All rights reserved.
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

static const uint32 kMegaByte = 0x100000;


CreateParametersPanel::CreateParametersPanel(BWindow* window,
	BPartition* partition, off_t offset, off_t size)
	:
	ChangeParametersPanel(window)
{
	_CreateCreateControls(partition, offset, size);

	Init(B_CREATE_PARAMETER_EDITOR, "", partition);
}


CreateParametersPanel::~CreateParametersPanel()
{
}


status_t
CreateParametersPanel::Go(off_t& offset, off_t& size, BString& name,
	BString& type, BString& parameters)
{
	// The object will be deleted in Go(), so we need to get the values via
	// a BMessage

	BMessage storage;
	status_t status = ChangeParametersPanel::Go(name, type, parameters,
		storage);
	if (status != B_OK)
		return status;

	// Return the value back as bytes.
	size = storage.GetInt64("size", 0);
	offset = storage.GetInt64("offset", 0);

	return B_OK;
}


void
CreateParametersPanel::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SIZE_SLIDER:
			_UpdateSizeTextControl();
			break;

		case MSG_SIZE_TEXTCONTROL:
		{
			off_t size = atoi(fSizeTextControl->Text()) * kMegaByte;
			if (size >= 0 && size <= fSizeSlider->MaxPartitionSize())
				fSizeSlider->SetSize(size);
			else
				_UpdateSizeTextControl();
			break;
		}

		default:
			ChangeParametersPanel::MessageReceived(message);
	}
}


bool
CreateParametersPanel::NeedsEditor() const
{
	return false;
}


status_t
CreateParametersPanel::ParametersReceived(const BString& parameters,
	BMessage& storage)
{
	// Return the value back as bytes.
	status_t status = storage.SetInt64("size", fSizeSlider->Size());
	if (status == B_OK)
		status = storage.SetInt64("offset", fSizeSlider->Offset());

	if (status != B_OK)
		return status;

	return ChangeParametersPanel::ParametersReceived(parameters, storage);
}


void
CreateParametersPanel::AddControls(BLayoutBuilder::Group<>& builder,
	BView* editorView)
{
	builder
		.Add(fSizeSlider)
		.Add(fSizeTextControl);

	ChangeParametersPanel::AddControls(builder, editorView);
}


void
CreateParametersPanel::_CreateCreateControls(BPartition* parent, off_t offset,
	off_t size)
{
	// Setup the controls
	// TODO: use a lower granularity for smaller disks -- but this would
	// require being able to parse arbitrary size strings with unit
	fSizeSlider = new SizeSlider("Slider", B_TRANSLATE("Partition size"), NULL,
		offset, size, kMegaByte);
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

	CreateChangeControls(parent);

	fOkButton->SetLabel(B_TRANSLATE("Create"));
}


void
CreateParametersPanel::_UpdateSizeTextControl()
{
	BString sizeString;
	sizeString << fSizeSlider->Size() / kMegaByte;
	fSizeTextControl->SetText(sizeString.String());
}
