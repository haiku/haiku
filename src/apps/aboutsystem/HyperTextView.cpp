/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT license.
 */

#include "HyperTextView.h"

#include <Cursor.h>
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
HyperTextAction::MouseOver(HyperTextView* view, BPoint where, BMessage* message)
{
	BCursor linkCursor(B_CURSOR_ID_FOLLOW_LINK);
	view->SetViewCursor(&linkCursor);
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


HyperTextView::HyperTextView(const char* name, uint32 flags)
	:
	BTextView(name, flags),
	fActionInfos(new ActionInfoList(100, true))
{
}


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

	BTextView::MouseDown(where);
}


void
HyperTextView::MouseUp(BPoint where)
{
	BMessage* message = Window()->CurrentMessage();

	HyperTextAction* action = _ActionAt(where);
	if (action != NULL)
		action->Clicked(this, where, message);

	BTextView::MouseUp(where);
}


void
HyperTextView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	BMessage* message = Window()->CurrentMessage();

	uint32 buttons;
	HyperTextAction* action;
	if (message->FindInt32("buttons", (int32*)&buttons) == B_OK
		&& buttons == 0 && (action = _ActionAt(where)) != NULL) {
		action->MouseOver(this, where, message);
		return;
	}

	BTextView::MouseMoved(where, transit, dragMessage);
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


HyperTextAction*
HyperTextView::_ActionAt(const BPoint& where) const
{
	int32 offset = OffsetAt(where);

	ActionInfo pointer(offset, offset + 1, NULL);

	const ActionInfo* action = fActionInfos->BinarySearch(pointer,
			ActionInfo::CompareEqualIfIntersecting);
	if (action != NULL) {
		// verify that the text region was hit
		BRegion textRegion;
		GetTextRegion(action->startOffset, action->endOffset, &textRegion);
		if (textRegion.Contains(where))
			return action->action;
	}

	return NULL;
}

