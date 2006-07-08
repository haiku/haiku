/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef STYLE_LIST_VIEW_H
#define STYLE_LIST_VIEW_H

#include "ListViews.h"
#include "StyleManager.h"

class CommandStack;
class Selection;
class Shape;
class ShapeContainer;
class ShapeListener;
class Style;
class StyleListItem;

class StyleListView : public SimpleListView,
					  public StyleManagerListener {
 public:
								StyleListView(BRect frame,
											 const char* name,
											 BMessage* selectionMessage = NULL,
											 BHandler* target = NULL);
	virtual						~StyleListView();

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
	virtual	void				RemoveItemList(BList& indices);

	virtual	BListItem*			CloneItem(int32 atIndex) const;

	// StyleManagerListener interface
	virtual	void				StyleAdded(Style* style);
	virtual	void				StyleRemoved(Style* style);

	// StyleListView
			void				SetStyleManager(StyleManager* container);
			void				SetShapeContainer(ShapeContainer* container);
			void				SetSelection(Selection* selection);

			void				SetCurrentShape(Shape* shape);
			Shape*				CurrentShape() const
									{ return fCurrentShape; }

 private:
			bool				_AddStyle(Style* style);
			bool				_RemoveStyle(Style* style);

			StyleListItem*		_ItemForStyle(Style* style) const;

	friend class ShapeStyleListener;
			void				_UpdateMarks();
			void				_SetStyleMarked(Style* style, bool marked);

			BMessage*			fMessage;

			StyleManager*		fStyleContainer;
			ShapeContainer*		fShapeContainer;
			Selection*			fSelection;
			CommandStack*		fCommandStack;

			Shape*				fCurrentShape;
				// the style item will be marked that
				// is referenced by this shape

			ShapeStyleListener*	fShapeListener;
};

#endif // STYLE_LIST_VIEW_H
