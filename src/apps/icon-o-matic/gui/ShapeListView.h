/*
 * Copyright 2006-2007, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef SHAPE_LIST_VIEW_H
#define SHAPE_LIST_VIEW_H


#include "Container.h"
#include "ListViews.h"
#include "IconBuild.h"


class BMenu;
class BMenuItem;
class CommandStack;
class ShapeListItem;
class Selection;

_BEGIN_ICON_NAMESPACE
	class Style;
	class Shape;
	class VectorPath;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


enum {
	MSG_ADD_SHAPE					= 'adsh',
	MSG_ADD_REFERENCE_IMAGE			= 'aimg',
};

class ShapeListView : public SimpleListView,
					  public ContainerListener<Shape> {
 public:
	enum {
		kSelectionArchiveCode		= 'sshp',
	};
									ShapeListView(BRect frame,
												  const char* name,
												  BMessage* selectionMessage = NULL,
												  BHandler* target = NULL);
	virtual							~ShapeListView();

	// SimpleListView interface
	virtual	void					SelectionChanged();

	virtual	void					MessageReceived(BMessage* message);

	virtual	status_t				ArchiveSelection(BMessage* into, bool deep = true) const;
	virtual	bool					InstantiateSelection(const BMessage* archive, int32 dropIndex);

	virtual	void					MoveItems(BList& items, int32 toIndex);
	virtual	void					CopyItems(BList& items, int32 toIndex);
	virtual	void					RemoveItemList(BList& items);

	virtual	BListItem*				CloneItem(int32 atIndex) const;

	virtual	int32					IndexOfSelectable(Selectable* selectable) const;
	virtual	Selectable*				SelectableFor(BListItem* item) const;

	// ContainerListener<Shape> interface
	virtual	void					ItemAdded(Shape* shape, int32 index);
	virtual	void					ItemRemoved(Shape* shape);

	// ShapeListView
			void					SetMenu(BMenu* menu);
			void					SetShapeContainer(Container<Shape>* container);
			void					SetStyleContainer(Container<Style>* container);
			void					SetPathContainer(Container<VectorPath>* container);
			void					SetCommandStack(CommandStack* stack);

 private:
			bool					_AddShape(Shape* shape, int32 index);
			bool					_RemoveShape(Shape* shape);

			ShapeListItem*			_ItemForShape(Shape* shape) const;
			void					_UpdateMenu();

			void					_GetSelectedShapes(BList& shapes) const;

			BMessage*				fMessage;

			Container<Shape>*		fShapeContainer;
			Container<Style>*		fStyleContainer;
			Container<VectorPath>*	fPathContainer;
			CommandStack*			fCommandStack;

			BMenu*					fMenu;
			BMenuItem*				fAddEmptyMI;
			BMenuItem*				fAddWidthPathMI;
			BMenuItem*				fAddWidthStyleMI;
			BMenuItem*				fAddWidthPathAndStyleMI;
			BMenuItem*				fAddReferenceImageMI;
			BMenuItem*				fDuplicateMI;
			BMenuItem*				fResetTransformationMI;
			BMenuItem*				fFreezeTransformationMI;
			BMenuItem*				fRemoveMI;
};

#endif // SHAPE_LIST_VIEW_H
