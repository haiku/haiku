/*
 * Copyright 2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ALMLayoutBuilder.h"


#include <Window.h>

#include "Area.h"


namespace BALM {


BALMLayoutBuilder::BALMLayoutBuilder(BView* view, float hSpacing,
	float vSpacing, BALMLayout* friendLayout)
{
	fLayout = new BALMLayout(hSpacing, vSpacing, friendLayout);
	view->SetLayout(fLayout);
}


BALMLayoutBuilder::BALMLayoutBuilder(BView* view, BALMLayout* layout)
{
	fLayout = layout;
	view->SetLayout(layout);
}


BALMLayoutBuilder::BALMLayoutBuilder(BWindow* window, float hSpacing,
	float vSpacing, BALMLayout* friendLayout)
{
	fLayout = new BALMLayout(hSpacing, vSpacing, friendLayout);
	window->SetLayout(fLayout);

	fLayout->Owner()->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		// TODO: we get a white background if we don't do this
}


BALMLayoutBuilder::BALMLayoutBuilder(BWindow* window, BALMLayout* layout)
{
	fLayout = layout;
	window->SetLayout(fLayout);

	fLayout->Owner()->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		// TODO: we get a white background if we don't do this
}


BALMLayoutBuilder::BALMLayoutBuilder(BALMLayout* layout)
{
	fLayout = layout;
}


BALMLayoutBuilder&
BALMLayoutBuilder::Add(BView* view, XTab* left, YTab* top,
	XTab* right, YTab* bottom)
{
	Area* a = (fLayout->AddView(view, left, top, right, bottom));
	_SetCurrentArea(a);
	return *this;
}


BALMLayoutBuilder&
BALMLayoutBuilder::Add(BView* view, Row* row, Column* column)
{
	_SetCurrentArea(fLayout->AddView(view, row, column));
	return *this;
}


BALMLayoutBuilder&
BALMLayoutBuilder::Add(BLayoutItem* item, XTab* left, YTab* top,
	XTab* right, YTab* bottom)
{
	_SetCurrentArea(fLayout->AddItem(item, left, top, right, bottom));
	return *this;
}


BALMLayoutBuilder&
BALMLayoutBuilder::Add(BLayoutItem* item, Row* row, Column* column)
{
	fLayout->AddItem(item, row, column);
	return *this;
}



BALMLayoutBuilder&
BALMLayoutBuilder::SetInsets(float insets)
{
	fLayout->SetInsets(insets);
	return *this;
}


BALMLayoutBuilder&
BALMLayoutBuilder::SetInsets(float horizontal, float vertical)
{
	fLayout->SetInsets(horizontal, vertical);
	return *this;
}


BALMLayoutBuilder&
BALMLayoutBuilder::SetInsets(float left, float top, float right,
									float bottom)
{
	fLayout->SetInsets(left, top, right, bottom);
	return *this;
}


BALMLayoutBuilder&
BALMLayoutBuilder::SetSpacing(float horizontal, float vertical)
{
	fLayout->SetSpacing(horizontal, vertical);
	return *this;
}


BALMLayoutBuilder&
BALMLayoutBuilder::StartingAt(BView* view)
{
	fAreaStack.MakeEmpty();
	fAreaStack.AddItem(fLayout->AreaFor(view));
	return *this;
}


BALMLayoutBuilder&
BALMLayoutBuilder::StartingAt(BLayoutItem* item)
{
	fAreaStack.MakeEmpty();
	fAreaStack.AddItem(fLayout->AreaFor(item));
	return *this;
}


BALMLayoutBuilder&
BALMLayoutBuilder::AddToLeft(BView* view, XTab* _left, YTab* top, YTab* bottom)
{
	Area* currentArea = _CurrentArea();
	BReference<XTab> left = _left;
	if (_left == NULL)
		left = fLayout->AddXTab();
	XTab* right = currentArea->Left();
	if (!top)
		top = currentArea->Top();
	if (!bottom)
		bottom = currentArea->Bottom();

	return Add(view, left, top, right, bottom);
}


BALMLayoutBuilder&
BALMLayoutBuilder::AddToRight(BView* view, XTab* _right, YTab* top,
	YTab* bottom)
{
	Area* currentArea = _CurrentArea();
	XTab* left = currentArea->Right();
	BReference<XTab> right = _right;
	if (_right == NULL)
		right = fLayout->AddXTab();
	if (!top)
		top = currentArea->Top();
	if (!bottom)
		bottom = currentArea->Bottom();

	return Add(view, left, top, right, bottom);
}


BALMLayoutBuilder&
BALMLayoutBuilder::AddAbove(BView* view, YTab* _top, XTab* left,
	XTab* right)
{
	Area* currentArea = _CurrentArea();
	if (!left)
		left = currentArea->Left();
	if (!right)
		right = currentArea->Right();
	BReference<YTab> top = _top;
	if (_top == NULL)
		top = fLayout->AddYTab();
	YTab* bottom = currentArea->Top();

	return Add(view, left, top, right, bottom);
}


BALMLayoutBuilder&
BALMLayoutBuilder::AddBelow(BView* view, YTab* _bottom, XTab* left,
	XTab* right)
{
	Area* currentArea = _CurrentArea();
	if (!left)
		left = currentArea->Left();
	if (!right)
		right = currentArea->Right();
	YTab* top = currentArea->Bottom();
	BReference<YTab> bottom = _bottom;
	if (_bottom == NULL)
		bottom = fLayout->AddYTab();

	return Add(view, left, top, right, bottom);
}


BALMLayoutBuilder&
BALMLayoutBuilder::AddToLeft(BLayoutItem* item, XTab* _left, YTab* top,
	YTab* bottom)
{
	Area* currentArea = _CurrentArea();
	BReference<XTab> left = _left;
	if (_left == NULL)
		left = fLayout->AddXTab();
	XTab* right = currentArea->Left();
	if (!top)
		top = currentArea->Top();
	if (!bottom)
		bottom = currentArea->Bottom();

	return Add(item, left, top, right, bottom);
}


BALMLayoutBuilder&
BALMLayoutBuilder::AddToRight(BLayoutItem* item, XTab* _right, YTab* top,
	YTab* bottom)
{
	Area* currentArea = _CurrentArea();
	XTab* left = currentArea->Right();
	BReference<XTab> right = _right;
	if (_right == NULL)
		right = fLayout->AddXTab();
	if (!top)
		top = currentArea->Top();
	if (!bottom)
		bottom = currentArea->Bottom();

	return Add(item, left, top, right, bottom);
}


BALMLayoutBuilder&
BALMLayoutBuilder::AddAbove(BLayoutItem* item, YTab* _top, XTab* left,
	XTab* right)
{
	Area* currentArea = _CurrentArea();
	if (!left)
		left = currentArea->Left();
	if (!right)
		right = currentArea->Right();
	BReference<YTab> top = _top;
	if (_top == NULL)
		top = fLayout->AddYTab();
	YTab* bottom = currentArea->Top();

	return Add(item, left, top, right, bottom);
}


BALMLayoutBuilder&
BALMLayoutBuilder::AddBelow(BLayoutItem* item, YTab* _bottom, XTab* left,
	XTab* right)
{
	Area* currentArea = _CurrentArea();
	if (!left)
		left = currentArea->Left();
	if (!right)
		right = currentArea->Right();
	YTab* top = currentArea->Bottom();
	BReference<YTab> bottom = _bottom;
	if (_bottom == NULL)
		bottom = fLayout->AddYTab();

	return Add(item, left, top, right, bottom);
}


BALMLayoutBuilder&
BALMLayoutBuilder::Push()
{
	fAreaStack.AddItem(_CurrentArea());
	return *this;
}


BALMLayoutBuilder&
BALMLayoutBuilder::Pop()
{
	if (fAreaStack.CountItems() > 0)
		fAreaStack.RemoveItemAt(fAreaStack.CountItems() - 1);
	return *this;
}


void
BALMLayoutBuilder::_SetCurrentArea(Area* area)
{
	if (fAreaStack.CountItems() > 0)
		fAreaStack.ReplaceItem(fAreaStack.CountItems() - 1, area);
	else
		fAreaStack.AddItem(area);
}


Area*
BALMLayoutBuilder::_CurrentArea() const
{
	return fAreaStack.ItemAt(fAreaStack.CountItems() - 1);
}


};
