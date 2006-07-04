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

class CommandStack;
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
	virtual	void				ShapeAdded(Shape* shape, int32 index);
	virtual	void				ShapeRemoved(Shape* shape);

	// ShapeListView
			void				SetShapeContainer(ShapeContainer* container);
			void				SetSelection(Selection* selection);
			void				SetCommandStack(CommandStack* stack);

 private:
			bool				_AddShape(Shape* shape, int32 index);
			bool				_RemoveShape(Shape* shape);

			ShapeListItem*		_ItemForShape(Shape* shape) const;

			BMessage*			fMessage;

			ShapeContainer*		fShapeContainer;
			Selection*			fSelection;
			CommandStack*		fCommandStack;
};

#endif // SHAPE_LIST_VIEW_H
