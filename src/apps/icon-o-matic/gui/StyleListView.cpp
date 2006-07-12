/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "StyleListView.h"

#include <stdio.h>

#include <Application.h>
#include <ListItem.h>
#include <Message.h>
#include <Mime.h>
#include <Window.h>

#include "Style.h"
#include "Observer.h"
#include "Shape.h"
#include "ShapeContainer.h"
#include "Selection.h"

static const float kMarkWidth		= 14.0;
static const float kBorderOffset	= 3.0;
static const float kTextOffset		= 4.0;

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
	  fSelection(NULL),
	  fCommandStack(NULL),

	  fCurrentShape(NULL),
	  fShapeListener(new ShapeStyleListener(this))
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

// SelectionChanged
void
StyleListView::SelectionChanged()
{
	// NOTE: single selection list

	StyleListItem* item
		= dynamic_cast<StyleListItem*>(ItemAt(CurrentSelection(0)));
	if (fMessage) {
		BMessage message(*fMessage);
		message.AddPointer("style", item ? (void*)item->style : NULL);
		Invoke(&message);
	}

	// modify global Selection
	if (!fSelection)
		return;

	if (item)
		fSelection->Select(item->style);
	else
		fSelection->DeselectAll();
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
// TODO: code this command...
//			Command* command = new SetStyleToShapeCommand(
//										fCurrentShape, style);
//			fCommandStack->Perform(command);
fCurrentShape->SetStyle(style);
			handled = true;
		}
	}

	if (!handled)
		SimpleListView::MouseDown(where);
}

// MessageReceived
void
StyleListView::MessageReceived(BMessage* message)
{
	SimpleListView::MessageReceived(message);
}

// MakeDragMessage
void
StyleListView::MakeDragMessage(BMessage* message) const
{
}

// AcceptDragMessage
bool
StyleListView::AcceptDragMessage(const BMessage* message) const
{
	return false;
}

// SetDropTargetRect
void
StyleListView::SetDropTargetRect(const BMessage* message, BPoint where)
{
}

// MoveItems
void
StyleListView::MoveItems(BList& items, int32 toIndex)
{
}

// CopyItems
void
StyleListView::CopyItems(BList& items, int32 toIndex)
{
}

// RemoveItemList
void
StyleListView::RemoveItemList(BList& items)
{
	// TODO: allow removing items
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

// #pragma mark -

// StyleAdded
void
StyleListView::StyleAdded(Style* style)
{
	// NOTE: we are in the thread that messed with the
	// StyleManager, so no need to lock the
	// container, when this is changed to asynchronous
	// notifications, then it would need to be read-locked!
	if (!LockLooper())
		return;

	// NOTE: shapes are always added at the end
	// of the list, so the sorting is synced...
	if (_AddStyle(style))
		Select(CountItems() - 1);

	UnlockLooper();
}

// StyleRemoved
void
StyleListView::StyleRemoved(Style* style)
{
	// NOTE: we are in the thread that messed with the
	// StyleManager, so no need to lock the
	// container, when this is changed to asynchronous
	// notifications, then it would need to be read-locked!
	if (!LockLooper())
		return;

	// NOTE: we're only interested in Style objects
	_RemoveStyle(style);

	UnlockLooper();
}

// #pragma mark -

// SetStyleManager
void
StyleListView::SetStyleManager(StyleManager* container)
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
//	if (!fStyleContainer->ReadLock())
//		return;

	int32 count = fStyleContainer->CountStyles();
	for (int32 i = 0; i < count; i++)
		_AddStyle(fStyleContainer->StyleAtFast(i));

//	fStyleContainer->ReadUnlock();
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

// SetSelection
void
StyleListView::SetSelection(Selection* selection)
{
	fSelection = selection;
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
StyleListView::_AddStyle(Style* style)
{
	if (style)
		 return AddItem(new StyleListItem(style, this, fCurrentShape != NULL));
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
