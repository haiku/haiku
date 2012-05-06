/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef SHAPE_LIST_VIEW_H
#define SHAPE_LIST_VIEW_H


#include "ListViews.h"
#include "ShapeContainer.h"


class BMenu;
class BMenuItem;
class CommandStack;
class ShapeListItem;
class StyleContainer;
class PathContainer;
class Selection;

_BEGIN_ICON_NAMESPACE
	class Shape;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


enum {
	MSG_ADD_SHAPE					= 'adsh',
};

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
	virtual	bool				HandleDropMessage(const BMessage* message,
									int32 dropIndex);

	virtual	void				MoveItems(BList& items, int32 toIndex);
	virtual	void				CopyItems(BList& items, int32 toIndex);
	virtual	void				RemoveItemList(BList& items);

	virtual	BListItem*			CloneItem(int32 atIndex) const;

	virtual	int32				IndexOfSelectable(Selectable* selectable) const;
	virtual	Selectable*			SelectableFor(BListItem* item) const;

	// ShapeContainerListener interface
	virtual	void				ShapeAdded(Shape* shape, int32 index);
	virtual	void				ShapeRemoved(Shape* shape);

	// ShapeListView
			void				SetMenu(BMenu* menu);
			void				SetShapeContainer(ShapeContainer* container);
			void				SetStyleContainer(StyleContainer* container);
			void				SetPathContainer(PathContainer* container);
			void				SetCommandStack(CommandStack* stack);

 private:
			bool				_AddShape(Shape* shape, int32 index);
			bool				_RemoveShape(Shape* shape);

			ShapeListItem*		_ItemForShape(Shape* shape) const;
			void				_UpdateMenu();

			void				_GetSelectedShapes(BList& shapes) const;

			BMessage*			fMessage;

			ShapeContainer*		fShapeContainer;
			StyleContainer*		fStyleContainer;
			PathContainer*		fPathContainer;
			CommandStack*		fCommandStack;

			BMenu*				fMenu;
			BMenuItem*			fAddEmptyMI;
			BMenuItem*			fAddWidthPathMI;
			BMenuItem*			fAddWidthStyleMI;
			BMenuItem*			fAddWidthPathAndStyleMI;
			BMenuItem*			fDuplicateMI;
			BMenuItem*			fResetTransformationMI;
			BMenuItem*			fFreezeTransformationMI;
			BMenuItem*			fRemoveMI;
};

#endif // SHAPE_LIST_VIEW_H
