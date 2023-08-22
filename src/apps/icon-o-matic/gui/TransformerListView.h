/*
 * Copyright 2006-2007, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef TRANSFORMER_LIST_VIEW_H
#define TRANSFORMER_LIST_VIEW_H


#include "Container.h"
#include "ListViews.h"
#include "IconBuild.h"


class BMenu;
class BMenuItem;
class CommandStack;
class TransformerItem;
class Selection;

_BEGIN_ICON_NAMESPACE
	class Shape;
	class Transformer;
_END_ICON_NAMESPACE


class TransformerListView : public SimpleListView,
							public ContainerListener<Transformer> {
 public:
	enum {
		kSelectionArchiveCode		= 'strn',
	};
								TransformerListView(BRect frame,
											  const char* name,
											  BMessage* selectionMessage = NULL,
											  BHandler* target = NULL);
	virtual						~TransformerListView();

	// SimpleListView interface
	virtual	void				Draw(BRect updateRect);

	virtual	void				SelectionChanged();

	virtual	void				MessageReceived(BMessage* message);

	virtual	status_t			ArchiveSelection(BMessage* into, bool deep = true) const;
	virtual	bool				InstantiateSelection(const BMessage* archive, int32 dropIndex);

	virtual	void				MoveItems(BList& items, int32 toIndex);
	virtual	void				CopyItems(BList& items, int32 toIndex);
	virtual	void				RemoveItemList(BList& items);

	virtual	BListItem*			CloneItem(int32 atIndex) const;

	virtual	int32				IndexOfSelectable(Selectable* selectable) const;
	virtual	Selectable*			SelectableFor(BListItem* item) const;

	// ContainerListener<Transformer> interface
	virtual	void				ItemAdded(Transformer* transformer, int32 index);
	virtual	void				ItemRemoved(Transformer* transformer);

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
			BMenuItem*			fContourMI;
			BMenuItem*			fStrokeMI;
			BMenuItem*			fPerspectiveMI;
};

#endif // TRANSFORMER_LIST_VIEW_H
