/*
 * Copyright 2006-2009, 2023, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
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
#include "ReferenceImage.h"
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
					TransformerItem(Transformer* t, TransformerListView* listView)
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
							transformer->ReleaseReference();
						}

						transformer = t;

						if (transformer) {
							transformer->AcquireReference();
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


TransformerListView::~TransformerListView()
{
	_MakeEmpty();
	delete fMessage;

	if (fShape)
		fShape->Transformers()->RemoveListener(this);
}


void
TransformerListView::Draw(BRect updateRect)
{
	SimpleListView::Draw(updateRect);

	if (fShape)
		return;

	// display helpful messages
	const char* message1 = B_TRANSLATE_COMMENT("Click on a shape above",
 		"Empty transformers list - 1st line");
	const char* message2 = B_TRANSLATE_COMMENT("to attach transformers.",
 		"Empty transformers list - 2nd line");

	// Dark Themes
	rgb_color lowColor = LowColor();
	if (lowColor.red + lowColor.green + lowColor.blue > 128 * 3)
		SetHighColor(tint_color(LowColor(), B_DARKEN_2_TINT));
	else
		SetHighColor(tint_color(LowColor(), B_LIGHTEN_2_TINT));

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
				= TransformerFactory::TransformerFor(type, fShape->VertexSource(), fShape);
			if (!transformer)
				break;

			Transformer* transformers[1];
			transformers[0] = transformer;
			::Command* command = new (nothrow) AddTransformersCommand(
				fShape->Transformers(), transformers, 1, fShape->Transformers()->CountItems());

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


status_t
TransformerListView::ArchiveSelection(BMessage* into, bool deep) const
{
	into->what = TransformerListView::kSelectionArchiveCode;

	int32 count = CountSelectedItems();
	for (int32 i = 0; i < count; i++) {
		TransformerItem* item = dynamic_cast<TransformerItem*>(
			ItemAt(CurrentSelection(i)));
		if (item != NULL) {
			BMessage archive;
			if (item->transformer->Archive(&archive, deep) == B_OK)
				into->AddMessage("transformer", &archive);
		} else
			return B_ERROR;
	}

	return B_OK;
}


bool
TransformerListView::InstantiateSelection(const BMessage* archive, int32 dropIndex)
{
	if (archive->what != TransformerListView::kSelectionArchiveCode
		|| fCommandStack == NULL || fShape == NULL)
		return false;

	// Drag may have come from another instance, like in another window.
	// Reconstruct the Styles from the archive and add them at the drop
	// index.
	int index = 0;
	BList transformers;
	while (true) {
		BMessage transformerArchive;
		if (archive->FindMessage("transformer", index, &transformerArchive) != B_OK)
			break;

		Transformer* transformer = TransformerFactory::TransformerFor(
			&transformerArchive, fShape->VertexSource(), fShape);
		if (transformer == NULL)
			break;

		if (!transformers.AddItem(transformer)) {
			delete transformer;
			break;
		}

		index++;
	}

	int32 count = transformers.CountItems();
	if (count == 0)
		return false;

	AddTransformersCommand* command = new(nothrow) AddTransformersCommand(
		fShape->Transformers(), (Transformer**)transformers.Items(), count, dropIndex);
	if (command == NULL) {
		for (int32 i = 0; i < count; i++)
			delete (Transformer*)transformers.ItemAtFast(i);
		return false;
	}

	fCommandStack->Perform(command);

	return true;
}


// #pragma mark -


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
		= new (nothrow) MoveTransformersCommand(
			fShape->Transformers(), transformers, count, toIndex);
	if (!command) {
		delete[] transformers;
		return;
	}

	fCommandStack->Perform(command);
}


void
TransformerListView::CopyItems(BList& items, int32 toIndex)
{
	MoveItems(items, toIndex);
	// TODO: allow copying items
}


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
		= new (nothrow) RemoveTransformersCommand(fShape->Transformers(), indices, count);
	fCommandStack->Perform(command);
}


BListItem*
TransformerListView::CloneItem(int32 index) const
{
	if (TransformerItem* item = dynamic_cast<TransformerItem*>(ItemAt(index))) {
		return new TransformerItem(item->transformer,
								   const_cast<TransformerListView*>(this));
	}
	return NULL;
}


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


Selectable*
TransformerListView::SelectableFor(BListItem* item) const
{
	TransformerItem* transformerItem = dynamic_cast<TransformerItem*>(item);
	if (transformerItem)
		return transformerItem->transformer;
	return NULL;
}

// #pragma mark -


void
TransformerListView::ItemAdded(Transformer* transformer, int32 index)
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


void
TransformerListView::ItemRemoved(Transformer* transformer)
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


// #pragma mark -


void
TransformerListView::SetMenu(BMenu* menu)
{
	if (fMenu == menu)
		return;

	fMenu = menu;
	if (fMenu == NULL)
		return;

	BMenu* addMenu = new BMenu(B_TRANSLATE("Add"));

	// Keep translated strings that were brought in from another file
#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Transformation"
	BMessage* message = new BMessage(MSG_ADD_TRANSFORMER);
	message->AddInt32("type", CONTOUR_TRANSFORMER);
	fContourMI = new BMenuItem(B_TRANSLATE("Contour"), message);

	message = new BMessage(MSG_ADD_TRANSFORMER);
	message->AddInt32("type", STROKE_TRANSFORMER);
	fStrokeMI = new BMenuItem(B_TRANSLATE("Stroke"), message);

	message = new BMessage(MSG_ADD_TRANSFORMER);
	message->AddInt32("type", PERSPECTIVE_TRANSFORMER);
	fPerspectiveMI = new BMenuItem(B_TRANSLATE("Perspective"), message);

	// message = new BMessage(MSG_ADD_TRANSFORMER);
	// message->AddInt32("type", AFFINE_TRANSFORMER);
	// fTransformationMI = new BMenuItem(B_TRANSLATE("Transformation"), message);
#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-TransformersList"

	addMenu->AddItem(fContourMI);
	addMenu->AddItem(fStrokeMI);
	addMenu->AddItem(fPerspectiveMI);

	addMenu->SetTargetForItems(this);
	fMenu->AddItem(addMenu);

	_UpdateMenu();
}


void
TransformerListView::SetShape(Shape* shape)
{
	if (fShape == shape)
		return;

	// detach from old container
	if (fShape)
		fShape->Transformers()->RemoveListener(this);

	_MakeEmpty();

	fShape = shape;

	if (fShape) {
		fShape->Transformers()->AddListener(this);

		int32 count = fShape->Transformers()->CountItems();
		for (int32 i = 0; i < count; i++)
			_AddTransformer(fShape->Transformers()->ItemAtFast(i), i);
	}

	_UpdateMenu();
}


void
TransformerListView::SetCommandStack(CommandStack* stack)
{
	fCommandStack = stack;
}


// #pragma mark -


bool
TransformerListView::_AddTransformer(Transformer* transformer, int32 index)
{
	if (transformer)
		 return AddItem(new TransformerItem(transformer, this), index);
	return false;
}


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


void
TransformerListView::_UpdateMenu()
{
	fMenu->SetEnabled(fShape != NULL);

	bool isReferenceImage = dynamic_cast<ReferenceImage*>(fShape) != NULL;
	fContourMI->SetEnabled(!isReferenceImage);
	fStrokeMI->SetEnabled(!isReferenceImage);
}
