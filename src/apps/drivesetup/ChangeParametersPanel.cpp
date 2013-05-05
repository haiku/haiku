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


#include "ChangeParametersPanel.h"

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
#define B_TRANSLATION_CONTEXT "ChangeParametersPanel"


enum {
	MSG_PARTITION_TYPE			= 'type',
};


ChangeParametersPanel::ChangeParametersPanel(BWindow* window,
	BPartition* partition)
	:
	AbstractParametersPanel(window)
{
	CreateChangeControls(partition);

	Init(B_PROPERTIES_PARAMETER_EDITOR, "", partition);
}


ChangeParametersPanel::~ChangeParametersPanel()
{
}


status_t
ChangeParametersPanel::Go(BString& name, BString& type, BString& parameters)
{
	BMessage storage;
	return Go(name, type, parameters, storage);
}


void
ChangeParametersPanel::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_PARTITION_TYPE:
			if (fEditor != NULL) {
				const char* type;
				if (message->FindString("type", &type) == B_OK)
					fEditor->ParameterChanged("type", BVariant(type));
			}
			break;

		default:
			AbstractParametersPanel::MessageReceived(message);
	}
}


// #pragma mark - protected


ChangeParametersPanel::ChangeParametersPanel(BWindow* window)
	:
	AbstractParametersPanel(window)
{
}


status_t
ChangeParametersPanel::Go(BString& name, BString& type, BString& parameters,
	BMessage& storage)
{
	// The object will be deleted in Go(), so we need to get the values via
	// a BMessage
	status_t status = AbstractParametersPanel::Go(parameters, storage);
	if (status != B_OK)
		return status;

	// get name
	name.SetTo(storage.GetString("name", NULL));

	// get type
	type.SetTo(storage.GetString("type", NULL));

	return B_OK;
}


void
ChangeParametersPanel::CreateChangeControls(BPartition* parent)
{
	fNameTextControl = new BTextControl("Name Control",
		B_TRANSLATE("Partition name:"),	"", NULL);
	fSupportsName = parent->SupportsChildName();

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
	fSupportsType = fTypePopUpMenu->CountItems() != 0;

	fOkButton->SetLabel(B_TRANSLATE("Change"));
}


bool
ChangeParametersPanel::NeedsEditor() const
{
	return !fSupportsName && !fSupportsType;
}


bool
ChangeParametersPanel::ValidWithoutEditor() const
{
	return !NeedsEditor();
}


status_t
ChangeParametersPanel::ParametersReceived(const BString& parameters,
	BMessage& storage)
{
	// get name
	status_t status = storage.SetString("name", fNameTextControl->Text());

	// get type
	if (BMenuItem* item = fTypeMenuField->Menu()->FindMarked()) {
		const char* type;
		BMessage* message = item->Message();
		if (!message || message->FindString("type", &type) != B_OK)
			type = kPartitionTypeBFS;

		if (status == B_OK)
			status = storage.SetString("type", type);
	}

	return status;
}


void
ChangeParametersPanel::AddControls(BLayoutBuilder::Group<>& builder,
	BView* editorView)
{
	if (fSupportsName || fSupportsType) {
		BLayoutBuilder::Group<>::GridBuilder gridBuilder
			= builder.AddGrid(0.0, B_USE_DEFAULT_SPACING);

		if (fSupportsName) {
			gridBuilder.Add(fNameTextControl->CreateLabelLayoutItem(), 0, 0)
				.Add(fNameTextControl->CreateTextViewLayoutItem(), 1, 0);
		}
		if (fSupportsType) {
			gridBuilder.Add(fTypeMenuField->CreateLabelLayoutItem(), 0, 1)
				.Add(fTypeMenuField->CreateMenuBarLayoutItem(), 1, 1);
		}
	}

	if (editorView != NULL)
		builder.Add(editorView);
}
