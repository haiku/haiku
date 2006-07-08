/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef TRANSFORMER_LIST_VIEW_H
#define TRANSFORMER_LIST_VIEW_H

#include "ListViews.h"
#include "Shape.h"

class CommandStack;
class Transformer;
class TransformerItem;
class Selection;

class TransformerListView : public SimpleListView,
							public ShapeListener {
 public:
								TransformerListView(BRect frame,
											  const char* name,
											  BMessage* selectionMessage = NULL,
											  BHandler* target = NULL);
	virtual						~TransformerListView();

	// SimpleListView interface
	virtual	void				SelectionChanged();

	virtual	void				MakeDragMessage(BMessage* message) const;

	virtual	void				MoveItems(BList& items, int32 toIndex);
	virtual	void				CopyItems(BList& items, int32 toIndex);
	virtual	void				RemoveItemList(BList& indices);

	virtual	BListItem*			CloneItem(int32 atIndex) const;

	// ShapeListener interface
	virtual	void				TransformerAdded(Transformer* transformer,
												 int32 index);
	virtual	void				TransformerRemoved(Transformer* transformer);

	virtual	void				StyleChanged(Style* oldStyle, Style* newStyle);

	// TransformerListView
			void				SetShape(Shape* shape);
			void				SetSelection(Selection* selection);
			void				SetCommandStack(CommandStack* stack);

 private:
			bool				_AddTransformer(
									Transformer* transformer, int32 index);
			bool				_RemoveTransformer(
									Transformer* transformer);

			TransformerItem*	_ItemForTransformer(
									Transformer* transformer) const;

			BMessage*			fMessage;

			Shape*				fShape;
			Selection*			fSelection;
			CommandStack*		fCommandStack;
};

#endif // TRANSFORMER_LIST_VIEW_H
