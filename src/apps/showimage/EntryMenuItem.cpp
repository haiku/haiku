/*****************************************************************************/
// EntryMenuItem
// Written by Michael Pfeiffer
//
// EntryMenuItem.cpp
//
//
// Copyright (c) 2003 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#include <Node.h>
#include <NodeInfo.h>

#include "EntryMenuItem.h"

EntryMenuItem::EntryMenuItem(entry_ref* ref, const char* label, BMessage* message, char shortcut, uint32 modifiers)
	: BMenuItem(label, message, shortcut, modifiers)
	, fEntry(*ref)
	, fSmallIcon(NULL)
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
		fSmallIcon = LoadIcon(); // load on demand
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
	icon = new BBitmap(BRect(0, 0, kIconSize-1, kIconSize-1), B_CMAP8);
	if (mimeType->GetIcon(icon, B_MINI_ICON) != B_OK) {
		delete icon; icon = NULL;
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

	// try to get the icon stored in file attribute
	icon = new BBitmap(BRect(0, 0, kIconSize-1, kIconSize-1), B_CMAP8);
	if (info.GetIcon(icon, B_MINI_ICON) == B_OK) {
		return icon;
	} 
	delete icon; icon = NULL;

	// get the icon from type
	if (info.GetType(type) == B_OK) {
		BMimeType mimeType(type);
		BMimeType superType;
		if (mimeType.InitCheck() == B_OK) {
			icon = GetIcon(&mimeType);
			// or super type
			if (icon == NULL && mimeType.GetSupertype(&superType) == B_OK) {
				icon = GetIcon(&superType);
			}
		}
	}

	return icon;
}
