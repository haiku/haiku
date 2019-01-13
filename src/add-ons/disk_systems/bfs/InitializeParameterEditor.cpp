/*
 * Copyright 2013, Axel Dörfler, axeld@pinc-software.de.
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
#define B_TRANSLATION_CONTEXT "BFS_Initialize_Parameter"


static uint32 MSG_BLOCK_SIZE = 'blsz';
static uint32 MSG_NAME_CHANGED = 'nmch';


InitializeBFSEditor::InitializeBFSEditor()
	:
	BPartitionParameterEditor(),
	fView(NULL),
	fNameControl(NULL),
	fBlockSizeMenuField(NULL),
	fUseIndicesCheckBox(NULL)
{
	_CreateViewControls();
}


InitializeBFSEditor::~InitializeBFSEditor()
{
}


void
InitializeBFSEditor::SetTo(BPartition* partition)
{
	BString name = partition->Name();
	if (name.IsEmpty())
		name = partition->ContentName();
	if (!name.IsEmpty())
		fNameControl->SetText(name.String());
}


BView*
InitializeBFSEditor::View()
{
	return fView;
}


bool
InitializeBFSEditor::ValidateParameters() const
{
	// The name must be set
	return fNameControl->TextView()->TextLength() > 0;
}


status_t
InitializeBFSEditor::ParameterChanged(const char* name, const BVariant& variant)
{
	if (!strcmp(name, "name"))
		fNameControl->SetText(variant.ToString());
	return B_OK;
}


status_t
InitializeBFSEditor::GetParameters(BString& parameters)
{
	parameters = "";

	if (BMenuItem* item = fBlockSizeMenuField->Menu()->FindMarked()) {
		const char* size;
		BMessage* message = item->Message();
		if (!message || message->FindString("size", &size) < B_OK)
			size = "2048";
		// TODO: use libroot driver settings API
		parameters << "block_size " << size << ";\n";
	}
	if (fUseIndicesCheckBox->Value() == B_CONTROL_OFF)
		parameters << "noindex;\n";

	parameters << "name \"" << fNameControl->Text() << "\";\n";
	return B_OK;
}


void
InitializeBFSEditor::_CreateViewControls()
{
	fNameControl = new BTextControl(B_TRANSLATE("Name:"), "Haiku", NULL);
	fNameControl->SetModificationMessage(new BMessage(MSG_NAME_CHANGED));
	fNameControl->TextView()->SetMaxBytes(31);

	BPopUpMenu* blocksizeMenu = new BPopUpMenu("blocksize");
	BMessage* message = new BMessage(MSG_BLOCK_SIZE);
	message->AddString("size", "1024");
	blocksizeMenu->AddItem(new BMenuItem(
		B_TRANSLATE("1024 (Mostly small files)"), message));
	message = new BMessage(MSG_BLOCK_SIZE);
	message->AddString("size", "2048");
	BMenuItem* defaultItem = new BMenuItem(B_TRANSLATE("2048 (Recommended)"),
		message);
	blocksizeMenu->AddItem(defaultItem);
	message = new BMessage(MSG_BLOCK_SIZE);
	message->AddString("size", "4096");
	blocksizeMenu->AddItem(new BMenuItem("4096", message));
	message = new BMessage(MSG_BLOCK_SIZE);
	message->AddString("size", "8192");
	blocksizeMenu->AddItem(new BMenuItem(
		B_TRANSLATE("8192 (Mostly large files)"), message));

	fBlockSizeMenuField = new BMenuField(B_TRANSLATE("Blocksize:"),
		blocksizeMenu);
	defaultItem->SetMarked(true);

	fUseIndicesCheckBox = new BCheckBox(B_TRANSLATE("Enable query support"),
		NULL);
	fUseIndicesCheckBox->SetValue(true);
	fUseIndicesCheckBox->SetToolTip(B_TRANSLATE("Disabling query support "
		"may speed up certain file system operations, but should\nonly be "
		"used if one is absolutely certain that one will not need queries."
		"\nAny volume that is intended for booting Haiku must have query "
		"support enabled."));

	float spacing = be_control_look->DefaultItemSpacing();

	fView = BGridLayoutBuilder(spacing, spacing)
		// row 1
		.Add(fNameControl->CreateLabelLayoutItem(), 0, 0)
		.Add(fNameControl->CreateTextViewLayoutItem(), 1, 0)

		// row 2
		.Add(fBlockSizeMenuField->CreateLabelLayoutItem(), 0, 1)
		.Add(fBlockSizeMenuField->CreateMenuBarLayoutItem(), 1, 1)

		// row 3
		.Add(fUseIndicesCheckBox, 0, 2, 2).View()
	;
}
