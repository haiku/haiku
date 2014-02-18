/*
 * Copyright 2015, François Revol <revol@free.fr>
 * Copyright 2009-2010, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2009, Bryce Groff, brycegroff@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>

#include "InitializeParameterEditor.h"

#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <GridLayoutBuilder.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Partition.h>
#include <PopUpMenu.h>
#include <SpaceLayoutItem.h>
#include <TextControl.h>
#include <Variant.h>
#include <View.h>
#include <Window.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FAT_Initialize_Parameter"


static uint32 MSG_NAME_CHANGED = 'nmch';
static uint32 MSG_FAT_BITS = 'fatb';

InitializeFATEditor::InitializeFATEditor()
	:
	BPartitionParameterEditor(),
	fView(NULL),
	fNameControl(NULL),
	fFatBitsMenuField(NULL),
	fParameters(NULL)
{
	_CreateViewControls();
}


InitializeFATEditor::~InitializeFATEditor()
{
}


void
InitializeFATEditor::SetTo(BPartition* partition)
{
	BString name = partition->ContentName();
	if (!name.IsEmpty())
		fNameControl->SetText(name.String());
}


BView*
InitializeFATEditor::View()
{
	return fView;
}


bool
InitializeFATEditor::ValidateParameters() const
{
	// The name must be set
	return fNameControl->TextView()->TextLength() > 0;
}


status_t
InitializeFATEditor::ParameterChanged(const char* name,
	const BVariant& variant)
{
	if (!strcmp(name, "name"))
		fNameControl->SetText(variant.ToString());
	return B_OK;
}


status_t
InitializeFATEditor::GetParameters(BString& parameters)
{
	parameters = "";

	if (BMenuItem* item = fFatBitsMenuField->Menu()->FindMarked()) {
		const char* size;
		BMessage* message = item->Message();
		if (!message || message->FindString("fat", &size) < B_OK)
			size = "0"; // autodetect
		// TODO: use libroot driver settings API
		parameters << "fat " << size << ";\n";
	}

	parameters << "name \"";
	parameters << fNameControl->Text() << "\";\n";
printf("p:%s", parameters.String());
	return B_OK;
}


void
InitializeFATEditor::_CreateViewControls()
{
	fNameControl = new BTextControl(B_TRANSLATE("Name:"), "New FAT Vol",
		NULL);
	fNameControl->SetModificationMessage(new BMessage(MSG_NAME_CHANGED));
	// TODO find out what is the max length for this specific FS partition name
	fNameControl->TextView()->SetMaxBytes(11);

	BPopUpMenu* fatBitsMenu = new BPopUpMenu("fat size");
	BMessage* message = new BMessage(MSG_FAT_BITS);
	message->AddString("fat", "0");
	BMenuItem* defaultItem = new BMenuItem(
		B_TRANSLATE("Auto (default)"), message);
	fatBitsMenu->AddItem(defaultItem);
	message = new BMessage(MSG_FAT_BITS);
	message->AddString("fat", "12");
	fatBitsMenu->AddItem(new BMenuItem("12", message));
	message = new BMessage(MSG_FAT_BITS);
	message->AddString("fat", "16");
	fatBitsMenu->AddItem(new BMenuItem("16", message));
	message = new BMessage(MSG_FAT_BITS);
	message->AddString("fat", "32");
	fatBitsMenu->AddItem(new BMenuItem("32", message));

	fFatBitsMenuField = new BMenuField(B_TRANSLATE("FAT bits:"),
		fatBitsMenu);
	defaultItem->SetMarked(true);

	float spacing = be_control_look->DefaultItemSpacing();

	fView = BGridLayoutBuilder(spacing, spacing)
		// row 1
		.Add(fNameControl->CreateLabelLayoutItem(), 0, 0)
		.Add(fNameControl->CreateTextViewLayoutItem(), 1, 0)

		// row 2
		.Add(fFatBitsMenuField->CreateLabelLayoutItem(), 0, 1)
		.Add(fFatBitsMenuField->CreateMenuBarLayoutItem(), 1, 1).View()
	;
}
