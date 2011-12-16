/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef LIST_SELECTION_MODEL_H
#define LIST_SELECTION_MODEL_H


#include <SupportDefs.h>

#include <Array.h>
#include <ObjectList.h>


class ListSelectionModel {
public:
			class Listener;

public:
								ListSelectionModel();
								ListSelectionModel(
									const ListSelectionModel& other);
								~ListSelectionModel();

			int32				CountSelectedItems() const
									{ return fSelectedItems.Count(); }
			int32				SelectedItemAt(int32 index) const;
			bool				IsItemSelected(int32 itemIndex) const;

			void				Clear();
			bool				SelectItem(int32 itemIndex,
									bool extendSelection);
			bool				SelectItems(int32 itemIndex, int32 count,
									bool extendSelection);
			void				DeselectItem(int32 itemIndex);
			void				DeselectItems(int32 itemIndex, int32 count);

			void				ItemsAdded(int32 itemIndex, int32 count);
			void				ItemsRemoved(int32 itemIndex, int32 count);

			bool				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

			ListSelectionModel&	operator=(const ListSelectionModel& other);

private:
			typedef BObjectList<Listener> ListenerList;

private:
			int32				_FindItem(int32 itemIndex) const;
			int32				_CountSelectedItemsInRange(int32 index,
									int32 endItemIndex) const;

			void				_NotifyItemsSelected(int32 index, int32 count);
			void				_NotifyItemsDeselected(int32 index,
									int32 count);

private:
			Array<int32>		fSelectedItems;
			ListenerList		fListeners;
};


class ListSelectionModel::Listener {
public:
	virtual						~Listener();

	virtual	void				ItemsSelected(ListSelectionModel* model,
									int32 index, int32 count);
	virtual	void				ItemsDeselected(ListSelectionModel* model,
									int32 index, int32 count);
};


inline bool
ListSelectionModel::IsItemSelected(int32 itemIndex) const
{
	int32 index = _FindItem(itemIndex);
	return index < fSelectedItems.Count() && fSelectedItems[index] == itemIndex;
}


inline int32
ListSelectionModel::SelectedItemAt(int32 index) const
{
	return index >= 0 && index < fSelectedItems.Count()
		? fSelectedItems[index] : -1;
}


inline bool
ListSelectionModel::SelectItem(int32 itemIndex, bool extendSelection)
{
	return SelectItems(itemIndex, 1, extendSelection);
}


inline void
ListSelectionModel::DeselectItem(int32 itemIndex)
{
	DeselectItems(itemIndex, 1);
}


#endif	// LIST_SELECTION_MODEL_H
