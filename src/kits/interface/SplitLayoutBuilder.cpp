/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <SplitLayoutBuilder.h>

#include <new>


using std::nothrow;


// constructor
BSplitLayoutBuilder::BSplitLayoutBuilder(enum orientation orientation,
		float spacing)
	: fView(new BSplitView(orientation, spacing))
{
}

// constructor
BSplitLayoutBuilder::BSplitLayoutBuilder(BSplitView* view)
	: fView(view)
{
}

// SplitView
BSplitView*
BSplitLayoutBuilder::SplitView() const
{
	return fView;
}

// GetSplitView
BSplitLayoutBuilder&
BSplitLayoutBuilder::GetSplitView(BSplitView** view)
{
	*view = fView;
	return *this;
}

// Add
BSplitLayoutBuilder&
BSplitLayoutBuilder::Add(BView* view)
{
	fView->AddChild(view);
	return *this;
}

// Add
BSplitLayoutBuilder&
BSplitLayoutBuilder::Add(BView* view, float weight)
{
	fView->AddChild(view, weight);
	return *this;
}

// Add
BSplitLayoutBuilder&
BSplitLayoutBuilder::Add(BLayoutItem* item)
{
	fView->AddChild(item);
	return *this;
}

// Add
BSplitLayoutBuilder&
BSplitLayoutBuilder::Add(BLayoutItem* item, float weight)
{
	fView->AddChild(item, weight);
	return *this;
}

// SetCollapsible
BSplitLayoutBuilder&
BSplitLayoutBuilder::SetCollapsible(bool collapsible)
{
	int32 count = fView->CountChildren();
	if (count > 0)
		fView->SetCollapsible(count - 1, collapsible);
	return *this;
}

// SetInsets
BSplitLayoutBuilder&
BSplitLayoutBuilder::SetInsets(float left, float top, float right, float bottom)
{
	fView->SetInsets(left, top, right, bottom);

	return *this;
}

// cast operator BSplitView*
BSplitLayoutBuilder::operator BSplitView*()
{
	return fView;
}

