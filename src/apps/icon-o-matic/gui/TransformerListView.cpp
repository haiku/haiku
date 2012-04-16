/*
 * Copyright 2006-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "TransformerListView.h"

#include <new>
#include <stdio.h>

#include <Application.h>
#include <Catalog.h>
#include <ListItem.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Mime.h>
#include <Message.h>
#include <Window.h>

#include "AddTransformersCommand.h"
#include "CommandStack.h"
#include "MoveTransformersCommand.h"
#include "RemoveTransformersCommand.h"
#include "Transformer.h"
#include "TransformerFactory.h"
#include "Observer.h"
#include "Selection.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-TransformersList"


using std::nothrow;

class TransformerItem : public SimpleItem,
						public Observer {
 public:
					TransformerItem(Transformer* t,
									TransformerListView* listView)
						: SimpleItem(t->Name()),
						  transformer(NULL),
						  fListView(listView)
					{
						SetTransformer(t);
					}

	virtual			~TransformerItem()
					{
						SetTransformer(NULL);
					}

	// Observer interface
	virtual	void	ObjectChanged(const Observable* object)
					{
						UpdateText();
					}

	// TransformerItem
			void	SetTransformer(Transformer* t)
					{
						if (t == transformer)
							return;

						if (transformer) {
							transformer->RemoveObserver(this);
							transformer->Release();
						}

						transformer = t;

						if (transformer) {
							transformer->Acquire();
							transformer->AddObserver(this);
							UpdateText();
						}
					}
			void	UpdateText()
					{
						SetText(transformer->Name());
						// :-/
						if (fListView->LockLooper()) {
							fListView->InvalidateItem(
								fListView->IndexOf(this));
							fListView->UnlockLooper();
						}
					}

	Transformer* 	transformer;
 private:
	TransformerListView* fListView;
};

// #pragma mark -

enum {
	MSG_DRAG_TRANSFORMER			= 'drgt',
	MSG_ADD_TRANSFORMER				= 'adtr',
};

// constructor
TransformerListView::TransformerListView(BRect frame, const char* name,
										 BMessage* message, BHandler* target)
	: SimpleListView(frame, name,
					 NULL, B_MULTIPLE_SELECTION_LIST),
	  fMessage(message),
	  fShape(NULL),
	  fCommandStack(NULL)
{
	SetDragCommand(MSG_DRAG_TRANSFORMER);
	SetTarget(target);
}

// destructor
TransformerListView::~TransformerListView()
{
	_MakeEmpty();
	delete fMessage;

	if (fShape)
		fShape->RemoveListener(this);
}

// Draw
void
TransformerListView::Draw(BRect updateRect)
{
	SimpleListView::Draw(updateRect);

	if (fShape)
		return;

	// display helpful messages
	const char* message1 = B_TRANSLATE_CONTEXT("Click on a shape above", 
		"Empty transformers list - 1st line");
	const char* message2 = B_TRANSLATE_CONTEXT("to attach transformers.",
		"Empty transformers list - 2nd line");

	SetHighColor(tint_color(LowColor(), B_DARKEN_2_TINT));
	font_height fh;
	GetFontHeight(&fh);
	BRect b(Bounds());

	BPoint middle;
	float textHeight = (fh.ascent + fh.descent) * 1.5;
	middle.y = (b.top + b.bottom - textHeight) / 2.0;
	middle.x = (b.left + b.right - StringWidth(message1)) / 2.0;
	DrawString(message1, middle);

	middle.y += textHeight;
	middle.x = (b.left + b.right - StringWidth(message2)) / 2.0;
	DrawString(message2, middle);
}

// SelectionChanged
void
TransformerListView::SelectionChanged()
{
	if (CountSelectedItems() > 0)
		SimpleListView::SelectionChanged();
	// else
	// TODO: any selected transformer will still be visible in the
	// PropertyListView

	if (!fSyncingToSelection) {
		TransformerItem* item
			= dynamic_cast<TransformerItem*>(ItemAt(CurrentSelection(0)));
		if (fMessage) {
			BMessage message(*fMessage);
			message.AddPointer("transformer", item ? (void*)item->transformer : NULL);
			Invoke(&message);
		}
	}
}

// MessageReceived
void
TransformerListView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_ADD_TRANSFORMER: {
			if (!fShape || !fCommandStack)
				break;
		
			uint32 type;
			if (message->FindInt32("type", (int32*)&type) < B_OK)
				break;
		
			Transformer* transformer
				= TransformerFactory::TransformerFor(type,
													 fShape->VertexSource());
			if (!transformer)
				break;
		
			Transformer* transformers[1];
			transformers[0] = transformer;
			::Command* command = new (nothrow) AddTransformersCommand(
				fShape, transformers, 1, fShape->CountTransformers());
		
			if (!command)
				delete transformer;
		
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
TransformerListView::MakeDragMessage(BMessage* message) const
{
	SimpleListView::MakeDragMessage(message);
	message->AddPointer("container", fShape);
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		TransformerItem* item = dynamic_cast<TransformerItem*>(ItemAt(CurrentSelection(i)));
		if (item) {
			message->AddPointer("transformer", (void*)item->transformer);
		} else
			break;
	}
}

// #pragma mark -

// MoveItems
void
TransformerListView::MoveItems(BList& items, int32 toIndex)
{
	if (!fCommandStack || !fShape)
		return;

	int32 count = items.CountItems();
	Transformer** transformers = new (nothrow) Transformer*[count];
	if (!transformers)
		return;

	for (int32 i = 0; i < count; i++) {
		TransformerItem* item
			= dynamic_cast<TransformerItem*>((BListItem*)items.ItemAtFast(i));
		transformers[i] = item ? item->transformer : NULL;
	}

	MoveTransformersCommand* command
		= new (nothrow) MoveTransformersCommand(fShape,
												transformers, count, toIndex);
	if (!command) {
		delete[] transformers;
		return;
	}

	fCommandStack->Perform(command);
}

// CopyItems
void
TransformerListView::CopyItems(BList& items, int32 toIndex)
{
	MoveItems(items, toIndex);
	// TODO: allow copying items
}

// RemoveItemList
void
TransformerListView::RemoveItemList(BList& items)
{
	if (!fCommandStack || !fShape)
		return;

	int32 count = items.CountItems();
	int32 indices[count];
	for (int32 i = 0; i < count; i++)
		indices[i] = IndexOf((BListItem*)items.ItemAtFast(i));

	RemoveTransformersCommand* command
		= new (nothrow) RemoveTransformersCommand(fShape,
												  indices, count);
	fCommandStack->Perform(command);
}

// CloneItem
BListItem*
TransformerListView::CloneItem(int32 index) const
{
	if (TransformerItem* item = dynamic_cast<TransformerItem*>(ItemAt(index))) {
		return new TransformerItem(item->transformer,
								   const_cast<TransformerListView*>(this));
	}
	return NULL;
}

// IndexOfSelectable
int32
TransformerListView::IndexOfSelectable(Selectable* selectable) const
{
	Transformer* transformer = dynamic_cast<Transformer*>(selectable);
	if (!transformer)
		return -1;

	for (int32 i = 0;
		 TransformerItem* item = dynamic_cast<TransformerItem*>(ItemAt(i));
		 i++) {
		if (item->transformer == transformer)
			return i;
	}

	return -1;
}

// SelectableFor
Selectable*
TransformerListView::SelectableFor(BListItem* item) const
{
	TransformerItem* transformerItem = dynamic_cast<TransformerItem*>(item);
	if (transformerItem)
		return transformerItem->transformer;
	return NULL;
}

// #pragma mark -

// TransformerAdded
void
TransformerListView::TransformerAdded(Transformer* transformer, int32 index)
{
	// NOTE: we are in the thread that messed with the
	// Shape, so no need to lock the document, when this is
	// changed to asynchronous notifications, then it would
	// need to be read-locked!
	if (!LockLooper())
		return;

	_AddTransformer(transformer, index);

	UnlockLooper();
}

// TransformerRemoved
void
TransformerListView::TransformerRemoved(Transformer* transformer)
{
	// NOTE: we are in the thread that messed with the
	// Shape, so no need to lock the document, when this is
	// changed to asynchronous notifications, then it would
	// need to be read-locked!
	if (!LockLooper())
		return;

	_RemoveTransformer(transformer);

	UnlockLooper();
}

// StyleChanged
void
TransformerListView::StyleChanged(Style* oldStyle, Style* newStyle)
{
	// we don't care
}

// #pragma mark -

// SetMenu
void
TransformerListView::SetMenu(BMenu* menu)
{
	if (fMenu == menu)
		return;

	fMenu = menu;
	if (fMenu == NULL)
		return;

	BMenu* addMenu = new BMenu(B_TRANSLATE("Add"));
	int32 cookie = 0;
	uint32 type;
	BString name;
	while (TransformerFactory::NextType(&cookie, &type, &name)) {
		// TODO: Disable the "Transformation" and "Perspective" transformers
		// since they are not very useful or even implemented at all.
		if (name == B_TRANSLATE("Transformation") 
			|| name == B_TRANSLATE("Perspective"))
			continue;
		// End of TODO.
		BMessage* message = new BMessage(MSG_ADD_TRANSFORMER);
		message->AddInt32("type", type);
		addMenu->AddItem(new BMenuItem(name.String(), message));
	}
	addMenu->SetTargetForItems(this);
	fMenu->AddItem(addMenu);

	_UpdateMenu();
}

// SetShape
void
TransformerListView::SetShape(Shape* shape)
{
	if (fShape == shape)
		return;

	// detach from old container
	if (fShape)
		fShape->RemoveListener(this);

	_MakeEmpty();

	fShape = shape;

	if (fShape) {
		fShape->AddListener(this);
	
		int32 count = fShape->CountTransformers();
		for (int32 i = 0; i < count; i++)
			_AddTransformer(fShape->TransformerAtFast(i), i);
	}

	_UpdateMenu();
}

// SetCommandStack
void
TransformerListView::SetCommandStack(CommandStack* stack)
{
	fCommandStack = stack;
}

// #pragma mark -

// _AddTransformer
bool
TransformerListView::_AddTransformer(Transformer* transformer, int32 index)
{
	if (transformer)
		 return AddItem(new TransformerItem(transformer, this), index);
	return false;
}

// _RemoveTransformer
bool
TransformerListView::_RemoveTransformer(Transformer* transformer)
{
	TransformerItem* item = _ItemForTransformer(transformer);
	if (item && RemoveItem(item)) {
		delete item;
		return true;
	}
	return false;
}

// _ItemForTransformer
TransformerItem*
TransformerListView::_ItemForTransformer(Transformer* transformer) const
{
	for (int32 i = 0;
		 TransformerItem* item = dynamic_cast<TransformerItem*>(ItemAt(i));
		 i++) {
		if (item->transformer == transformer)
			return item;
	}
	return NULL;
}

// _UpdateMenu
void
TransformerListView::_UpdateMenu()
{
	fMenu->SetEnabled(fShape != NULL);
}
