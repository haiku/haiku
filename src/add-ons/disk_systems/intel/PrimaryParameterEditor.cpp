/*
 * Copyright 2013, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2009, Bryce Groff, brycegroff@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include "PrimaryParameterEditor.h"

#include <Catalog.h>
#include <DiskDeviceTypes.h>
#include <GroupView.h>
#include <Partition.h>
#include <PartitionParameterEditor.h>
#include <Variant.h>
#include <View.h>
#include <driver_settings.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PrimaryPartitionEditor"


PrimaryPartitionEditor::PrimaryPartitionEditor(bool create)
	:
	fCreate(create)
{
	fActiveCheckBox = new BCheckBox("active", B_TRANSLATE("Active partition"),
		NULL);
	fView = new BGroupView(B_VERTICAL);
	fView->AddChild(fActiveCheckBox);
}


PrimaryPartitionEditor::~PrimaryPartitionEditor()
{
}


void
PrimaryPartitionEditor::SetTo(BPartition* partition)
{
	bool active = false;
	if (fCreate) {
		active = partition->CountChildren() == 0;
	} else {
		void* handle = parse_driver_settings_string(partition->Parameters());
		active = get_driver_boolean_parameter(handle, "active", false, true);
		unload_driver_settings(handle);
	}
	fActiveCheckBox->SetValue(active ? B_CONTROL_ON : B_CONTROL_OFF);
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
	if (strcmp(name, "type") == 0) {
		fActiveCheckBox->SetEnabled(strcmp(variant.ToString(),
			kPartitionTypeIntelExtended) != 0);
		fActiveCheckBox->SetValue(false);
	}

	if (strcmp(name, "active") == 0)
		fActiveCheckBox->SetValue(variant.ToBool());

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
