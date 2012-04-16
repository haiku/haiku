/*
 * Copyright 2006-2009, 2011, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "StyleListView.h"

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

#include "AddStylesCommand.h"
#include "AssignStyleCommand.h"
#include "CurrentColor.h"
#include "CommandStack.h"
#include "GradientTransformable.h"
#include "MoveStylesCommand.h"
#include "RemoveStylesCommand.h"
#include "Style.h"
#include "Observer.h"
#include "ResetTransformationCommand.h"
#include "Shape.h"
#include "ShapeContainer.h"
#include "Selection.h"
#include "Util.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-StylesList"


using std::nothrow;

static const float kMarkWidth		= 14.0;
static const float kBorderOffset	= 3.0;
static const float kTextOffset		= 4.0;

enum {
	MSG_ADD							= 'adst',
	MSG_REMOVE						= 'rmst',
	MSG_DUPLICATE					= 'dpst',
	MSG_RESET_TRANSFORMATION		= 'rstr',
};

class StyleListItem : public SimpleItem,
					 public Observer {
 public:
					StyleListItem(Style* s,
								  StyleListView* listView,
								  bool markEnabled)
						: SimpleItem(""),
						  style(NULL),
						  fListView(listView),
						  fMarkEnabled(markEnabled),
						  fMarked(false)
					{
						SetStyle(s);
					}

	virtual			~StyleListItem()
					{
						SetStyle(NULL);
					}

	// SimpleItem interface
	virtual	void	Draw(BView* owner, BRect itemFrame, uint32 flags)
	{
		SimpleItem::DrawBackground(owner, itemFrame, flags);

		// text
		owner->SetHighColor(0, 0, 0, 255);
		font_height fh;
		owner->GetFontHeight(&fh);
		BString truncatedString(Text());
		owner->TruncateString(&truncatedString, B_TRUNCATE_MIDDLE,
							  itemFrame.Width()
							  - kBorderOffset
							  - kMarkWidth
							  - kTextOffset
							  - kBorderOffset);
		float height = itemFrame.Height();
		float textHeight = fh.ascent + fh.descent;
		BPoint pos;
		pos.x = itemFrame.left
					+ kBorderOffset + kMarkWidth + kTextOffset;
		pos.y = itemFrame.top
					 + ceilf((height - textHeight) / 2.0 + fh.ascent);
		owner->DrawString(truncatedString.String(), pos);

		if (!fMarkEnabled)
			return;

		// mark
		BRect markRect = itemFrame;
		markRect.left += kBorderOffset;
		markRect.right = markRect.left + kMarkWidth;
		markRect.top = (markRect.top + markRect.bottom - kMarkWidth) / 2.0;
		markRect.bottom = markRect.top + kMarkWidth;
		owner->SetHighColor(tint_color(owner->LowColor(), B_DARKEN_1_TINT));
		owner->StrokeRect(markRect);
		markRect.InsetBy(1, 1);
		owner->SetHighColor(tint_color(owner->LowColor(), 1.04));
		owner->FillRect(markRect);
		if (fMarked) {
			markRect.InsetBy(2, 2);
			owner->SetHighColor(tint_color(owner->LowColor(),
								B_DARKEN_4_TINT));
			owner->SetPenSize(2);
			owner->StrokeLine(markRect.LeftTop(), markRect.RightBottom());
			owner->StrokeLine(markRect.LeftBottom(), markRect.RightTop());
			owner->SetPenSize(1);
		}
	}

	// Observer interface
	virtual	void	ObjectChanged(const Observable* object)
					{
						UpdateText();
					}

	// StyleListItem
			void	SetStyle(Style* s)
					{
						if (s == style)
							return;

						if (style) {
							style->RemoveObserver(this);
							style->Release();
						}

						style = s;

						if (style) {
							style->Acquire();
							style->AddObserver(this);
							UpdateText();
						}
					}
			void	UpdateText()
					{
						SetText(style->Name());
						Invalidate();
					}

			void	SetMarkEnabled(bool enabled)
					{
						if (fMarkEnabled == enabled)
							return;
						fMarkEnabled = enabled;
						Invalidate();
					}
			void	SetMarked(bool marked)
					{
						if (fMarked == marked)
							return;
						fMarked = marked;
						Invalidate();
					}

			void Invalidate()
					{
						// :-/
						if (fListView->LockLooper()) {
							fListView->InvalidateItem(
								fListView->IndexOf(this));
							fListView->UnlockLooper();
						}
					}

	Style* 	style;
 private:
	StyleListView*	fListView;
	bool			fMarkEnabled;
	bool			fMarked;
};


class ShapeStyleListener : public ShapeListener,
						   public ShapeContainerListener {
 public:
	ShapeStyleListener(StyleListView* listView)
		: fListView(listView),
		  fShape(NULL)
	{
	}
	virtual ~ShapeStyleListener()
	{
		SetShape(NULL);
	}

	// ShapeListener interface
	virtual	void TransformerAdded(Transformer* t, int32 index) {}
	virtual	void TransformerRemoved(Transformer* t) {}

	virtual void StyleChanged(Style* oldStyle, Style* newStyle)
	{
		fListView->_SetStyleMarked(oldStyle, false);
		fListView->_SetStyleMarked(newStyle, true);
	}

	// ShapeContainerListener interface
	virtual void ShapeAdded(Shape* shape, int32 index) {}
	virtual void ShapeRemoved(Shape* shape)
	{
		fListView->SetCurrentShape(NULL);
	}

	// ShapeStyleListener
	void SetShape(Shape* shape)
	{
		if (fShape == shape)
			return;

		if (fShape)
			fShape->RemoveListener(this);

		fShape = shape;

		if (fShape)
			fShape->AddListener(this);
	}

	Shape* CurrentShape() const
	{
		return fShape;
	}

 private:
	StyleListView*	fListView;
	Shape*			fShape;
};

// #pragma mark -

// constructor
StyleListView::StyleListView(BRect frame,
						   const char* name,
						   BMessage* message, BHandler* target)
	: SimpleListView(frame, name,
					 NULL, B_SINGLE_SELECTION_LIST),
	  fMessage(message),
	  fStyleContainer(NULL),
	  fShapeContainer(NULL),
	  fCommandStack(NULL),
	  fCurrentColor(NULL),

	  fCurrentShape(NULL),
	  fShapeListener(new ShapeStyleListener(this)),

	  fMenu(NULL)
{
	SetTarget(target);
}

// destructor
StyleListView::~StyleListView()
{
	_MakeEmpty();
	delete fMessage;

	if (fStyleContainer)
		fStyleContainer->RemoveListener(this);

	if (fShapeContainer)
		fShapeContainer->RemoveListener(fShapeListener);

	delete fShapeListener;
}

// #pragma mark -

// MessageReceived
void
StyleListView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_ADD: {
			Style* style;
			AddStylesCommand* command;
			rgb_color color;
			if (fCurrentColor != NULL)
				color = fCurrentColor->Color();
			else {
				color.red = 0;
				color.green = 0;
				color.blue = 0;
				color.alpha = 255;
			}
			new_style(color, fStyleContainer, &style, &command);
			fCommandStack->Perform(command);
			break;
		}
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
			int32 count = CountSelectedItems();
			BList gradients;
			for (int32 i = 0; i < count; i++) {
				StyleListItem* item = dynamic_cast<StyleListItem*>(
					ItemAt(CurrentSelection(i)));
				if (item && item->style && item->style->Gradient())
					if (!gradients.AddItem(
							(void*)item->style->Gradient()))
						break;
			}
			count = gradients.CountItems();
			if (count < 0)
				break;

			Transformable* transformables[count];
			for (int32 i = 0; i < count; i++) {
				Gradient* gradient = (Gradient*)gradients.ItemAtFast(i);
				transformables[i] = gradient;
			}

			ResetTransformationCommand* command = 
				new ResetTransformationCommand(transformables, count);

			fCommandStack->Perform(command);
			break;
		}
		default:
			SimpleListView::MessageReceived(message);
			break;
	}
}

// SelectionChanged
void
StyleListView::SelectionChanged()
{
	SimpleListView::SelectionChanged();

	if (!fSyncingToSelection) {
		// NOTE: single selection list
		StyleListItem* item
			= dynamic_cast<StyleListItem*>(ItemAt(CurrentSelection(0)));
		if (fMessage) {
			BMessage message(*fMessage);
			message.AddPointer("style", item ? (void*)item->style : NULL);
			Invoke(&message);
		}
	}

	_UpdateMenu();
}

// MouseDown
void
StyleListView::MouseDown(BPoint where)
{
	if (!fCurrentShape) {
		SimpleListView::MouseDown(where);
		return;
	}

	bool handled = false;
	int32 index = IndexOf(where);
	StyleListItem* item = dynamic_cast<StyleListItem*>(ItemAt(index));
	if (item) {
		BRect itemFrame(ItemFrame(index));
		itemFrame.right = itemFrame.left
							+ kBorderOffset + kMarkWidth
							+ kTextOffset / 2.0;
		Style* style = item->style;
		if (itemFrame.Contains(where)) {
			// set the style on the shape
			if (fCommandStack) {
				::Command* command = new AssignStyleCommand(
											fCurrentShape, style);
				fCommandStack->Perform(command);
			} else {
				fCurrentShape->SetStyle(style);
			}
			handled = true;
		}
	}

	if (!handled)
		SimpleListView::MouseDown(where);
}

// MakeDragMessage
void
StyleListView::MakeDragMessage(BMessage* message) const
{
	SimpleListView::MakeDragMessage(message);
	message->AddPointer("container", fStyleContainer);
	int32 count = CountSelectedItems();
	for (int32 i = 0; i < count; i++) {
		StyleListItem* item = dynamic_cast<StyleListItem*>(
			ItemAt(CurrentSelection(i)));
		if (item)
			message->AddPointer("style", (void*)item->style);
		else
			break;
	}
}

// AcceptDragMessage
bool
StyleListView::AcceptDragMessage(const BMessage* message) const
{
	return SimpleListView::AcceptDragMessage(message);
}

// SetDropTargetRect
void
StyleListView::SetDropTargetRect(const BMessage* message, BPoint where)
{
	SimpleListView::SetDropTargetRect(message, where);
}

// MoveItems
void
StyleListView::MoveItems(BList& items, int32 toIndex)
{
	if (!fCommandStack || !fStyleContainer)
		return;

	int32 count = items.CountItems();
	Style** styles = new (nothrow) Style*[count];
	if (!styles)
		return;

	for (int32 i = 0; i < count; i++) {
		StyleListItem* item
			= dynamic_cast<StyleListItem*>((BListItem*)items.ItemAtFast(i));
		styles[i] = item ? item->style : NULL;
	}

	MoveStylesCommand* command
		= new (nothrow) MoveStylesCommand(fStyleContainer,
										  styles, count, toIndex);
	if (!command) {
		delete[] styles;
		return;
	}

	fCommandStack->Perform(command);
}

// CopyItems
void
StyleListView::CopyItems(BList& items, int32 toIndex)
{
	if (!fCommandStack || !fStyleContainer)
		return;

	int32 count = items.CountItems();
	Style* styles[count];

	for (int32 i = 0; i < count; i++) {
		StyleListItem* item
			= dynamic_cast<StyleListItem*>((BListItem*)items.ItemAtFast(i));
		styles[i] = item ? new (nothrow) Style(*item->style) : NULL;
	}

	AddStylesCommand* command
		= new (nothrow) AddStylesCommand(fStyleContainer,
										 styles, count, toIndex);
	if (!command) {
		for (int32 i = 0; i < count; i++)
			delete styles[i];
		return;
	}

	fCommandStack->Perform(command);
}

// RemoveItemList
void
StyleListView::RemoveItemList(BList& items)
{
	if (!fCommandStack || !fStyleContainer)
		return;

	int32 count = items.CountItems();
	Style* styles[count];
	for (int32 i = 0; i < count; i++) {
		StyleListItem* item = dynamic_cast<StyleListItem*>(
			(BListItem*)items.ItemAtFast(i));
		if (item)
			styles[i] = item->style;
		else
			styles[i] = NULL;
	}

	RemoveStylesCommand* command
		= new (nothrow) RemoveStylesCommand(fStyleContainer,
											styles, count);
	fCommandStack->Perform(command);
}

// CloneItem
BListItem*
StyleListView::CloneItem(int32 index) const
{
	if (StyleListItem* item = dynamic_cast<StyleListItem*>(ItemAt(index))) {
		return new StyleListItem(item->style,
								 const_cast<StyleListView*>(this),
								 fCurrentShape != NULL);
	}
	return NULL;
}

// IndexOfSelectable
int32
StyleListView::IndexOfSelectable(Selectable* selectable) const
{
	Style* style = dynamic_cast<Style*>(selectable);
	if (!style)
		return -1;

	for (int32 i = 0;
		 StyleListItem* item = dynamic_cast<StyleListItem*>(ItemAt(i));
		 i++) {
		if (item->style == style)
			return i;
	}

	return -1;
}

// SelectableFor
Selectable*
StyleListView::SelectableFor(BListItem* item) const
{
	StyleListItem* styleItem = dynamic_cast<StyleListItem*>(item);
	if (styleItem)
		return styleItem->style;
	return NULL;
}

// #pragma mark -

// StyleAdded
void
StyleListView::StyleAdded(Style* style, int32 index)
{
	// NOTE: we are in the thread that messed with the
	// StyleContainer, so no need to lock the
	// container, when this is changed to asynchronous
	// notifications, then it would need to be read-locked!
	if (!LockLooper())
		return;

	if (_AddStyle(style, index))
		Select(index);

	UnlockLooper();
}

// StyleRemoved
void
StyleListView::StyleRemoved(Style* style)
{
	// NOTE: we are in the thread that messed with the
	// StyleContainer, so no need to lock the
	// container, when this is changed to asynchronous
	// notifications, then it would need to be read-locked!
	if (!LockLooper())
		return;

	// NOTE: we're only interested in Style objects
	_RemoveStyle(style);

	UnlockLooper();
}

// #pragma mark -

// SetMenu
void
StyleListView::SetMenu(BMenu* menu)
{
	if (fMenu == menu)
		return;

	fMenu = menu;
	if (fMenu == NULL)
		return;

	fAddMI = new BMenuItem(B_TRANSLATE("Add"), new BMessage(MSG_ADD));
	fMenu->AddItem(fAddMI);

	fMenu->AddSeparatorItem();

	fDuplicateMI = new BMenuItem(B_TRANSLATE("Duplicate"),
		new BMessage(MSG_DUPLICATE));
	fMenu->AddItem(fDuplicateMI);

	fResetTransformationMI = new BMenuItem(B_TRANSLATE("Reset transformation"),
		new BMessage(MSG_RESET_TRANSFORMATION));
	fMenu->AddItem(fResetTransformationMI);

	fMenu->AddSeparatorItem();

	fRemoveMI = new BMenuItem(B_TRANSLATE("Remove"), new BMessage(MSG_REMOVE));
	fMenu->AddItem(fRemoveMI);

	fMenu->SetTargetForItems(this);

	_UpdateMenu();
}

// SetStyleContainer
void
StyleListView::SetStyleContainer(StyleContainer* container)
{
	if (fStyleContainer == container)
		return;

	// detach from old container
	if (fStyleContainer)
		fStyleContainer->RemoveListener(this);

	_MakeEmpty();

	fStyleContainer = container;

	if (!fStyleContainer)
		return;

	fStyleContainer->AddListener(this);

	// sync
	int32 count = fStyleContainer->CountStyles();
	for (int32 i = 0; i < count; i++)
		_AddStyle(fStyleContainer->StyleAtFast(i), i);
}

// SetShapeContainer
void
StyleListView::SetShapeContainer(ShapeContainer* container)
{
	if (fShapeContainer == container)
		return;

	// detach from old container
	if (fShapeContainer)
		fShapeContainer->RemoveListener(fShapeListener);

	fShapeContainer = container;

	if (fShapeContainer)
		fShapeContainer->AddListener(fShapeListener);
}

// SetCommandStack
void
StyleListView::SetCommandStack(CommandStack* stack)
{
	fCommandStack = stack;
}

// SetCurrentColor
void
StyleListView::SetCurrentColor(CurrentColor* color)
{
	fCurrentColor = color;
}

// SetCurrentShape
void
StyleListView::SetCurrentShape(Shape* shape)
{
	if (fCurrentShape == shape)
		return;

	fCurrentShape = shape;
	fShapeListener->SetShape(shape);

	_UpdateMarks();
}

// #pragma mark -

// _AddStyle
bool
StyleListView::_AddStyle(Style* style, int32 index)
{
	if (style) {
		 return AddItem(new StyleListItem(
		 	style, this, fCurrentShape != NULL), index);
	}
	return false;
}

// _RemoveStyle
bool
StyleListView::_RemoveStyle(Style* style)
{
	StyleListItem* item = _ItemForStyle(style);
	if (item && RemoveItem(item)) {
		delete item;
		return true;
	}
	return false;
}

// _ItemForStyle
StyleListItem*
StyleListView::_ItemForStyle(Style* style) const
{
	for (int32 i = 0;
		 StyleListItem* item = dynamic_cast<StyleListItem*>(ItemAt(i));
		 i++) {
		if (item->style == style)
			return item;
	}
	return NULL;
}

// #pragma mark -

// _UpdateMarks
void
StyleListView::_UpdateMarks()
{
	int32 count = CountItems();
	if (fCurrentShape) {
		// enable display of marks and mark items whoes
		// style is contained in fCurrentShape
		for (int32 i = 0; i < count; i++) {
			StyleListItem* item = dynamic_cast<StyleListItem*>(ItemAt(i));
			if (!item)
				continue;
			item->SetMarkEnabled(true);
			item->SetMarked(fCurrentShape->Style() == item->style);
		}
	} else {
		// disable display of marks
		for (int32 i = 0; i < count; i++) {
			StyleListItem* item = dynamic_cast<StyleListItem*>(ItemAt(i));
			if (!item)
				continue;
			item->SetMarkEnabled(false);
		}
	}

	Invalidate();
}

// _SetStyleMarked
void
StyleListView::_SetStyleMarked(Style* style, bool marked)
{
	if (StyleListItem* item = _ItemForStyle(style)) {
		item->SetMarked(marked);
	}
}

// _UpdateMenu
void
StyleListView::_UpdateMenu()
{
	if (!fMenu)
		return;

	bool gotSelection = CurrentSelection(0) >= 0;

	fDuplicateMI->SetEnabled(gotSelection);
	// TODO: only enable fResetTransformationMI if styles
	// with gradients are selected!
	fResetTransformationMI->SetEnabled(gotSelection);
	fRemoveMI->SetEnabled(gotSelection);
}

