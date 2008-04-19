/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT license.
 */

#include "HyperTextView.h"

#include <Message.h>
#include <Region.h>
#include <Window.h>

#include <ObjectList.h>


// #pragma mark - HyperTextAction


HyperTextAction::HyperTextAction()
{
}


HyperTextAction::~HyperTextAction()
{
}


void
HyperTextAction::Clicked(HyperTextView* view, BPoint where, BMessage* message)
{
}


// #pragma mark - HyperTextView


struct HyperTextView::ActionInfo {
	ActionInfo(int32 startOffset, int32 endOffset, HyperTextAction* action)
		:
		startOffset(startOffset),
		endOffset(endOffset),
		action(action)
	{
	}

	~ActionInfo()
	{
		delete action;
	}

	static int Compare(const ActionInfo* a, const ActionInfo* b)
	{
		return a->startOffset - b->startOffset;
	}

	static int CompareEqualIfIntersecting(const ActionInfo* a,
		const ActionInfo* b)
	{
		if (a->startOffset < b->endOffset && b->startOffset < a->endOffset)
			return 0;
		return a->startOffset - b->startOffset;
	}

	int32				startOffset;
	int32				endOffset;
	HyperTextAction*	action;
};



class HyperTextView::ActionInfoList
	: public BObjectList<HyperTextView::ActionInfo> {
public:
	ActionInfoList(int32 itemsPerBlock = 20, bool owning = false)
		: BObjectList<HyperTextView::ActionInfo>(itemsPerBlock, owning)
	{
	}
};



HyperTextView::HyperTextView(BRect frame, const char* name, BRect textRect,
	uint32 resizeMask, uint32 flags)
	:
	BTextView(frame, name, textRect, resizeMask, flags),
	fActionInfos(new ActionInfoList(100, true))
{
}


HyperTextView::~HyperTextView()
{
	delete fActionInfos;
}


void
HyperTextView::MouseDown(BPoint where)
{
	// We eat all mouse button events.
}


void
HyperTextView::MouseUp(BPoint where)
{
	BMessage* message = Window()->CurrentMessage();

	int32 offset = OffsetAt(where);

	ActionInfo pointer(offset, offset + 1, NULL);

    const ActionInfo* action = fActionInfos->BinarySearch(pointer,
			ActionInfo::CompareEqualIfIntersecting);
	if (action != NULL) {
		// verify that the text region was hit
		BRegion textRegion;
		GetTextRegion(action->startOffset, action->endOffset, &textRegion);
		if (textRegion.Contains(where))
			action->action->Clicked(this, where, message);
	}
}


void
HyperTextView::AddHyperTextAction(int32 startOffset, int32 endOffset,
	HyperTextAction* action)
{
	if (action == NULL || startOffset >= endOffset) {
		delete action;
		return;
	}

	fActionInfos->BinaryInsert(new ActionInfo(startOffset, endOffset, action),
		ActionInfo::Compare);

	// TODO: Of course we should check for overlaps...
}


void
HyperTextView::InsertHyperText(const char* inText, HyperTextAction* action,
	const text_run_array* inRuns)
{
	int32 startOffset = TextLength();
	Insert(inText, inRuns);
	int32 endOffset = TextLength();

	AddHyperTextAction(startOffset, endOffset, action);
}


void
HyperTextView::InsertHyperText(const char* inText, int32 inLength,
	HyperTextAction* action, const text_run_array* inRuns)
{
	int32 startOffset = TextLength();
	Insert(inText, inLength, inRuns);
	int32 endOffset = TextLength();

	AddHyperTextAction(startOffset, endOffset, action);
}
