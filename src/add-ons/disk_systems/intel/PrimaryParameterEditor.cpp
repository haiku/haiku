/*
 * Copyright 2013, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2009, Bryce Groff, brycegroff@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include "PrimaryParameterEditor.h"

#include <Catalog.h>
#include <DiskDeviceTypes.h>
#include <GroupView.h>
#include <PartitionParameterEditor.h>
#include <Variant.h>
#include <View.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PrimaryPartitionEditor"


PrimaryPartitionEditor::PrimaryPartitionEditor()
{
	fActiveCheckBox = new BCheckBox("active", B_TRANSLATE("Active partition"),
		NULL);
	fView = new BGroupView(B_VERTICAL);
	fView->AddChild(fActiveCheckBox);
}


PrimaryPartitionEditor::~PrimaryPartitionEditor()
{
}


BView*
PrimaryPartitionEditor::View()
{
	return fView;
}


status_t
PrimaryPartitionEditor::ParameterChanged(const char* name,
	const BVariant& variant)
{
	if (!strcmp(name, "type")) {
		fActiveCheckBox->SetEnabled(strcmp(variant.ToString(),
			kPartitionTypeIntelExtended) != 0);
	}
	return B_OK;
}


status_t
PrimaryPartitionEditor::GetParameters(BString& parameters)
{
	if (fActiveCheckBox->IsEnabled()) {
		if (fActiveCheckBox->Value() == B_CONTROL_ON)
			parameters.SetTo("active true ;");
		else
			parameters.SetTo("active false ;");
	} else
		parameters.SetTo("");

	return B_OK;
}
