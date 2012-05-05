/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef PATH_LIST_VIEW_H
#define PATH_LIST_VIEW_H


#include "ListViews.h"
#include "PathContainer.h"


class BMenu;
class BMenuItem;

_BEGIN_ICON_NAMESPACE
	class VectorPath;
	class Shape;
	class ShapeContainer;
_END_ICON_NAMESPACE

class CommandStack;
class PathListItem;
class Selection;
class ShapePathListener;


_USING_ICON_NAMESPACE


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

	virtual	void				MouseDown(BPoint where);
	virtual	void				MessageReceived(BMessage* message);

	virtual	void				MakeDragMessage(BMessage* message) const;

	virtual	bool				AcceptDragMessage(const BMessage* message) const;
	virtual	void				SetDropTargetRect(const BMessage* message,
									BPoint where);

	virtual	void				MoveItems(BList& items, int32 toIndex);
	virtual	void				CopyItems(BList& items, int32 toIndex);
	virtual	void				RemoveItemList(BList& items);

	virtual	BListItem*			CloneItem(int32 atIndex) const;

	virtual	int32				IndexOfSelectable(Selectable* selectable) const;
	virtual	Selectable*			SelectableFor(BListItem* item) const;

	// ShapeContainerListener interface
	virtual	void				PathAdded(VectorPath* path, int32 index);
	virtual	void				PathRemoved(VectorPath* path);

	// PathListView
			void				SetPathContainer(PathContainer* container);
			void				SetShapeContainer(ShapeContainer* container);
			void				SetCommandStack(CommandStack* stack);
			void				SetMenu(BMenu* menu);

			void				SetCurrentShape(Shape* shape);
			Shape*				CurrentShape() const
									{ return fCurrentShape; }

 private:
			bool				_AddPath(VectorPath* path, int32 index);
			bool				_RemovePath(VectorPath* path);

			PathListItem*		_ItemForPath(VectorPath* path) const;

	friend class ShapePathListener;
			void				_UpdateMarks();
			void				_SetPathMarked(VectorPath* path, bool marked);
			void				_UpdateMenu();

			BMessage*			fMessage;

			BMenu*				fMenu;
			BMenuItem*			fAddMI;
			BMenuItem*			fAddRectMI;
			BMenuItem*			fAddCircleMI;
			BMenuItem*			fAddArcMI;
			BMenuItem*			fDuplicateMI;
			BMenuItem*			fReverseMI;
			BMenuItem*			fCleanUpMI;
			BMenuItem*			fRotateIndicesLeftMI;
			BMenuItem*			fRotateIndicesRightMI;
			BMenuItem*			fRemoveMI;

			PathContainer*		fPathContainer;
			ShapeContainer*		fShapeContainer;
			CommandStack*		fCommandStack;

			Shape*				fCurrentShape;
				// those path items will be marked that
				// are referenced by this shape

			ShapePathListener*	fShapePathListener;
};

#endif // PATH_LIST_VIEW_H
