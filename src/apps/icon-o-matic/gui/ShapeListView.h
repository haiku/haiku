/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef SHAPE_LIST_VIEW_H
#define SHAPE_LIST_VIEW_H

#include "ListViews.h"
#include "ShapeContainer.h"

class Shape;
class ShapeListItem;
class Selection;

class ShapeListView : public SimpleListView,
					  public ShapeContainerListener {
 public:
								ShapeListView(BRect frame,
											  const char* name,
											  BMessage* selectionMessage = NULL,
											  BHandler* target = NULL);
	virtual						~ShapeListView();

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
	virtual	void				ShapeAdded(Shape* shape);
	virtual	void				ShapeRemoved(Shape* shape);

	// ShapeListView
			void				SetShapeContainer(ShapeContainer* container);
			void				SetSelection(Selection* selection);

 private:
			bool				_AddShape(Shape* shape);
			bool				_RemoveShape(Shape* shape);

			ShapeListItem*		_ItemForShape(Shape* shape) const;
			void				_MakeEmpty();

			BMessage*			fMessage;

			ShapeContainer*		fShapeContainer;
			Selection*			fSelection;
};

#endif // SHAPE_LIST_VIEW_H
