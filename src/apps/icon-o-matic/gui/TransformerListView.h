/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef TRANSFORMER_LIST_VIEW_H
#define TRANSFORMER_LIST_VIEW_H


#include "ListViews.h"
#include "Shape.h"


class BMenu;
class CommandStack;
class TransformerItem;
class Selection;

_BEGIN_ICON_NAMESPACE
	class Transformer;
_END_ICON_NAMESPACE


class TransformerListView : public SimpleListView,
							public ShapeListener {
 public:
								TransformerListView(BRect frame,
											  const char* name,
											  BMessage* selectionMessage = NULL,
											  BHandler* target = NULL);
	virtual						~TransformerListView();

	// SimpleListView interface
	virtual	void				Draw(BRect updateRect);

	virtual	void				SelectionChanged();

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				MakeDragMessage(BMessage* message) const;

	virtual	void				MoveItems(BList& items, int32 toIndex);
	virtual	void				CopyItems(BList& items, int32 toIndex);
	virtual	void				RemoveItemList(BList& items);

	virtual	BListItem*			CloneItem(int32 atIndex) const;

	virtual	int32				IndexOfSelectable(Selectable* selectable) const;
	virtual	Selectable*			SelectableFor(BListItem* item) const;

	// ShapeListener interface
	virtual	void				TransformerAdded(Transformer* transformer,
												 int32 index);
	virtual	void				TransformerRemoved(Transformer* transformer);

	virtual	void				StyleChanged(Style* oldStyle, Style* newStyle);

	// TransformerListView
			void				SetMenu(BMenu* menu);
			void				SetShape(Shape* shape);
			void				SetCommandStack(CommandStack* stack);

 private:
			bool				_AddTransformer(
									Transformer* transformer, int32 index);
			bool				_RemoveTransformer(
									Transformer* transformer);

			TransformerItem*	_ItemForTransformer(
									Transformer* transformer) const;

			void				_UpdateMenu();

			BMessage*			fMessage;

			Shape*				fShape;
			CommandStack*		fCommandStack;

			BMenu*				fMenu;
};

#endif // TRANSFORMER_LIST_VIEW_H
