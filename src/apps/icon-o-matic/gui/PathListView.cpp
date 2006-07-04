/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "PathListView.h"

#include <stdio.h>

#include <Application.h>
#include <ListItem.h>
#include <Message.h>
#include <Mime.h>
#include <Window.h>

#include "VectorPath.h"
#include "Observer.h"
#include "Shape.h"
#include "ShapeContainer.h"
#include "Selection.h"

static const float kMarkWidth		= 14.0;
static const float kBorderOffset	= 3.0;
static const float kTextOffset		= 4.0;

class PathListItem : public SimpleItem,
					 public Observer {
 public:
					PathListItem(VectorPath* p,
								 PathListView* listView,
								 bool markEnabled)
						: SimpleItem(""),
						  path(NULL),
						  fListView(listView),
						  fMarkEnabled(markEnabled),
						  fMarked(false)
					{
						SetPath(p);
					}

	virtual			~PathListItem()
					{
						SetPath(NULL);
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

	// PathListItem
			void	SetPath(VectorPath* p)
					{
						if (p == path)
							return;

						if (path) {
							path->RemoveObserver(this);
							path->Release();
						}

						path = p;

						if (path) {
							path->Acquire();
							path->AddObserver(this);
							UpdateText();
						}
					}
			void	UpdateText()
					{
						SetText(path->Name());
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

	VectorPath* 	path;
 private:
	PathListView*	fListView;
	bool			fMarkEnabled;
	bool			fMarked;
};


class ShapePathListener : public PathContainerListener,
						  public ShapeContainerListener {
 public:
	ShapePathListener(PathListView* listView)
		: fListView(listView),
		  fShape(NULL)
	{
	}
	virtual ~ShapePathListener()
	{
		SetShape(NULL);
	}

	virtual void PathAdded(VectorPath* path)
	{
		fListView->_SetPathMarked(path, true);
	}
	virtual void PathRemoved(VectorPath* path)
	{
		fListView->_SetPathMarked(path, false);
	}

	virtual void ShapeAdded(Shape* shape, int32 index) {}
	virtual void ShapeRemoved(Shape* shape)
	{
		fListView->SetCurrentShape(NULL);
	}

	void SetShape(Shape* shape)
	{
		if (fShape == shape)
			return;

		if (fShape)
			fShape->Paths()->RemoveListener(this);

		fShape = shape;

		if (fShape)
			fShape->Paths()->AddListener(this);
	}

	Shape* CurrentShape() const
	{
		return fShape;
	}

 private:
	PathListView*	fListView;
	Shape*			fShape;
};

// #pragma mark -

// constructor
PathListView::PathListView(BRect frame,
						   const char* name,
						   BMessage* message, BHandler* target)
	: SimpleListView(frame, name,
					 NULL, B_SINGLE_SELECTION_LIST),
	  fMessage(message),
	  fPathContainer(NULL),
	  fShapeContainer(NULL),
	  fSelection(NULL),
	  fCommandStack(NULL),

	  fCurrentShape(NULL),
	  fShapePathListener(new ShapePathListener(this))
{
	SetTarget(target);
}

// destructor
PathListView::~PathListView()
{
	_MakeEmpty();
	delete fMessage;

	if (fPathContainer)
		fPathContainer->RemoveListener(this);

	if (fShapeContainer)
		fShapeContainer->RemoveListener(fShapePathListener);

	delete fShapePathListener;
}

// SelectionChanged
void
PathListView::SelectionChanged()
{
	// NOTE: single selection list

	PathListItem* item
		= dynamic_cast<PathListItem*>(ItemAt(CurrentSelection(0)));
	if (item && fMessage) {
		BMessage message(*fMessage);
		message.AddPointer("path", (void*)item->path);
		Invoke(&message);
	}

	// modify global Selection
	if (!fSelection)
		return;

//	if (item)
//		fSelection->Select(item->path);
//	else
//		fSelection->DeselectAll();
}

// MouseDown
void
PathListView::MouseDown(BPoint where)
{
	if (!fCurrentShape) {
		SimpleListView::MouseDown(where);
		return;
	}

	bool handled = false;
	int32 index = IndexOf(where);
	PathListItem* item = dynamic_cast<PathListItem*>(ItemAt(index));
	if (item) {
		BRect itemFrame(ItemFrame(index));
		itemFrame.right = itemFrame.left
							+ kBorderOffset + kMarkWidth
							+ kTextOffset / 2.0;
		VectorPath* path = item->path;
		if (itemFrame.Contains(where)) {
			// add or remove the path to the shape
// TODO: code these commands...
//			Command* command;
//			if (fCurrentShape->Paths()->HasPath(path)) {
//				command = new RemovePathFromShapeCommand(
//								fCurrentShape, path);
//			} else {
//				command = new AddPathToShapeCommand(
//								fCurrentShape, path);
//			}
//			fCommandStack->Perform(command);
if (fCurrentShape->Paths()->HasPath(path)) {
	fCurrentShape->Paths()->RemovePath(path);
} else {
	fCurrentShape->Paths()->AddPath(path);
}
			handled = true;
		}
	}

	if (!handled)
		SimpleListView::MouseDown(where);
}

// MessageReceived
void
PathListView::MessageReceived(BMessage* message)
{
	SimpleListView::MessageReceived(message);
}

// MakeDragMessage
void
PathListView::MakeDragMessage(BMessage* message) const
{
}

// AcceptDragMessage
bool
PathListView::AcceptDragMessage(const BMessage* message) const
{
	return false;
}

// SetDropTargetRect
void
PathListView::SetDropTargetRect(const BMessage* message, BPoint where)
{
}

// MoveItems
void
PathListView::MoveItems(BList& items, int32 toIndex)
{
}

// CopyItems
void
PathListView::CopyItems(BList& items, int32 toIndex)
{
}

// RemoveItemList
void
PathListView::RemoveItemList(BList& indices)
{
	// TODO: allow removing items
}

// CloneItem
BListItem*
PathListView::CloneItem(int32 index) const
{
	if (PathListItem* item = dynamic_cast<PathListItem*>(ItemAt(index))) {
		return new PathListItem(item->path,
								const_cast<PathListView*>(this),
								fCurrentShape != NULL);
	}
	return NULL;
}

// #pragma mark -

// PathAdded
void
PathListView::PathAdded(VectorPath* path)
{
	// NOTE: we are in the thread that messed with the
	// ShapeContainer, so no need to lock the
	// container, when this is changed to asynchronous
	// notifications, then it would need to be read-locked!
	if (!LockLooper())
		return;

	// NOTE: shapes are always added at the end
	// of the list, so the sorting is synced...
	if (_AddPath(path))
		Select(CountItems() - 1);

	UnlockLooper();
}

// PathRemoved
void
PathListView::PathRemoved(VectorPath* path)
{
	// NOTE: we are in the thread that messed with the
	// ShapeContainer, so no need to lock the
	// container, when this is changed to asynchronous
	// notifications, then it would need to be read-locked!
	if (!LockLooper())
		return;

	// NOTE: we're only interested in VectorPath objects
	_RemovePath(path);

	UnlockLooper();
}

// #pragma mark -

// SetPathContainer
void
PathListView::SetPathContainer(PathContainer* container)
{
	if (fPathContainer == container)
		return;

	// detach from old container
	if (fPathContainer)
		fPathContainer->RemoveListener(this);

	_MakeEmpty();

	fPathContainer = container;

	if (!fPathContainer)
		return;

	fPathContainer->AddListener(this);

	// sync
//	if (!fPathContainer->ReadLock())
//		return;

	int32 count = fPathContainer->CountPaths();
	for (int32 i = 0; i < count; i++)
		_AddPath(fPathContainer->PathAtFast(i));

//	fPathContainer->ReadUnlock();
}

// SetShapeContainer
void
PathListView::SetShapeContainer(ShapeContainer* container)
{
	if (fShapeContainer == container)
		return;

	// detach from old container
	if (fShapeContainer)
		fShapeContainer->RemoveListener(fShapePathListener);

	fShapeContainer = container;

	if (fShapeContainer)
		fShapeContainer->AddListener(fShapePathListener);
}

// SetSelection
void
PathListView::SetSelection(Selection* selection)
{
	fSelection = selection;
}

// SetCurrentShape
void
PathListView::SetCurrentShape(Shape* shape)
{
	if (fCurrentShape == shape)
		return;

	fCurrentShape = shape;
	fShapePathListener->SetShape(shape);

	_UpdateMarks();
}

// #pragma mark -

// _AddPath
bool
PathListView::_AddPath(VectorPath* path)
{
	if (path)
		 return AddItem(new PathListItem(path, this, fCurrentShape != NULL));
	return false;
}

// _RemovePath
bool
PathListView::_RemovePath(VectorPath* path)
{
	PathListItem* item = _ItemForPath(path);
	if (item && RemoveItem(item)) {
		delete item;
		return true;
	}
	return false;
}

// _ItemForPath
PathListItem*
PathListView::_ItemForPath(VectorPath* path) const
{
	for (int32 i = 0;
		 PathListItem* item = dynamic_cast<PathListItem*>(ItemAt(i));
		 i++) {
		if (item->path == path)
			return item;
	}
	return NULL;
}

// _MakeEmpty
void
PathListView::_MakeEmpty()
{
	// NOTE: BListView::MakeEmpty() uses ScrollTo()
	// for which the object needs to be attached to
	// a BWindow.... :-(
	int32 count = CountItems();
	for (int32 i = count - 1; i >= 0; i--)
		delete RemoveItem(i);
}

// #pragma mark -

// _UpdateMarks
void
PathListView::_UpdateMarks()
{
	int32 count = CountItems();
	if (fCurrentShape) {
		// enable display of marks and mark items whoes
		// path is contained in fCurrentShape
		for (int32 i = 0; i < count; i++) {
			PathListItem* item = dynamic_cast<PathListItem*>(ItemAt(i));
			if (!item)
				continue;
			item->SetMarkEnabled(true);
			item->SetMarked(fCurrentShape->Paths()->HasPath(item->path));
		}
	} else {
		// disable display of marks
		for (int32 i = 0; i < count; i++) {
			PathListItem* item = dynamic_cast<PathListItem*>(ItemAt(i));
			if (!item)
				continue;
			item->SetMarkEnabled(false);
		}
	}

	Invalidate();
}

// _SetPathMarked
void
PathListView::_SetPathMarked(VectorPath* path, bool marked)
{
	if (PathListItem* item = _ItemForPath(path)) {
		item->SetMarked(marked);
	}
}
