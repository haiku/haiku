/*
 * Copyright 2006-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "ShapeListView.h"

#include <new>
#include <stdio.h>

#include <Application.h>
#include <Catalog.h>
#include <ListItem.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Message.h>
#include <Mime.h>
#include <Window.h>

#include "AddPathsCommand.h"
#include "AddShapesCommand.h"
#include "AddStylesCommand.h"
#include "CommandStack.h"
#include "FreezeTransformationCommand.h"
#include "MoveShapesCommand.h"
#include "Observer.h"
#include "RemoveShapesCommand.h"
#include "ResetTransformationCommand.h"
#include "Selection.h"
#include "Shape.h"
#include "Util.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-ShapesList"


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
	MSG_REMOVE						= 'rmsh',
	MSG_DUPLICATE					= 'dpsh',
	MSG_RESET_TRANSFORMATION		= 'rstr',
	MSG_FREEZE_TRANSFORMATION		= 'frzt',

	MSG_DRAG_SHAPE					= 'drgs',
};

// constructor
ShapeListView::ShapeListView(BRect frame,
							 const char* name,
							 BMessage* message, BHandler* target)
	: SimpleListView(frame, name,
					 NULL, B_MULTIPLE_SELECTION_LIST),
	  fMessage(message),
	  fShapeContainer(NULL),
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
	SimpleListView::SelectionChanged();

	if (!fSyncingToSelection) {
		ShapeListItem* item
			= dynamic_cast<ShapeListItem*>(ItemAt(CurrentSelection(0)));
		if (fMessage) {
			BMessage message(*fMessage);
			message.AddPointer("shape", item ? (void*)item->shape : NULL);
			Invoke(&message);
		}
	}

	_UpdateMenu();
}

// MessageReceived
void
ShapeListView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_REMOVE:
			RemoveSelected();
			break;
		case MSG_DUPLICATE: {
			int32 count = CountSelectedItems();
			int32 index = 0;
			BList items;
			for (int32 i = 0; i < count; i++) {
				index = CurrentSelection(i);
				BListItem* item = ItemAt(index);
				if (item)
					items.AddItem((void*)item);
			}
			CopyItems(items, index + 1);
			break;
		}
		case MSG_RESET_TRANSFORMATION: {
			BList shapes;
			_GetSelectedShapes(shapes);
			int32 count = shapes.CountItems();
			if (count < 0)
				break;

			Transformable* transformables[count];
			for (int32 i = 0; i < count; i++) {
				Shape* shape = (Shape*)shapes.ItemAtFast(i);
				transformables[i] = shape;
			}

			ResetTransformationCommand* command = 
				new ResetTransformationCommand(transformables, count);

			fCommandStack->Perform(command);
			break;
		}
		case MSG_FREEZE_TRANSFORMATION: {
			BList shapes;
			_GetSelectedShapes(shapes);
			int32 count = shapes.CountItems();
			if (count < 0)
				break;

			FreezeTransformationCommand* command = 
				new FreezeTransformationCommand((Shape**)shapes.Items(),
					count);

			fCommandStack->Perform(command);
			break;
		}
		default:
			SimpleListView::MessageReceived(message);
			break;
	}
}

// MakeDragMessage
void
ShapeListView::MakeDragMessage(BMessage* message) const
{
	SimpleListView::MakeDragMessage(message);
	message->AddPointer("container", fShapeContainer);
	int32 count = CountSelectedItems();
	for (int32 i = 0; i < count; i++) {
		ShapeListItem* item = dynamic_cast<ShapeListItem*>(
			ItemAt(CurrentSelection(i)));
		if (item)
			message->AddPointer("shape", (void*)item->shape);
		else
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
	if (!fCommandStack || !fShapeContainer)
		return;

	int32 count = items.CountItems();
	Shape* shapes[count];

	for (int32 i = 0; i < count; i++) {
		ShapeListItem* item
			= dynamic_cast<ShapeListItem*>((BListItem*)items.ItemAtFast(i));
		shapes[i] = item ? new (nothrow) Shape(*item->shape) : NULL;
	}

	AddShapesCommand* command
		= new (nothrow) AddShapesCommand(fShapeContainer,
										 shapes, count, toIndex,
										 fSelection);
	if (!command) {
		for (int32 i = 0; i < count; i++)
			delete shapes[i];
		return;
	}

	fCommandStack->Perform(command);
}

// RemoveItemList
void
ShapeListView::RemoveItemList(BList& items)
{
	if (!fCommandStack || !fShapeContainer)
		return;

	int32 count = items.CountItems();
	int32 indices[count];
	for (int32 i = 0; i < count; i++)
	 	indices[i] = IndexOf((BListItem*)items.ItemAtFast(i));

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

// IndexOfSelectable
int32
ShapeListView::IndexOfSelectable(Selectable* selectable) const
{
	Shape* shape = dynamic_cast<Shape*>(selectable);
	if (!shape) {
		Transformer* transformer = dynamic_cast<Transformer*>(selectable);
		if (!transformer)
			return -1;
		for (int32 i = 0;
			 ShapeListItem* item = dynamic_cast<ShapeListItem*>(ItemAt(i));
			 i++) {
			if (item->shape->HasTransformer(transformer))
				return i;
		}
	} else {
		for (int32 i = 0;
			 ShapeListItem* item = dynamic_cast<ShapeListItem*>(ItemAt(i));
			 i++) {
			if (item->shape == shape)
				return i;
		}
	}

	return -1;
}

// SelectableFor
Selectable*
ShapeListView::SelectableFor(BListItem* item) const
{
	ShapeListItem* shapeItem = dynamic_cast<ShapeListItem*>(item);
	if (shapeItem)
		return shapeItem->shape;
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

	if (_AddShape(shape, index))
		Select(index);

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

// SetMenu
void
ShapeListView::SetMenu(BMenu* menu)
{
	if (fMenu == menu)
		return;

	fMenu = menu;
	if (fMenu == NULL)
		return;

	BMessage* message = new BMessage(MSG_ADD_SHAPE);
	fAddEmptyMI = new BMenuItem(B_TRANSLATE("Add empty"), message);

	message = new BMessage(MSG_ADD_SHAPE);
	message->AddBool("path", true);
	fAddWidthPathMI = new BMenuItem(B_TRANSLATE("Add with path"), message);

	message = new BMessage(MSG_ADD_SHAPE);
	message->AddBool("style", true);
	fAddWidthStyleMI = new BMenuItem(B_TRANSLATE("Add with style"), message);

	message = new BMessage(MSG_ADD_SHAPE);
	message->AddBool("path", true);
	message->AddBool("style", true);
	fAddWidthPathAndStyleMI = new BMenuItem(
		B_TRANSLATE("Add with path & style"), message);

	fDuplicateMI = new BMenuItem(B_TRANSLATE("Duplicate"), 
		new BMessage(MSG_DUPLICATE));
	fResetTransformationMI = new BMenuItem(B_TRANSLATE("Reset transformation"),
		new BMessage(MSG_RESET_TRANSFORMATION));
	fFreezeTransformationMI = new BMenuItem(
		B_TRANSLATE("Freeze transformation"), 
		new BMessage(MSG_FREEZE_TRANSFORMATION));

	fRemoveMI = new BMenuItem(B_TRANSLATE("Remove"), new BMessage(MSG_REMOVE));


	fMenu->AddItem(fAddEmptyMI);
	fMenu->AddItem(fAddWidthPathMI);
	fMenu->AddItem(fAddWidthStyleMI);
	fMenu->AddItem(fAddWidthPathAndStyleMI);

	fMenu->AddSeparatorItem();

	fMenu->AddItem(fDuplicateMI);
	fMenu->AddItem(fResetTransformationMI);
	fMenu->AddItem(fFreezeTransformationMI);

	fMenu->AddSeparatorItem();

	fMenu->AddItem(fRemoveMI);

	fDuplicateMI->SetTarget(this);
	fResetTransformationMI->SetTarget(this);
	fFreezeTransformationMI->SetTarget(this);
	fRemoveMI->SetTarget(this);

	_UpdateMenu();
}

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
	int32 count = fShapeContainer->CountShapes();
	for (int32 i = 0; i < count; i++)
		_AddShape(fShapeContainer->ShapeAtFast(i), i);
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

// _UpdateMenu
void
ShapeListView::_UpdateMenu()
{
	if (!fMenu)
		return;

	bool gotSelection = CurrentSelection(0) >= 0;

	fDuplicateMI->SetEnabled(gotSelection);
	fResetTransformationMI->SetEnabled(gotSelection);
	fFreezeTransformationMI->SetEnabled(gotSelection);
	fRemoveMI->SetEnabled(gotSelection);
}

// _GetSelectedShapes
void
ShapeListView::_GetSelectedShapes(BList& shapes) const
{
	int32 count = CountSelectedItems();
	for (int32 i = 0; i < count; i++) {
		ShapeListItem* item = dynamic_cast<ShapeListItem*>(
			ItemAt(CurrentSelection(i)));
		if (item && item->shape) {
			if (!shapes.AddItem((void*)item->shape))
				break;
		}
	}
}
