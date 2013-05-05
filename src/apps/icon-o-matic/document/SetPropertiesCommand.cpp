/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "SetPropertiesCommand.h"

#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>

#include "CommonPropertyIDs.h"
#include "IconObject.h"
#include "Property.h"
#include "PropertyObject.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-Properties"


// constructor
SetPropertiesCommand::SetPropertiesCommand(IconObject** objects,
										   int32 objectCount,
										   PropertyObject* previous,
										   PropertyObject* current)
	: Command(),
	  fObjects(objects),
	  fObjectCount(objectCount),

	  fOldProperties(previous),
	  fNewProperties(current)
{
}

// destructor
SetPropertiesCommand::~SetPropertiesCommand()
{
	delete[] fObjects;
	delete fOldProperties;
	delete fNewProperties;
}

// InitCheck
status_t
SetPropertiesCommand::InitCheck()
{
	return fObjects && fOldProperties && fNewProperties
		   && fObjectCount > 0 && fOldProperties->CountProperties() > 0
		   && fOldProperties->ContainsSameProperties(*fNewProperties) ?
		   B_OK : B_NO_INIT;
}

// Perform
status_t
SetPropertiesCommand::Perform()
{
	for (int32 i = 0; i < fObjectCount; i++) {
		if (fObjects[i])
			fObjects[i]->SetToPropertyObject(fNewProperties);
	}
	return B_OK;
}

// Undo
status_t
SetPropertiesCommand::Undo()
{
	for (int32 i = 0; i < fObjectCount; i++) {
		if (fObjects[i])
			fObjects[i]->SetToPropertyObject(fOldProperties);
	}
	return B_OK;
}

// GetName
void
SetPropertiesCommand::GetName(BString& name)
{
	if (fOldProperties->CountProperties() > 1) {
		if (fObjectCount > 1)
			name << B_TRANSLATE("Multi Paste Properties");
		else
			name << B_TRANSLATE("Paste Properties");
	} else {
		BString property = name_for_id(
			fOldProperties->PropertyAt(0)->Identifier());
		if (fObjectCount > 1)
			name << B_TRANSLATE_CONTEXT("Multi Set ",
				"Multi Set (property name)") << property;
		else
			name << B_TRANSLATE_CONTEXT("Set ", "Set (property name)")
				 << property;
	}
}
