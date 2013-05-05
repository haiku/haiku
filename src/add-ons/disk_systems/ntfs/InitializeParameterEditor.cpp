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
#include <ControlLook.h>
#include <GridLayoutBuilder.h>
#include <Partition.h>
#include <SpaceLayoutItem.h>
#include <TextControl.h>
#include <Variant.h>
#include <View.h>
#include <Window.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "NTFS_Initialize_Parameter"


static uint32 MSG_NAME_CHANGED = 'nmch';


InitializeNTFSEditor::InitializeNTFSEditor()
	:
	BPartitionParameterEditor(),
	fView(NULL),
	fNameControl(NULL),
	fParameters(NULL)
{
	_CreateViewControls();
}


InitializeNTFSEditor::~InitializeNTFSEditor()
{
}


void
InitializeNTFSEditor::SetTo(BPartition* partition)
{
	BString name = partition->ContentName();
	if (!name.IsEmpty())
		fNameControl->SetText(name.String());
}


BView*
InitializeNTFSEditor::View()
{
	return fView;
}


bool
InitializeNTFSEditor::ValidateParameters() const
{
	// The name must be set
	return fNameControl->TextView()->TextLength() > 0;
}


status_t
InitializeNTFSEditor::ParameterChanged(const char* name,
	const BVariant& variant)
{
	if (!strcmp(name, "name"))
		fNameControl->SetText(variant.ToString());
	return B_OK;
}


status_t
InitializeNTFSEditor::GetParameters(BString& parameters)
{
	parameters = "name \"";
	parameters << fNameControl->Text() << "\";\n";
	return B_OK;
}


void
InitializeNTFSEditor::_CreateViewControls()
{
	fNameControl = new BTextControl(B_TRANSLATE("Name:"), "New NTFS Volume",
		NULL);
	fNameControl->SetModificationMessage(new BMessage(MSG_NAME_CHANGED));
	fNameControl->TextView()->SetMaxBytes(127);

	float spacing = be_control_look->DefaultItemSpacing();

	fView = BGridLayoutBuilder(spacing, spacing)
		.Add(fNameControl->CreateLabelLayoutItem(), 0, 0)
		.Add(fNameControl->CreateTextViewLayoutItem(), 1, 0).View()
	;
}
