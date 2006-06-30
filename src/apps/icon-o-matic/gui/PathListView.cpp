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
#include "Selection.h"

class PathListItem : public SimpleItem,
					 public Observer {
 public:
					PathListItem(VectorPath* p,
								 PathListView* listView)
						: SimpleItem(""),
						  path(NULL),
						  fListView(listView)
					{
						SetPath(p);
					}

	virtual			~PathListItem()
					{
						SetPath(NULL);
					}

	virtual	void	ObjectChanged(const Observable* object)
					{
						UpdateText();
					}

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
	  fSelection(NULL)
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
								const_cast<PathListView*>(this));
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
	_AddPath(path);

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

// SetSelection
void
PathListView::SetSelection(Selection* selection)
{
	fSelection = selection;
}

// #pragma mark -

// _AddPath
bool
PathListView::_AddPath(VectorPath* path)
{
	if (path)
		 return AddItem(new PathListItem(path, this));
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

