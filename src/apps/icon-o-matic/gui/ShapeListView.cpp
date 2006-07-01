/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "ShapeListView.h"

#include <new>
#include <stdio.h>

#include <Application.h>
#include <ListItem.h>
#include <Message.h>
#include <Mime.h>
#include <Window.h>

#include "CommandStack.h"
#include "MoveShapesCommand.h"
#include "RemoveShapesCommand.h"
#include "Shape.h"
#include "Observer.h"
#include "Selection.h"

using std::nothrow;

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
	  fSelection(NULL),
	  fCommandStack(NULL)
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

// #pragma mark -

// MoveItems
void
ShapeListView::MoveItems(BList& items, int32 toIndex)
{
	if (!fCommandStack || !fShapeContainer)
		return;

	int32 count = items.CountItems();
	Shape** shapes = new (nothrow) Shape*[count];
	if (!shapes)
		return;

	for (int32 i = 0; i < count; i++) {
		ShapeListItem* item
			= dynamic_cast<ShapeListItem*>((BListItem*)items.ItemAtFast(i));
		shapes[i] = item ? item->shape : NULL;
	}

	MoveShapesCommand* command
		= new (nothrow) MoveShapesCommand(fShapeContainer,
										  shapes, count, toIndex);
	if (!command) {
		delete[] shapes;
		return;
	}

	fCommandStack->Perform(command);
}

// CopyItems
void
ShapeListView::CopyItems(BList& items, int32 toIndex)
{
	MoveItems(items, toIndex);
	// TODO: allow copying items
}

// RemoveItemList
void
ShapeListView::RemoveItemList(BList& indexList)
{
	if (!fCommandStack || !fShapeContainer)
		return;

	int32 count = indexList.CountItems();
	const int32* indices = (int32*)indexList.Items();

	RemoveShapesCommand* command
		= new (nothrow) RemoveShapesCommand(fShapeContainer,
											indices, count);
	fCommandStack->Perform(command);
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
ShapeListView::ShapeAdded(Shape* shape, int32 index)
{
	// NOTE: we are in the thread that messed with the
	// ShapeContainer, so no need to lock the
	// container, when this is changed to asynchronous
	// notifications, then it would need to be read-locked!
	if (!LockLooper())
		return;

	// NOTE: shapes are always added at the end
	// of the list, so the sorting is synced...
	_AddShape(shape, index);

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
		_AddShape(fShapeContainer->ShapeAtFast(i), i);

//	fShapeContainer->ReadUnlock();
}

// SetSelection
void
ShapeListView::SetSelection(Selection* selection)
{
	fSelection = selection;
}

// SetCommandStack
void
ShapeListView::SetCommandStack(CommandStack* stack)
{
	fCommandStack = stack;
}

// #pragma mark -

// _AddShape
bool
ShapeListView::_AddShape(Shape* shape, int32 index)
{
	if (shape)
		 return AddItem(new ShapeListItem(shape, this), index);
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

