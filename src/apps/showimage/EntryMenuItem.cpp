/*
 * Copyright 2003-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer, laplace@haiku-os.org
 */

#include "EntryMenuItem.h"

#include <Node.h>
#include <NodeInfo.h>


EntryMenuItem::EntryMenuItem(entry_ref* ref, const char* label,
	BMessage* message, char shortcut, uint32 modifiers)
	:
	BMenuItem(label, message, shortcut, modifiers),
	fEntry(*ref),
	fSmallIcon(NULL)
{
}


EntryMenuItem::~EntryMenuItem()
{
	delete fSmallIcon;
}


void 
EntryMenuItem::GetContentSize(float* width, float* height)
{
	BMenuItem::GetContentSize(width, height);
	*width += kTextIndent;
	if (*height < kIconSize) {
		*height = kIconSize;
	}
}


void
EntryMenuItem::DrawContent()
{
	BView* view = Menu();
	BPoint pos(view->PenLocation());

	if (fSmallIcon == NULL) {
		fSmallIcon = LoadIcon(); // Load on demand
	}

	view->MovePenBy(kTextIndent, 0);
	BMenuItem::DrawContent();
	
	if (fSmallIcon) {
		view->SetDrawingMode(B_OP_OVER);
		view->DrawBitmap(fSmallIcon, pos);
	}
}


BBitmap*
EntryMenuItem::GetIcon(BMimeType* mimeType)
{
	BBitmap* icon;
	icon = new BBitmap(BRect(0, 0, kIconSize - 1, kIconSize - 1), B_CMAP8);
	if (mimeType->GetIcon(icon, B_MINI_ICON) != B_OK) {
		delete icon;
		icon = NULL;
	}
	return icon;
}


BBitmap*
EntryMenuItem::LoadIcon()
{
	BBitmap* icon = NULL;
	BNode node(&fEntry);
	BNodeInfo info(&node);
	char type[B_MIME_TYPE_LENGTH+1];

	// Note: BNodeInfo::GetTrackerIcon does not work as expected!

	// Try to get the icon stored in file attribute
	icon = new BBitmap(BRect(0, 0, kIconSize-1, kIconSize-1), B_CMAP8);
	if (info.GetIcon(icon, B_MINI_ICON) == B_OK) {
		return icon;
	} 
	delete icon;
	icon = NULL;

	// Get the icon from type
	if (info.GetType(type) == B_OK) {
		BMimeType mimeType(type);
		BMimeType superType;
		if (mimeType.InitCheck() == B_OK) {
			icon = GetIcon(&mimeType);
			// Or super type
			if (icon == NULL && mimeType.GetSupertype(&superType) == B_OK) {
				icon = GetIcon(&superType);
			}
		}
	}

	return icon;
}

