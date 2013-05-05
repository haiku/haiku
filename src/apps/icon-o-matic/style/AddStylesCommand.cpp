/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "AddStylesCommand.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Catalog.h>
#include <Locale.h>

#include "StyleContainer.h"
#include "Style.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-AddStylesCmd"


using std::nothrow;

// constructor
AddStylesCommand::AddStylesCommand(StyleContainer* container,
								   Style** const styles,
								   int32 count,
								   int32 index)
	: Command(),
	  fContainer(container),
	  fStyles(styles && count > 0 ? new (nothrow) Style*[count] : NULL),
	  fCount(count),
	  fIndex(index),
	  fStylesAdded(false)
{
	if (!fContainer || !fStyles)
		return;

	memcpy(fStyles, styles, sizeof(Style*) * fCount);
}

// destructor
AddStylesCommand::~AddStylesCommand()
{
	if (!fStylesAdded && fStyles) {
		for (int32 i = 0; i < fCount; i++)
			fStyles[i]->Release();
	}
	delete[] fStyles;
}

// InitCheck
status_t
AddStylesCommand::InitCheck()
{
	return fContainer && fStyles ? B_OK : B_NO_INIT;
}

// Perform
status_t
AddStylesCommand::Perform()
{
	status_t ret = B_OK;

	// add shapes to container
	int32 index = fIndex;
	for (int32 i = 0; i < fCount; i++) {
		if (fStyles[i] && !fContainer->AddStyle(fStyles[i], index)) {
			ret = B_ERROR;
			// roll back
			for (int32 j = i - 1; j >= 0; j--)
				fContainer->RemoveStyle(fStyles[j]);
			break;
		}
		index++;
	}
	fStylesAdded = true;

	return ret;
}

// Undo
status_t
AddStylesCommand::Undo()
{
	// remove shapes from container
	for (int32 i = 0; i < fCount; i++) {
		fContainer->RemoveStyle(fStyles[i]);
	}
	fStylesAdded = false;

	return B_OK;
}

// GetName
void
AddStylesCommand::GetName(BString& name)
{
	if (fCount > 1)
		name << B_TRANSLATE("Add Styles");
	else
		name << B_TRANSLATE("Add Style");
}
