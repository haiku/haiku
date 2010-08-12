/*
 * Copyright 2009, Bryce Groff, brycegroff@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include "CreationParameterEditor.h"

#include <DiskDeviceTypes.h>
#include <GroupView.h>
#include <PartitionParameterEditor.h>
#include <View.h>


PrimaryPartitionEditor::PrimaryPartitionEditor()
	:
	BPartitionParameterEditor(),
	fView(NULL),
	fActiveCB(NULL),
	fParameters(NULL)
{
	fActiveCB = new BCheckBox("active", "Active partition", NULL);
	fView = new BGroupView(B_VERTICAL);
	fView->AddChild(fActiveCB);
}


PrimaryPartitionEditor::~PrimaryPartitionEditor()
{
}


BView*
PrimaryPartitionEditor::View()
{
	return fView;
}


bool
PrimaryPartitionEditor::FinishedEditing()
{
	if (fActiveCB->IsEnabled()) {
		if (fActiveCB->Value() == B_CONTROL_ON)
			fParameters.SetTo("active true ;");
		else
			fParameters.SetTo("active false ;");
	} else fParameters.SetTo("");

	return true;
}


status_t
PrimaryPartitionEditor::GetParameters(BString* parameters)
{
	if (fParameters == NULL)
		return B_BAD_VALUE;

	*parameters = fParameters;
	return B_OK;
}


status_t
PrimaryPartitionEditor::PartitionTypeChanged(const char* type)
{
	fActiveCB->SetEnabled(strcmp(type, kPartitionTypeIntelExtended) != 0);
	return B_OK;
}
