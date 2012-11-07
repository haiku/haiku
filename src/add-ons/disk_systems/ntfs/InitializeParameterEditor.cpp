/*
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
#include <PartitionParameterEditor.h>
#include <PopUpMenu.h>
#include <SpaceLayoutItem.h>
#include <TextControl.h>
#include <View.h>
#include <Window.h>


static uint32 MSG_NAME_CHANGED = 'nmch';


InitializeNTFSEditor::InitializeNTFSEditor()
	:
	BPartitionParameterEditor(),
	fView(NULL),
	fNameTC(NULL),
	fParameters(NULL)
{
	_CreateViewControls();
}


InitializeNTFSEditor::~InitializeNTFSEditor()
{
}


BView*
InitializeNTFSEditor::View()
{
	return fView;
}


bool
InitializeNTFSEditor::FinishedEditing()
{
	fParameters = "";
	fParameters << "name \"" << fNameTC->Text() << "\";\n";

	return true;
}


status_t
InitializeNTFSEditor::GetParameters(BString* parameters)
{
	if (parameters == NULL)
		return B_BAD_VALUE;

	*parameters = fParameters;
	return B_OK;
}


status_t
InitializeNTFSEditor::PartitionNameChanged(const char* name)
{
	fNameTC->SetText(name);
	return B_OK;
}


void
InitializeNTFSEditor::_CreateViewControls()
{
	fNameTC = new BTextControl("Name:", "New NTFS Volume", NULL);
	fNameTC->SetModificationMessage(new BMessage(MSG_NAME_CHANGED));
	// TODO find out what is the max length for this specific FS partition name
	fNameTC->TextView()->SetMaxBytes(31);

	float spacing = be_control_look->DefaultItemSpacing();

	fView = BGridLayoutBuilder(spacing, spacing)
		// row 1
		.Add(fNameTC->CreateLabelLayoutItem(), 0, 0)
		.Add(fNameTC->CreateTextViewLayoutItem(), 1, 0).View()
	;
}
