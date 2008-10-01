#include "ViewItem.h"
#include <View.h>
#include <Font.h>
#include <stdio.h>
#include <Debug.h>


ViewItem::ViewItem(BRect bounds, const char *name, 
	uint32 resizeMask, uint32 flags, uint32 level, bool expanded)
	: BView(bounds, name, resizeMask, flags), BListItem(level, expanded)
{
	fOwner = NULL;
	SetWidth(bounds.Width());
	SetHeight(bounds.Height());
}


ViewItem::~ViewItem()
{
	if (fOwner)
		fOwner->RemoveChild(this);
}


void
ViewItem::DrawItem(BView *ownerview, BRect frame, bool complete)
{
	(void)frame; (void)complete;
	if (!fOwner) {
		fOwner = dynamic_cast<BListView *>(ownerview);
		if (!ownerview)
			return;
		ownerview->AddChild(this);
	}
#if 0
	const BListItem **list = fOwner->Items();
	for (long i = 0; i < fOwner->CountItems(); i++) {
		if (list[i] == this) {
			BRect frame(fOwner->ItemFrame(i));
			//ResizeTo(frame.Width(), frame.Height());
			//MoveTo(frame.left, frame.top);
		}
	}
#endif
	//BListItem::DrawItem(ownerview, frame, complete);
}


void
ViewItem::Update(BView *ownerview, const BFont *font)
{
	PRINT(("ViewItem::Update()\n"));
	(void)font;
	if (!fOwner) {
		fOwner = dynamic_cast<BListView *>(ownerview);
		if (!ownerview)
			return;
	PRINT(("ViewItem::Update(), fOwner=%p\n", fOwner));
		fOwner->AddChild(this);
	}
	//BListItem::Update(ownerview, font);
	const BListItem **list = fOwner->Items();
	for (long i = 0; i < fOwner->CountItems(); i++) {
		if (list[i] == this) {
			BRect frame(fOwner->ItemFrame(i));
			ResizeTo(frame.Width(), Bounds().Height());
			MoveTo(frame.left, frame.top);
		}
	}
	SetWidth(Bounds().Width());
	SetHeight(Bounds().Height());
}

