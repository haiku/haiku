/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "ShapeListView.h"

#include <stdio.h>

#include <Application.h>
#include <ListItem.h>
#include <Message.h>
#include <Mime.h>
#include <Window.h>

#include "Shape.h"
#include "Observer.h"
#include "Selection.h"

class ShapeListItem : public SimpleItem,
					  public Observer {
 public:
					ShapeListItem(Shape* s,
								  ShapeListView* listView)
						: SimpleItem(""),
						  shape(NULL),
						  fListView(listView)
					{
						SetClip(s);
					}

	virtual			~ShapeListItem()
					{
						SetClip(NULL);
					}

	virtual	void	ObjectChanged(const Observable* object)
					{
						UpdateText();
					}

			void	SetClip(Shape* s)
					{
						if (s == shape)
							return;

						if (shape) {
							shape->RemoveObserver(this);
							shape->Release();
						}

						shape = s;

						if (shape) {
							shape->Acquire();
							shape->AddObserver(this);
							UpdateText();
						}
					}
			void	UpdateText()
					{
						SetText(shape->Name());
						// :-/
						if (fListView->LockLooper()) {
							fListView->InvalidateItem(
								fListView->IndexOf(this));
							fListView->UnlockLooper();
						}
					}

	Shape* 			shape;
 private:
	ShapeListView*	fListView;
};

// #pragma mark -

enum {
	MSG_DRAG_SHAPE = 'drgs',
};

// constructor
ShapeListView::ShapeListView(BRect frame,
							 const char* name,
							 BMessage* message, BHandler* target)
	: SimpleListView(frame, name,
					 NULL, B_MULTIPLE_SELECTION_LIST),
	  fMessage(message),
	  fShapeContainer(NULL),
	  fSelection(NULL)
{
	SetDragCommand(MSG_DRAG_SHAPE);
	SetTarget(target);
}

// destructor
ShapeListView::~ShapeListView()
{
	_MakeEmpty();
	delete fMessage;

	if (fShapeContainer)
		fShapeContainer->RemoveListener(this);
}

// SelectionChanged
void
ShapeListView::SelectionChanged()
{
	// TODO: single selection versus multiple selection

	ShapeListItem* item = dynamic_cast<ShapeListItem*>(ItemAt(CurrentSelection(0)));
	if (item && fMessage) {
		BMessage message(*fMessage);
		message.AddPointer("shape", (void*)item->shape);
		Invoke(&message);
	}

	// modify global Selection
	if (!fSelection)
		return;

	if (item)
		fSelection->Select(item->shape);
	else
		fSelection->DeselectAll();
}

// MessageReceived
void
ShapeListView::MessageReceived(BMessage* message)
{
	SimpleListView::MessageReceived(message);
}

// MakeDragMessage
void
ShapeListView::MakeDragMessage(BMessage* message) const
{
	SimpleListView::MakeDragMessage(message);
	message->AddPointer("container", fShapeContainer);
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		ShapeListItem* item = dynamic_cast<ShapeListItem*>(ItemAt(CurrentSelection(i)));
		if (item) {
			message->AddPointer("shape", (void*)item->shape);
		} else
			break;
	}

//		message->AddInt32("be:actions", B_COPY_TARGET);
//		message->AddInt32("be:actions", B_TRASH_TARGET);
//
//		message->AddString("be:types", B_FILE_MIME_TYPE);
////		message->AddString("be:filetypes", "");
////		message->AddString("be:type_descriptions", "");
//
//		message->AddString("be:clip_name", item->shape->Name());
//
//		message->AddString("be:originator", "Icon-O-Matic");
//		message->AddPointer("be:originator_data", (void*)item->shape);
}

// AcceptDragMessage
bool
ShapeListView::AcceptDragMessage(const BMessage* message) const
{
	return SimpleListView::AcceptDragMessage(message);
}

// SetDropTargetRect
void
ShapeListView::SetDropTargetRect(const BMessage* message, BPoint where)
{
	SimpleListView::SetDropTargetRect(message, where);
}

// MoveItems
void
ShapeListView::MoveItems(BList& items, int32 toIndex)
{
	SimpleListView::MoveItems(items, toIndex);
}

// CopyItems
void
ShapeListView::CopyItems(BList& items, int32 toIndex)
{
	MoveItems(items, toIndex);
	// copy operation not allowed -> ?!?
	// TODO: what about clips that reference the same file but
	// with different "in/out points"
}

// RemoveItemList
void
ShapeListView::RemoveItemList(BList& indices)
{
	// TODO: allow removing items
}

// CloneItem
BListItem*
ShapeListView::CloneItem(int32 index) const
{
	if (ShapeListItem* item = dynamic_cast<ShapeListItem*>(ItemAt(index))) {
		return new ShapeListItem(item->shape,
								 const_cast<ShapeListView*>(this));
	}
	return NULL;
}

// #pragma mark -

// ShapeAdded
void
ShapeListView::ShapeAdded(Shape* shape)
{
	// NOTE: we are in the thread that messed with the
	// ShapeContainer, so no need to lock the
	// container, when this is changed to asynchronous
	// notifications, then it would need to be read-locked!
	if (!LockLooper())
		return;

	// NOTE: shapes are always added at the end
	// of the list, so the sorting is synced...
	_AddShape(shape);

	UnlockLooper();
}

// ShapeRemoved
void
ShapeListView::ShapeRemoved(Shape* shape)
{
	// NOTE: we are in the thread that messed with the
	// ShapeContainer, so no need to lock the
	// container, when this is changed to asynchronous
	// notifications, then it would need to be read-locked!
	if (!LockLooper())
		return;

	// NOTE: we're only interested in Shape objects
	_RemoveShape(shape);

	UnlockLooper();
}

// #pragma mark -

// SetShapeContainer
void
ShapeListView::SetShapeContainer(ShapeContainer* container)
{
	if (fShapeContainer == container)
		return;

	// detach from old container
	if (fShapeContainer)
		fShapeContainer->RemoveListener(this);

	_MakeEmpty();

	fShapeContainer = container;

	if (!fShapeContainer)
		return;

	fShapeContainer->AddListener(this);

	// sync
//	if (!fShapeContainer->ReadLock())
//		return;

	int32 count = fShapeContainer->CountShapes();
	for (int32 i = 0; i < count; i++)
		_AddShape(fShapeContainer->ShapeAtFast(i));

//	fShapeContainer->ReadUnlock();
}

// SetSelection
void
ShapeListView::SetSelection(Selection* selection)
{
	fSelection = selection;
}

// #pragma mark -

// _AddShape
bool
ShapeListView::_AddShape(Shape* shape)
{
	if (shape)
		 return AddItem(new ShapeListItem(shape, this));
	return false;
}

// _RemoveShape
bool
ShapeListView::_RemoveShape(Shape* shape)
{
	ShapeListItem* item = _ItemForShape(shape);
	if (item && RemoveItem(item)) {
		delete item;
		return true;
	}
	return false;
}

// _ItemForShape
ShapeListItem*
ShapeListView::_ItemForShape(Shape* shape) const
{
	for (int32 i = 0;
		 ShapeListItem* item = dynamic_cast<ShapeListItem*>(ItemAt(i));
		 i++) {
		if (item->shape == shape)
			return item;
	}
	return NULL;
}

// _MakeEmpty
void
ShapeListView::_MakeEmpty()
{
	// NOTE: BListView::MakeEmpty() uses ScrollTo()
	// for which the object needs to be attached to
	// a BWindow.... :-(
	int32 count = CountItems();
	for (int32 i = count - 1; i >= 0; i--)
		delete RemoveItem(i);
}

