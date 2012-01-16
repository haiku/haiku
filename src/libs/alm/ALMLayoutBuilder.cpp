/*
 * Copyright 2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "ALMLayoutBuilder.h"


#include <Window.h>


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
	fLayout->AddView(view, left, top, right, bottom);
	return *this;
}


BALMLayoutBuilder&
BALMLayoutBuilder::Add(BView* view, Row* row, Column* column)
{
	fLayout->AddView(view, row, column);
	return *this;
}


BALMLayoutBuilder&
BALMLayoutBuilder::Add(BLayoutItem* item, XTab* left, YTab* top,
	XTab* right, YTab* bottom)
{
	fLayout->AddItem(item, left, top, right, bottom);
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


BALMLayoutBuilder::Snake
BALMLayoutBuilder::StartingAt(BView* view)
{
	return Snake(fLayout->AreaFor(view), this);
}


BALMLayoutBuilder::Snake
BALMLayoutBuilder::StartingAt(BLayoutItem* item)
{
	return Snake(fLayout->AreaFor(item), this);
}


BALMLayoutBuilder::Snake::Snake(Area* area, BALMLayoutBuilder* root)
{
	fCurrentArea = area;
	fRootBuilder = root;
	fPreviousSnake = NULL;
}


BALMLayoutBuilder::Snake::Snake(Area* area, Snake* previous)
{
	fCurrentArea = area;
	fRootBuilder = previous->fRootBuilder;
	fPreviousSnake = previous;
}


BALMLayoutBuilder::Snake&
BALMLayoutBuilder::Snake::AddToLeft(BView* view, XTab* _left, YTab* top,
	YTab* bottom)
{
	BReference<XTab> left = _left;
	if (_left == NULL)
		left = _Layout()->AddXTab();
	XTab* right = fCurrentArea->Left();
	if (!top)
		top = fCurrentArea->Top();
	if (!bottom)
		bottom = fCurrentArea->Bottom();

	fCurrentArea = _AddToLayout(view, left, top, right, bottom);
	return *this;
}


BALMLayoutBuilder::Snake&
BALMLayoutBuilder::Snake::AddToRight(BView* view, XTab* _right, YTab* top,
	YTab* bottom)
{
	XTab* left = fCurrentArea->Right();
	BReference<XTab> right = _right;
	if (_right == NULL)
		right = _Layout()->AddXTab();
	if (!top)
		top = fCurrentArea->Top();
	if (!bottom)
		bottom = fCurrentArea->Bottom();

	fCurrentArea = _AddToLayout(view, left, top, right, bottom);
	return *this;
}


BALMLayoutBuilder::Snake&
BALMLayoutBuilder::Snake::AddAbove(BView* view, YTab* _top, XTab* left,
	XTab* right)
{
	if (!left)
		left = fCurrentArea->Left();
	if (!right)
		right = fCurrentArea->Right();
	BReference<YTab> top = _top;
	if (_top == NULL)
		top = _Layout()->AddYTab();
	YTab* bottom = fCurrentArea->Top();

	fCurrentArea = _AddToLayout(view, left, top, right, bottom);
	return *this;
}


BALMLayoutBuilder::Snake&
BALMLayoutBuilder::Snake::AddBelow(BView* view, YTab* _bottom, XTab* left,
	XTab* right)
{
	if (!left)
		left = fCurrentArea->Left();
	if (!right)
		right = fCurrentArea->Right();
	YTab* top = fCurrentArea->Bottom();
	BReference<YTab> bottom = _bottom;
	if (_bottom == NULL)
		bottom = _Layout()->AddYTab();

	fCurrentArea = _AddToLayout(view, left, top, right, bottom);
	return *this;
}


BALMLayoutBuilder::Snake&
BALMLayoutBuilder::Snake::AddToLeft(BLayoutItem* item, XTab* _left, YTab* top,
	YTab* bottom)
{
	BReference<XTab> left = _left;
	if (_left == NULL)
		left = _Layout()->AddXTab();
	XTab* right = fCurrentArea->Left();
	if (!top)
		top = fCurrentArea->Top();
	if (!bottom)
		bottom = fCurrentArea->Bottom();

	fCurrentArea = _AddToLayout(item, left, top, right, bottom);
	return *this;
}


BALMLayoutBuilder::Snake&
BALMLayoutBuilder::Snake::AddToRight(BLayoutItem* item, XTab* _right, YTab* top,
	YTab* bottom)
{
	XTab* left = fCurrentArea->Right();
	BReference<XTab> right = _right;
	if (_right == NULL)
		right = _Layout()->AddXTab();
	if (!top)
		top = fCurrentArea->Top();
	if (!bottom)
		bottom = fCurrentArea->Bottom();

	fCurrentArea = _AddToLayout(item, left, top, right, bottom);
	return *this;
}


BALMLayoutBuilder::Snake&
BALMLayoutBuilder::Snake::AddAbove(BLayoutItem* item, YTab* _top, XTab* left,
	XTab* right)
{
	if (!left)
		left = fCurrentArea->Left();
	if (!right)
		right = fCurrentArea->Right();
	BReference<YTab> top = _top;
	if (_top == NULL)
		top = _Layout()->AddYTab();
	YTab* bottom = fCurrentArea->Top();

	fCurrentArea = _AddToLayout(item, left, top, right, bottom);
	return *this;
}


BALMLayoutBuilder::Snake&
BALMLayoutBuilder::Snake::AddBelow(BLayoutItem* item, YTab* _bottom, XTab* left,
	XTab* right)
{
	if (!left)
		left = fCurrentArea->Left();
	if (!right)
		right = fCurrentArea->Right();
	YTab* top = fCurrentArea->Bottom();
	BReference<YTab> bottom = _bottom;
	if (_bottom == NULL)
		bottom = _Layout()->AddYTab();

	fCurrentArea = _AddToLayout(item, left, top, right, bottom);
	return *this;
}


BALMLayoutBuilder::Snake
BALMLayoutBuilder::Snake::StartingAt(BView* view)
{
	return fRootBuilder->StartingAt(view);
}


BALMLayoutBuilder::Snake
BALMLayoutBuilder::Snake::StartingAt(BLayoutItem* item)
{
	return fRootBuilder->StartingAt(item);
}


BALMLayoutBuilder::Snake
BALMLayoutBuilder::Snake::Push()
{
	return Snake(fCurrentArea, this);
}


BALMLayoutBuilder::Snake&
BALMLayoutBuilder::Snake::Pop()
{
	return *fPreviousSnake;
}


BALMLayoutBuilder&
BALMLayoutBuilder::Snake::End()
{
	return *fRootBuilder;
}


BALMLayout*
BALMLayoutBuilder::Snake::_Layout()
{
	return fRootBuilder->fLayout;
}


Area*
BALMLayoutBuilder::Snake::_AddToLayout(BView* view, XTab* left, YTab* top,
	XTab* right, YTab* bottom)
{
	return _Layout()->AddView(view, left, top, right, bottom);
}


Area*
BALMLayoutBuilder::Snake::_AddToLayout(BLayoutItem* item, XTab* left,
	YTab* top, XTab* right, YTab* bottom)
{
	return _Layout()->AddItem(item, left, top, right, bottom);
}


};
