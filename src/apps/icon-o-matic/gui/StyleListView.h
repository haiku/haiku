/*
 * Copyright 2006-2007, 2011, Stephan Aßmus <superstippi@gmx.de>.
 * Distributed under the terms of the MIT License.
 */
#ifndef STYLE_LIST_VIEW_H
#define STYLE_LIST_VIEW_H


#include "ListViews.h"
#include "StyleContainer.h"


class BMenu;
class BMenuItem;
class CommandStack;
class CurrentColor;
class Selection;
class ShapeStyleListener;
class StyleListItem;

_BEGIN_ICON_NAMESPACE
	class Shape;
	class ShapeContainer;
	class ShapeListener;
	class Style;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class StyleListView : public SimpleListView, public StyleContainerListener {
public:
								StyleListView(BRect frame, const char* name,
									BMessage* selectionMessage = NULL,
									BHandler* target = NULL);
	virtual						~StyleListView();

	// SimpleListView interface
	virtual	void				MessageReceived(BMessage* message);

	virtual	void				SelectionChanged();

	virtual	void				MouseDown(BPoint where);

	virtual	void				MakeDragMessage(BMessage* message) const;

	virtual	bool				AcceptDragMessage(
									const BMessage* message) const;
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

	// StyleContainerListener interface
	virtual	void				StyleAdded(Style* style, int32 index);
	virtual	void				StyleRemoved(Style* style);

	// StyleListView
			void				SetMenu(BMenu* menu);
			void				SetStyleContainer(StyleContainer* container);
			void				SetShapeContainer(ShapeContainer* container);
			void				SetCommandStack(CommandStack* stack);
			void				SetCurrentColor(CurrentColor* color);

			void				SetCurrentShape(Shape* shape);
			Shape*				CurrentShape() const
									{ return fCurrentShape; }

private:
			bool				_AddStyle(Style* style, int32 index);
			bool				_RemoveStyle(Style* style);

			StyleListItem*		_ItemForStyle(Style* style) const;

			friend class ShapeStyleListener;
			void				_UpdateMarks();
			void				_SetStyleMarked(Style* style, bool marked);
			void				_UpdateMenu();

			BMessage*			fMessage;

			StyleContainer*		fStyleContainer;
			ShapeContainer*		fShapeContainer;
			CommandStack*		fCommandStack;
			CurrentColor*		fCurrentColor;

			Shape*				fCurrentShape;
				// the style item will be marked that
				// is referenced by this shape

			ShapeStyleListener*	fShapeListener;

			BMenu*				fMenu;
			BMenuItem*			fAddMI;
			BMenuItem*			fDuplicateMI;
			BMenuItem*			fResetTransformationMI;
			BMenuItem*			fRemoveMI;
};


#endif // STYLE_LIST_VIEW_H
