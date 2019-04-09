/*
 * Copyright 2013, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2009-2010, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2009, Bryce Groff, brycegroff@gmail.com.
 * Copyright 2019, Les De Ridder, les@lesderid.net
 *
 * Distributed under the terms of the MIT License.
 */


#include <cstdio>

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

#include "btrfs.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "BTRFS_Initialize_Parameter"


static uint32 MSG_NAME_CHANGED = 'nmch';


InitializeBTRFSEditor::InitializeBTRFSEditor()
	:
	BPartitionParameterEditor(),
	fView(NULL),
	fNameControl(NULL),
	fParameters(NULL)
{
	_CreateViewControls();
}


InitializeBTRFSEditor::~InitializeBTRFSEditor()
{
}


void
InitializeBTRFSEditor::SetTo(BPartition* partition)
{
	BString name = partition->Name();
	if (name.IsEmpty())
		name = partition->ContentName();
	if (!name.IsEmpty())
		fNameControl->SetText(name.String());
}


BView*
InitializeBTRFSEditor::View()
{
	return fView;
}


bool
InitializeBTRFSEditor::ValidateParameters() const
{
	// The name must be set
	return fNameControl->TextView()->TextLength() > 0;
}


status_t
InitializeBTRFSEditor::ParameterChanged(const char* name,
	const BVariant& variant)
{
	if (!strcmp(name, "name"))
		fNameControl->SetText(variant.ToString());
	return B_OK;
}


status_t
InitializeBTRFSEditor::GetParameters(BString& parameters)
{
	parameters = "name \"";
	parameters << fNameControl->Text() << "\";\n";
	return B_OK;
}


void
InitializeBTRFSEditor::_CreateViewControls()
{
	fNameControl = new BTextControl(B_TRANSLATE("Name:"), "New Btrfs Volume",
		NULL);
	fNameControl->SetModificationMessage(new BMessage(MSG_NAME_CHANGED));
	fNameControl->TextView()->SetMaxBytes(BTRFS_LABEL_SIZE);

	// TODO(lesderid): Controls for block size, sector size, uuid, etc.

	float spacing = be_control_look->DefaultItemSpacing();

	fView = BGridLayoutBuilder(spacing, spacing)
		.Add(fNameControl->CreateLabelLayoutItem(), 0, 0)
		.Add(fNameControl->CreateTextViewLayoutItem(), 1, 0).View()
	;
}
