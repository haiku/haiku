/*
 * Copyright 2006-2007, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef MOVE_COMMAND_H
#define MOVE_COMMAND_H

#include <new>
#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>
#include <StringFormat.h>

#include "Command.h"
#include "Container.h"
#include "IconBuild.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-MoveItemsCmd"

using std::nothrow;

_USING_ICON_NAMESPACE


/*! Moves a set of \c items to a different location in a \c container.

	\note This class should be subclassed and the \c GetName member overridden.
*/
template <class Type>
class MoveCommand : public Command {
 public:
								MoveCommand(
									Container<Type>* container,
									Type** items,
									int32 count,
									int32 toIndex);
	virtual						~MoveCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 protected:
			Container<Type>*	fContainer;
			Type**				fItems;
			int32*				fIndices;
			int32				fToIndex;
			int32				fCount;
};


template <class Type>
MoveCommand<Type>::MoveCommand(Container<Type>* container,
		Type** items,
		int32 count,
		int32 toIndex)
	: Command(),
	  fContainer(container),
	  fItems(items),
	  fIndices(count > 0 ? new (nothrow) int32[count] : NULL),
	  fToIndex(toIndex),
	  fCount(count)
{
	if (!fContainer || !fItems || !fIndices)
		return;

	// init original shape indices and
	// adjust toIndex compensating for items that
	// are removed before that index
	int32 itemsBeforeIndex = 0;
	for (int32 i = 0; i < fCount; i++) {
		fIndices[i] = fContainer->IndexOf(fItems[i]);
		if (fIndices[i] >= 0 && fIndices[i] < fToIndex)
			itemsBeforeIndex++;
	}
	fToIndex -= itemsBeforeIndex;
}


template <class Type>
MoveCommand<Type>::~MoveCommand()
{
	delete[] fItems;
	delete[] fIndices;
}


template <class Type>
status_t
MoveCommand<Type>::InitCheck()
{
	if (!fContainer || !fItems || !fIndices)
		return B_NO_INIT;

	// analyse the move, don't return B_OK in case
	// the container state does not change...

	int32 index = fIndices[0];
		// NOTE: fIndices == NULL if fCount < 1

	if (index != fToIndex) {
		// a change is guaranteed
		return B_OK;
	}

	// the insertion index is the same as the index of the first
	// moved item, a change only occures if the indices of the
	// moved items is not contiguous
	bool isContiguous = true;
	for (int32 i = 1; i < fCount; i++) {
		if (fIndices[i] != index + 1) {
			isContiguous = false;
			break;
		}
		index = fIndices[i];
	}
	if (isContiguous) {
		// the container state will not change because of the move
		return B_ERROR;
	}

	return B_OK;
}


template <class Type>
status_t
MoveCommand<Type>::Perform()
{
	status_t ret = B_OK;

	// remove paths from container
	for (int32 i = 0; i < fCount; i++) {
		if (fItems[i] && !fContainer->RemoveItem(fItems[i])) {
			ret = B_ERROR;
			break;
		}
	}
	if (ret < B_OK)
		return ret;

	// add paths to container at the insertion index
	int32 index = fToIndex;
	for (int32 i = 0; i < fCount; i++) {
		if (fItems[i] && !fContainer->AddItem(fItems[i], index++)) {
			ret = B_ERROR;
			break;
		}
	}

	return ret;
}


template <class Type>
status_t
MoveCommand<Type>::Undo()
{
	status_t ret = B_OK;

	// remove paths from container
	for (int32 i = 0; i < fCount; i++) {
		if (fItems[i] && !fContainer->RemoveItem(fItems[i])) {
			ret = B_ERROR;
			break;
		}
	}
	if (ret < B_OK)
		return ret;

	// add paths to container at remembered indices
	for (int32 i = 0; i < fCount; i++) {
		if (fItems[i] && !fContainer->AddItem(fItems[i], fIndices[i])) {
			ret = B_ERROR;
			break;
		}
	}

	return ret;
}


template <class Type>
void
MoveCommand<Type>::GetName(BString& name)
{
	static BStringFormat format(B_TRANSLATE("Move {0, plural, "
		"one{Item} other{Items}}"));
	format.Format(name, fCount);
}
#endif // MOVE_COMMAND_H
