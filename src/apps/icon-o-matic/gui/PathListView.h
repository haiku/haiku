/*
 * Copyright 2006-2007, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef PATH_LIST_VIEW_H
#define PATH_LIST_VIEW_H


#include "Container.h"
#include "ListViews.h"
#include "IconBuild.h"


class BMenu;
class BMenuItem;

_BEGIN_ICON_NAMESPACE
	class VectorPath;
	class PathSourceShape;
	class Shape;
_END_ICON_NAMESPACE

class CommandStack;
class PathListItem;
class Selection;
class ShapePathListener;


_USING_ICON_NAMESPACE


class PathListView : public SimpleListView,
					 public ContainerListener<VectorPath> {
 public:
	enum {
		kSelectionArchiveCode		= 'spth',
	};
									PathListView(BRect frame,
												 const char* name,
												 BMessage* selectionMessage = NULL,
												 BHandler* target = NULL);
	virtual							~PathListView();

	// SimpleListView interface
	virtual	void					SelectionChanged();

	virtual	void					MouseDown(BPoint where);
	virtual	void					MessageReceived(BMessage* message);

	virtual	status_t				ArchiveSelection(BMessage* into, bool deep = true) const;
	virtual	bool					InstantiateSelection(const BMessage* archive, int32 dropIndex);

	virtual	void					MoveItems(BList& items, int32 toIndex);
	virtual	void					CopyItems(BList& items, int32 toIndex);
	virtual	void					RemoveItemList(BList& items);

	virtual	BListItem*				CloneItem(int32 atIndex) const;

	virtual	int32					IndexOfSelectable(Selectable* selectable) const;
	virtual	Selectable*				SelectableFor(BListItem* item) const;

	// ContainerListener<VectorPath> interface
	virtual	void					ItemAdded(VectorPath* path, int32 index);
	virtual	void					ItemRemoved(VectorPath* path);

	// PathListView
			void					SetPathContainer(Container<VectorPath>* container);
			void					SetShapeContainer(Container<Shape>* container);
			void					SetCommandStack(CommandStack* stack);
			void					SetMenu(BMenu* menu);

			void					SetCurrentShape(Shape* shape);
			PathSourceShape*		CurrentShape() const
										{ return fCurrentShape; }

 private:
			bool					_AddPath(VectorPath* path, int32 index);
			bool					_RemovePath(VectorPath* path);

			PathListItem*			_ItemForPath(VectorPath* path) const;

	friend class ShapePathListener;
			void					_UpdateMarks();
			void					_SetPathMarked(VectorPath* path, bool marked);
			void					_UpdateMenu();

			BMessage*				fMessage;

			BMenu*					fMenu;
			BMenuItem*				fAddMI;
			BMenuItem*				fAddRectMI;
			BMenuItem*				fAddCircleMI;
			BMenuItem*				fAddArcMI;
			BMenuItem*				fDuplicateMI;
			BMenuItem*				fReverseMI;
			BMenuItem*				fCleanUpMI;
			BMenuItem*				fRotateIndicesLeftMI;
			BMenuItem*				fRotateIndicesRightMI;
			BMenuItem*				fRemoveMI;

			Container<VectorPath>*	fPathContainer;
			Container<Shape>*		fShapeContainer;
			CommandStack*			fCommandStack;

			PathSourceShape*		fCurrentShape;
				// those path items will be marked that
				// are referenced by this shape

			ShapePathListener*		fShapePathListener;
};

#endif // PATH_LIST_VIEW_H
