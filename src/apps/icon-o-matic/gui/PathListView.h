/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef PATH_LIST_VIEW_H
#define PATH_LIST_VIEW_H

#include "ListViews.h"
#include "PathContainer.h"

class VectorPath;
class PathListItem;
class Selection;

class PathListView : public SimpleListView,
					 public PathContainerListener {
 public:
								PathListView(BRect frame,
											 const char* name,
											 BMessage* selectionMessage = NULL,
											 BHandler* target = NULL);
	virtual						~PathListView();

	// SimpleListView interface
	virtual	void				SelectionChanged();

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				MakeDragMessage(BMessage* message) const;

	virtual	bool				AcceptDragMessage(const BMessage* message) const;
	virtual	void				SetDropTargetRect(const BMessage* message,
												  BPoint where);

	virtual	void				MoveItems(BList& items, int32 toIndex);
	virtual	void				CopyItems(BList& items, int32 toIndex);
	virtual	void				RemoveItemList(BList& indices);

	virtual	BListItem*			CloneItem(int32 atIndex) const;

	// ShapeContainerListener interface
	virtual	void				PathAdded(VectorPath* path);
	virtual	void				PathRemoved(VectorPath* path);

	// PathListView
			void				SetPathContainer(PathContainer* container);
			void				SetSelection(Selection* selection);

 private:
			bool				_AddPath(VectorPath* path);
			bool				_RemovePath(VectorPath* path);

			PathListItem*		_ItemForPath(VectorPath* path) const;
			void				_MakeEmpty();

			BMessage*			fMessage;

			PathContainer*		fPathContainer;
			Selection*			fSelection;
};

#endif // PATH_LIST_VIEW_H
