/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "RemoveStylesCommand.h"

#include <new>
#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>

#include "StyleContainer.h"
#include "Shape.h"
#include "Style.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-RemoveStylesCmd"


using std::nothrow;

// constructor
RemoveStylesCommand::RemoveStylesCommand(StyleContainer* container,
										 Style** const styles,
										 int32 count)
	: Command(),
	  fContainer(container),
	  fInfos(styles && count > 0 ? new (nothrow) StyleInfo[count] : NULL),
	  fCount(count),
	  fStylesRemoved(false)
{
	if (!fContainer || !fInfos)
		return;

	for (int32 i = 0; i < fCount; i++) {
		fInfos[i].style = styles[i];
		fInfos[i].index = fContainer->IndexOf(styles[i]);
		if (styles[i]) {
			int32 listenerCount = styles[i]->CountObservers();
			for (int32 j = 0; j < listenerCount; j++) {
				Shape* shape = dynamic_cast<Shape*>(styles[i]->ObserverAtFast(j));
				if (shape)
					fInfos[i].shapes.AddItem((void*)shape);
			}
		}
	}
}

// destructor
RemoveStylesCommand::~RemoveStylesCommand()
{
	if (fStylesRemoved && fInfos) {
		for (int32 i = 0; i < fCount; i++) {
			if (fInfos[i].style)
				fInfos[i].style->Release();
		}
	}
	delete[] fInfos;
}

// InitCheck
status_t
RemoveStylesCommand::InitCheck()
{
	return fContainer && fInfos ? B_OK : B_NO_INIT;
}

// Perform
status_t
RemoveStylesCommand::Perform()
{
	// remove styles from container
	for (int32 i = 0; i < fCount; i++) {
		if (!fInfos[i].style)
			continue;
		fContainer->RemoveStyle(fInfos[i].style);
	}

	// remove styles from shapes that reference them
	for (int32 i = 0; i < fCount; i++) {
		if (!fInfos[i].style)
			continue;
		int32 shapeCount = fInfos[i].shapes.CountItems();
		for (int32 j = 0; j < shapeCount; j++) {
			Shape* shape = (Shape*)fInfos[i].shapes.ItemAtFast(j);
			shape->SetStyle(fContainer->StyleAt(0));
		}
	}

	fStylesRemoved = true;

	return B_OK;
}

// Undo
status_t
RemoveStylesCommand::Undo()
{
	status_t ret = B_OK;

	// add styles to container and shapes which previously referenced them
	for (int32 i = 0; i < fCount; i++) {
		if (!fInfos[i].style)
			continue;
		if (!fContainer->AddStyle(fInfos[i].style, fInfos[i].index)) {
			// roll back
			ret = B_ERROR;
			for (int32 j = i - 1; j >= 0; j--) {
				fContainer->RemoveStyle(fInfos[j].style);
			}
			for (int32 j = i - 1; j >= 0; j--) {
				int32 shapeCount = fInfos[j].shapes.CountItems();
				for (int32 k = 0; k < shapeCount; k++) {
					Shape* shape = (Shape*)fInfos[j].shapes.ItemAtFast(k);
					shape->SetStyle(fContainer->StyleAt(0));
				}
			}
			break;
		}
		int32 shapeCount = fInfos[i].shapes.CountItems();
		for (int32 j = 0; j < shapeCount; j++) {
			Shape* shape = (Shape*)fInfos[i].shapes.ItemAtFast(j);
			shape->SetStyle(fInfos[i].style);
		}
	}

	fStylesRemoved = false;

	return ret;
}

// GetName
void
RemoveStylesCommand::GetName(BString& name)
{
	if (fCount > 1)
		name << B_TRANSLATE("Remove Styles");
	else
		name << B_TRANSLATE("Remove Style");
}
