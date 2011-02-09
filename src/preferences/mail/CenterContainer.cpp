/* CenterContainer - a container which centers its contents in the middle
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include "CenterContainer.h"

#include <ObjectList.h>


CenterContainer::CenterContainer(BRect rect,bool centerHoriz)
	: BView(rect,NULL,B_FOLLOW_ALL,0),
	fSpacing(7),
	fWidth(0),
	fCenterHoriz(centerHoriz)
{
}


void CenterContainer::AttachedToWindow()
{
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
}


void CenterContainer::AllAttached()
{
	Layout();
}


void CenterContainer::FrameResized(float width,float height)
{
	Layout();
}


void CenterContainer::GetPreferredSize(float *width, float *height)
{
	// calculate dimensions (and, well, layout views)
	if (fWidth == 0)
		Layout();

	if (width)
		*width = fWidth;
	if (height)
		*height = fHeight;
}


void CenterContainer::Layout()
{
	// compute the size of all views
	fHeight = 0;  fWidth = 0;
	for (int32 i = 0;BView *view = ChildAt(i);i++)
	{
		if (i != 0)		// the spacing between to items
			fHeight += fSpacing;
		fHeight += view->Bounds().Height();

		if (view->Bounds().Width() > fWidth)
			fWidth = view->Bounds().Width();
	}

	// layout views
	float y = (Bounds().Height() - fHeight) / 2;
	for (int32 i = 0;BView *view = ChildAt(i);i++)
	{
		view->MoveTo(fCenterHoriz ? (Bounds().Width() - view->Bounds().Width()) / 2
								  : view->Frame().left,
					 y);
		y += view->Bounds().Height() + fSpacing;
	}
}


void CenterContainer::SetSpacing(float spacing)
{
	if (fSpacing == spacing)
		return;

	fSpacing = spacing;
	Layout();
}


void CenterContainer::DeleteChildren()
{
	// there happens some magic in DetachedFromWindow and therefore all views
	// have to exist so delete them later...
	// TODO: maybe rethink this
	BObjectList<BView> temp;
	// remove all child views
	for (int32 i = CountChildren();i-- > 0;)
	{
		BView *view = ChildAt(i);
		temp.AddItem(view);
		RemoveChild(view);
	}
	for (int32 i = 0; i < temp.CountItems(); i++)
		delete temp.ItemAt(i);
}

