/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "View.h"

#include <stdio.h>

#include <Region.h>
#include <View.h>


View::View()
	: fFrame(0, 0, 0, 0),
	  fContainer(NULL),
	  fParent(NULL),
	  fChildren(),
	  fViewColor(B_TRANSPARENT_32_BIT),
	  fLayoutValid(false)
{
}


View::View(BRect frame)
	: fFrame(frame),
	  fContainer(NULL),
	  fParent(NULL),
	  fChildren(),
	  fViewColor(B_TRANSPARENT_32_BIT),
	  fLayoutValid(false)
{
}


View::~View()
{
	// delete children
	for (int32 i = CountChildren() - 1; i >= 0; i--)
		delete RemoveChild(i);
}


void
View::SetFrame(BRect frame)
{
	if (frame != fFrame && frame.IsValid()) {
		BRect oldFrame(frame);
		Invalidate();
		fFrame = frame;
		Invalidate();

		FrameChanged(oldFrame, frame);
	}

	// relayout if necessary
	if (!fLayoutValid) {
		Layout();
		fLayoutValid = true;
	}
}


BRect
View::Frame() const
{
	return fFrame;
}


BRect
View::Bounds() const
{
	return BRect(fFrame).OffsetToCopy(B_ORIGIN);
}


void
View::SetLocation(BPoint location)
{
	SetFrame(fFrame.OffsetToCopy(location));
}


BPoint
View::Location() const
{
	return fFrame.LeftTop();
}


void
View::SetSize(BSize size)
{
	BRect frame(fFrame);
	frame.right = frame.left + size.width;
	frame.bottom = frame.top + size.height;
	SetFrame(frame);
}


BSize
View::Size() const
{
	return Frame().Size();
}


BSize
View::MinSize()
{
	return BSize(-1, -1);
}


BSize
View::MaxSize()
{
	return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
}


BSize
View::PreferredSize()
{
	return MinSize();
}


BAlignment
View::Alignment()
{
	return BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_VERTICAL_CENTER);
}


BPoint
View::LocationInContainer() const
{
	BPoint location = fFrame.LeftTop();
	return (fParent ? fParent->LocationInContainer() + location : location);
}


BRect
View::FrameInContainer() const
{
	BRect frame(fFrame);
	return frame.OffsetToCopy(LocationInContainer());
}


BPoint
View::ConvertFromContainer(BPoint point) const
{
	return point - LocationInContainer();
}


BRect
View::ConvertFromContainer(BRect rect) const
{
	return rect.OffsetBySelf(-LocationInContainer());
}


BPoint
View::ConvertToContainer(BPoint point) const
{
	return point + LocationInContainer();
}


BRect
View::ConvertToContainer(BRect rect) const
{
	return rect.OffsetBySelf(LocationInContainer());
}


View*
View::Parent() const
{
	return fParent;
}


BView*
View::Container() const
{
	return fContainer;
}


bool
View::AddChild(View* child)
{
	if (!child)
		return false;

	if (child->Parent() || child->Container()) {
		fprintf(stderr, "View::AddChild(): view %p already has a parent "
			"or is the container view\n", child);
		return false;
	}

	if (!fChildren.AddItem(child))
		return false;

	child->_AddedToParent(this);

	child->Invalidate();
	InvalidateLayout();

	return true;
}


bool
View::RemoveChild(View* child)
{
	if (!child)
		return false;

	return RemoveChild(IndexOfChild(child));
}


View*
View::RemoveChild(int32 index)
{
	if (index < 0 || index >= fChildren.CountItems())
		return NULL;

	View* child = ChildAt(index);
	child->Invalidate();
	child->_RemovingFromParent();
	fChildren.RemoveItem(index);

	InvalidateLayout();

	return child;
}


int32
View::CountChildren() const
{
	return fChildren.CountItems();
}


View*
View::ChildAt(int32 index) const
{
	return (View*)fChildren.ItemAt(index);
}


View*
View::ChildAt(BPoint point) const
{
	for (int32 i = 0; View* child = ChildAt(i); i++) {
		if (child->Frame().Contains(point))
			return child;
	}

	return NULL;
}


View*
View::AncestorAt(BPoint point, BPoint* localPoint) const
{
	if (!Bounds().Contains(point))
		return NULL;

	View* view = const_cast<View*>(this);

	// Iterate deeper down the hierarchy, until we reach a view that
	// doesn't have a child at the location.
	while (true) {
		View* child = view->ChildAt(point);
		if (!child) {
			if (localPoint)
				*localPoint = point;
			return view;
		}

		view = child;
		point -= view->Frame().LeftTop();
	}
}


int32
View::IndexOfChild(View* child) const
{
	return (child ? fChildren.IndexOf(child) : -1);
}


void
View::Invalidate(BRect rect)
{
	if (fContainer) {
		rect = rect & Bounds();
		fContainer->Invalidate(rect.OffsetByCopy(LocationInContainer()));
	}
}


void
View::Invalidate()
{
	Invalidate(Bounds());
}


void
View::InvalidateLayout()
{
//printf("%p->View::InvalidateLayout(): %d\n", this, fLayoutValid);
	if (fLayoutValid) {
		fLayoutValid = false;
		if (fParent)
			fParent->InvalidateLayout();
	}
}


bool
View::IsLayoutValid() const
{
	return fLayoutValid;
}


void
View::SetViewColor(rgb_color color)
{
	fViewColor = color;
}


void
View::Draw(BView* container, BRect updateRect)
{
}


void
View::MouseDown(BPoint where, uint32 buttons, int32 modifiers)
{
}


void
View::MouseUp(BPoint where, uint32 buttons, int32 modifiers)
{
}


void
View::MouseMoved(BPoint where, uint32 buttons, int32 modifiers)
{
}


void
View::AddedToContainer()
{
}


void
View::RemovingFromContainer()
{
}


void
View::FrameChanged(BRect oldFrame, BRect newFrame)
{
}


void
View::Layout()
{
	// simply trigger relayouting the children
	for (int32 i = 0; View* child = ChildAt(i); i++)
		child->SetFrame(child->Frame());
}


void
View::_AddedToParent(View* parent)
{
	fParent = parent;

	if (parent->Container()) {
		Invalidate();
		_AddedToContainer(parent->Container());
	}
}


void
View::_RemovingFromParent()
{
	if (fContainer)
		_RemovingFromContainer();

	fParent = NULL;
}


void
View::_AddedToContainer(BView* container)
{
	fContainer = container;

	AddedToContainer();

	for (int32 i = 0; View* child = ChildAt(i); i++)
		child->_AddedToContainer(fContainer);
}


void
View::_RemovingFromContainer()
{
	for (int32 i = 0; View* child = ChildAt(i); i++)
		child->_RemovingFromContainer();

	RemovingFromContainer();

	fContainer = NULL;
}


void
View::_Draw(BView* container, BRect updateRect)
{
	// compute the clipping region
	BRegion region(Bounds());
	for (int32 i = 0; View* child = ChildAt(i); i++)
		region.Exclude(child->Frame());

	if (region.Frame().IsValid()) {
		// set the clipping region
		container->ConstrainClippingRegion(&region);

		// draw the background, if it isn't transparent
		if (fViewColor.alpha != 0) {
			container->SetLowColor(fViewColor);
			container->FillRect(updateRect, B_SOLID_LOW);
		}

		// draw this view
		Draw(container, updateRect);

		// revert the clipping region
		region.Set(Bounds());
		container->ConstrainClippingRegion(&region);
	}

	// draw the children
	if (CountChildren() > 0) {
		container->PushState();

		for (int32 i = 0; View* child = ChildAt(i); i++) {
			BRect childFrame = child->Frame();
			BRect childUpdateRect = updateRect & childFrame;
			if (childUpdateRect.IsValid()) {
				// set origin
				childUpdateRect.OffsetBy(-childFrame.LeftTop());
				container->SetOrigin(childFrame.LeftTop());

				// draw
				child->_Draw(container, childUpdateRect);
			}
		}

		container->PopState();
	}
}
