/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <GroupLayoutBuilder.h>

#include <new>

#include <SpaceLayoutItem.h>


using std::nothrow;


// constructor
BGroupLayoutBuilder::BGroupLayoutBuilder(enum orientation orientation,
	float spacing)
	: fRootLayout((new BGroupView(orientation, spacing))->GroupLayout())
{
	_PushLayout(fRootLayout);
}

// constructor
BGroupLayoutBuilder::BGroupLayoutBuilder(BGroupLayout* layout)
	: fRootLayout(layout)
{
	_PushLayout(fRootLayout);
}

// constructor
BGroupLayoutBuilder::BGroupLayoutBuilder(BGroupView* view)
	: fRootLayout(view->GroupLayout())
{
	_PushLayout(fRootLayout);
}

// RootLayout
BGroupLayout*
BGroupLayoutBuilder::RootLayout() const
{
	return fRootLayout;
}

// TopLayout
BGroupLayout*
BGroupLayoutBuilder::TopLayout() const
{
	int32 count = fLayoutStack.CountItems();
	return (count > 0
		? (BGroupLayout*)fLayoutStack.ItemAt(count - 1) : NULL);
}

// GetTopLayout
BGroupLayoutBuilder&
BGroupLayoutBuilder::GetTopLayout(BGroupLayout** _layout)
{
	*_layout = TopLayout();
	return *this;
}

// TopView
BView*
BGroupLayoutBuilder::TopView() const
{
	if (BGroupLayout* layout = TopLayout())
		return layout->Owner();
	return NULL;
}

// GetTopView
BGroupLayoutBuilder&
BGroupLayoutBuilder::GetTopView(BView** _view)
{
	if (BGroupLayout* layout = TopLayout())
		*_view = layout->Owner();
	else
		*_view = NULL;

	return *this;
}

// Add
BGroupLayoutBuilder&
BGroupLayoutBuilder::Add(BView* view)
{
	if (BGroupLayout* layout = TopLayout())
		layout->AddView(view);
	return *this;
}

// Add
BGroupLayoutBuilder&
BGroupLayoutBuilder::Add(BView* view, float weight)
{
	if (BGroupLayout* layout = TopLayout())
		layout->AddView(view, weight);
	return *this;
}

// Add
BGroupLayoutBuilder&
BGroupLayoutBuilder::Add(BLayoutItem* item)
{
	if (BGroupLayout* layout = TopLayout())
		layout->AddItem(item);
	return *this;
}

// Add
BGroupLayoutBuilder&
BGroupLayoutBuilder::Add(BLayoutItem* item, float weight)
{
	if (BGroupLayout* layout = TopLayout())
		layout->AddItem(item, weight);
	return *this;
}

// AddGroup
BGroupLayoutBuilder&
BGroupLayoutBuilder::AddGroup(enum orientation orientation,
	float spacing, float weight)
{
	if (BGroupLayout* layout = TopLayout()) {
		BGroupView* group = new(nothrow) BGroupView(orientation, spacing);
		if (group) {
			if (layout->AddView(group, weight))
				_PushLayout(group->GroupLayout());
			else
				delete group;
		}
	}

	return *this;
}

// End
BGroupLayoutBuilder&
BGroupLayoutBuilder::End()
{
	_PopLayout();
	return *this;
}

// AddGlue
BGroupLayoutBuilder&
BGroupLayoutBuilder::AddGlue(float weight)
{
	if (BGroupLayout* layout = TopLayout())
		layout->AddItem(BSpaceLayoutItem::CreateGlue(), weight);

	return *this;
}

// AddStrut
BGroupLayoutBuilder&
BGroupLayoutBuilder::AddStrut(float size)
{
	if (BGroupLayout* layout = TopLayout()) {
		if (layout->Orientation() == B_HORIZONTAL)
			layout->AddItem(BSpaceLayoutItem::CreateHorizontalStrut(size));
		else
			layout->AddItem(BSpaceLayoutItem::CreateVerticalStrut(size));
	}

	return *this;
}

// SetInsets
BGroupLayoutBuilder& 
BGroupLayoutBuilder::SetInsets(float left, float top, float right, float bottom)
{
	if (BGroupLayout* layout = TopLayout())
		layout->SetInsets(left, top, right, bottom);

	return *this;
}
	
// cast operator BGroupLayout*
BGroupLayoutBuilder::operator BGroupLayout*()
{
	return fRootLayout;
}

// _PushLayout
bool
BGroupLayoutBuilder::_PushLayout(BGroupLayout* layout)
{
	return fLayoutStack.AddItem(layout);
}

// _PopLayout
void
BGroupLayoutBuilder::_PopLayout()
{
	int32 count = fLayoutStack.CountItems();
	if (count > 0)
		fLayoutStack.RemoveItem(count - 1);
}
