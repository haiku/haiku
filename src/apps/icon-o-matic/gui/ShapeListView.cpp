/*
 * Copyright 2006-2012, Haiku, Inc. All rights reserved.
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

class ShapeListItem : public SimpleItem, public Observer {
public:
	ShapeListItem(Shape* s, ShapeListView* listView)
		:
		SimpleItem(""),
		shape(NULL),
		fListView(listView)
	{
		SetShape(s);
	}


	virtual	~ShapeListItem()
	{
		SetShape(NULL);
	}


	virtual	void ObjectChanged(const Observable* object)
	{
		UpdateText();
	}

	void SetShape(Shape* s)
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

	void UpdateText()
	{
		SetText(shape->Name());
		if (fListView->LockLooper()) {
			fListView->InvalidateItem(fListView->IndexOf(this));
			fListView->UnlockLooper();
		}
	}

public:
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


ShapeListView::ShapeListView(BRect frame, const char* name, BMessage* message,
	BHandler* target)
	:
	SimpleListView(frame, name, NULL, B_MULTIPLE_SELECTION_LIST),
	fMessage(message),
	fShapeContainer(NULL),
	fCommandStack(NULL)
{
	SetDragCommand(MSG_DRAG_SHAPE);
	SetTarget(target);
}


ShapeListView::~ShapeListView()
{
	_MakeEmpty();
	delete fMessage;

	if (fShapeContainer != NULL)
		fShapeContainer->RemoveListener(this);
}


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


void
ShapeListView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_REMOVE:
			RemoveSelected();
			break;

		case MSG_DUPLICATE:
		{
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
		
		case MSG_RESET_TRANSFORMATION:
		{
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
		
		case MSG_FREEZE_TRANSFORMATION:
		{
			BList shapes;
			_GetSelectedShapes(shapes);
			int32 count = shapes.CountItems();
			if (count < 0)
				break;

			FreezeTransformationCommand* command
				= new FreezeTransformationCommand((Shape**)shapes.Items(),
					count);

			fCommandStack->Perform(command);
			break;
		}

		default:
			SimpleListView::MessageReceived(message);
			break;
	}
}


void
ShapeListView::MakeDragMessage(BMessage* message) const
{
	SimpleListView::MakeDragMessage(message);
	message->AddPointer("container", fShapeContainer);
	int32 count = CountSelectedItems();
	for (int32 i = 0; i < count; i++) {
		ShapeListItem* item = dynamic_cast<ShapeListItem*>(
			ItemAt(CurrentSelection(i)));
		if (item != NULL)
			message->AddPointer("shape", (void*)item->shape);
		else
			break;
	}
}


bool
ShapeListView::AcceptDragMessage(const BMessage* message) const
{
	return SimpleListView::AcceptDragMessage(message);
}


void
ShapeListView::SetDropTargetRect(const BMessage* message, BPoint where)
{
	SimpleListView::SetDropTargetRect(message, where);
}


// #pragma mark -


void
ShapeListView::MoveItems(BList& items, int32 toIndex)
{
	if (fCommandStack == NULL || fShapeContainer == NULL)
		return;

	int32 count = items.CountItems();
	Shape** shapes = new(nothrow) Shape*[count];
	if (shapes == NULL)
		return;

	for (int32 i = 0; i < count; i++) {
		ShapeListItem* item
			= dynamic_cast<ShapeListItem*>((BListItem*)items.ItemAtFast(i));
		shapes[i] = item ? item->shape : NULL;
	}

	MoveShapesCommand* command = new (nothrow) MoveShapesCommand(
		fShapeContainer, shapes, count, toIndex);
	if (command == NULL) {
		delete[] shapes;
		return;
	}

	fCommandStack->Perform(command);
}

// CopyItems
void
ShapeListView::CopyItems(BList& items, int32 toIndex)
{
	if (fCommandStack == NULL || fShapeContainer == NULL)
		return;

	int32 count = items.CountItems();
	Shape* shapes[count];

	for (int32 i = 0; i < count; i++) {
		ShapeListItem* item
			= dynamic_cast<ShapeListItem*>((BListItem*)items.ItemAtFast(i));
		shapes[i] = item ? new(nothrow) Shape(*item->shape) : NULL;
	}

	AddShapesCommand* command = new(nothrow) AddShapesCommand(fShapeContainer,
		shapes, count, toIndex, fSelection);
	if (command == NULL) {
		for (int32 i = 0; i < count; i++)
			delete shapes[i];
		return;
	}

	fCommandStack->Perform(command);
}


void
ShapeListView::RemoveItemList(BList& items)
{
	if (fCommandStack == NULL || fShapeContainer == NULL)
		return;

	int32 count = items.CountItems();
	int32 indices[count];
	for (int32 i = 0; i < count; i++)
	 	indices[i] = IndexOf((BListItem*)items.ItemAtFast(i));

	RemoveShapesCommand* command = new(nothrow) RemoveShapesCommand(
		fShapeContainer, indices, count);

	fCommandStack->Perform(command);
}


BListItem*
ShapeListView::CloneItem(int32 index) const
{
	ShapeListItem* item = dynamic_cast<ShapeListItem*>(ItemAt(index));
	if (item != NULL) {
		return new ShapeListItem(item->shape,
			const_cast<ShapeListView*>(this));
	}
	return NULL;
}


int32
ShapeListView::IndexOfSelectable(Selectable* selectable) const
{
	Shape* shape = dynamic_cast<Shape*>(selectable);
	if (shape == NULL) {
		Transformer* transformer = dynamic_cast<Transformer*>(selectable);
		if (transformer == NULL)
			return -1;
		int32 count = CountItems();
		for (int32 i = 0; i < count; i++) {
			ShapeListItem* item = dynamic_cast<ShapeListItem*>(ItemAt(i));
			if (item != NULL && item->shape->HasTransformer(transformer))
				return i;
		}
	} else {
		int32 count = CountItems();
		for (int32 i = 0; i < count; i++) {
			ShapeListItem* item = dynamic_cast<ShapeListItem*>(ItemAt(i));
			if (item != NULL && item->shape == shape)
				return i;
		}
	}

	return -1;
}


Selectable*
ShapeListView::SelectableFor(BListItem* item) const
{
	ShapeListItem* shapeItem = dynamic_cast<ShapeListItem*>(item);
	if (shapeItem != NULL)
		return shapeItem->shape;
	return NULL;
}


// #pragma mark -


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


void
ShapeListView::SetShapeContainer(ShapeContainer* container)
{
	if (fShapeContainer == container)
		return;

	// detach from old container
	if (fShapeContainer != NULL)
		fShapeContainer->RemoveListener(this);

	_MakeEmpty();

	fShapeContainer = container;

	if (fShapeContainer == NULL)
		return;

	fShapeContainer->AddListener(this);

	// sync
	int32 count = fShapeContainer->CountShapes();
	for (int32 i = 0; i < count; i++)
		_AddShape(fShapeContainer->ShapeAtFast(i), i);
}


void
ShapeListView::SetCommandStack(CommandStack* stack)
{
	fCommandStack = stack;
}


// #pragma mark -


bool
ShapeListView::_AddShape(Shape* shape, int32 index)
{
	if (shape == NULL)
		return false;
	
	ShapeListItem* item = new(std::nothrow) ShapeListItem(shape, this);
	if (item == NULL)
		return false;
	
	if (!AddItem(item, index)) {
		delete item;
		return false;
	}
	
	return true;
}


bool
ShapeListView::_RemoveShape(Shape* shape)
{
	ShapeListItem* item = _ItemForShape(shape);
	if (item != NULL && RemoveItem(item)) {
		delete item;
		return true;
	}
	return false;
}


ShapeListItem*
ShapeListView::_ItemForShape(Shape* shape) const
{
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		ShapeListItem* item = dynamic_cast<ShapeListItem*>(ItemAt(i));
		if (item != NULL && item->shape == shape)
			return item;
	}
	return NULL;
}


void
ShapeListView::_UpdateMenu()
{
	if (fMenu == NULL)
		return;

	bool gotSelection = CurrentSelection(0) >= 0;

	fDuplicateMI->SetEnabled(gotSelection);
	fResetTransformationMI->SetEnabled(gotSelection);
	fFreezeTransformationMI->SetEnabled(gotSelection);
	fRemoveMI->SetEnabled(gotSelection);
}


void
ShapeListView::_GetSelectedShapes(BList& shapes) const
{
	int32 count = CountSelectedItems();
	for (int32 i = 0; i < count; i++) {
		ShapeListItem* item = dynamic_cast<ShapeListItem*>(
			ItemAt(CurrentSelection(i)));
		if (item != NULL && item->shape != NULL) {
			if (!shapes.AddItem((void*)item->shape))
				break;
		}
	}
}
