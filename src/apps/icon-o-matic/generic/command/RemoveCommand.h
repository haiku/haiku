/*
 * Copyright 2006-2007, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef REMOVE_COMMAND_H
#define REMOVE_COMMAND_H


#include <new>
#include <string.h>

#include <Catalog.h>
#include <Locale.h>

#include "Command.h"
#include "Container.h"
#include "IconBuild.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-RemoveItemsCmd"

using std::nothrow;

_USING_ICON_NAMESPACE


/*! Removes items located at certain \c indices from a \c Container.

	\note This class should be subclassed and the \c GetName member overridden.
*/
template <class Type>
class RemoveCommand : public Command {
 public:
								RemoveCommand(
									Container<Type>* container,
									const int32* indices,
									int32 count);
	virtual						~RemoveCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 protected:
			Container<Type>*	fContainer;
			Type**				fItems;
			int32*				fIndices;
			int32				fCount;
			bool				fItemsRemoved;
};


template <class Type>
RemoveCommand<Type>::RemoveCommand(
		Container<Type>* container, const int32* indices, int32 count)
	: Command(),
	  fContainer(container),
	  fItems(count > 0 ? new (nothrow) Type*[count] : NULL),
	  fIndices(count > 0 ? new (nothrow) int32[count] : NULL),
	  fCount(count),
	  fItemsRemoved(false)
{
	if (!fContainer || !fItems || !fIndices)
		return;

	memcpy(fIndices, indices, sizeof(int32) * fCount);
	for (int32 i = 0; i < fCount; i++)
		fItems[i] = fContainer->ItemAt(fIndices[i]);
}


template <class Type>
RemoveCommand<Type>::~RemoveCommand()
{
	if (fItemsRemoved && fItems) {
		for (int32 i = 0; i < fCount; i++) {
			if (fItems[i])
				fItems[i]->ReleaseReference();
		}
	}
	delete[] fItems;
	delete[] fIndices;
}


template <class Type>
status_t
RemoveCommand<Type>::InitCheck()
{
	return fContainer && fItems && fIndices ? B_OK : B_NO_INIT;
}


template <class Type>
status_t
RemoveCommand<Type>::Perform()
{
	status_t ret = B_OK;

	// remove shapes from container
	for (int32 i = 0; i < fCount; i++) {
		if (fItems[i] && !fContainer->RemoveItem(fItems[i])) {
			ret = B_ERROR;
			break;
		}
	}
	fItemsRemoved = true;

	return ret;
}


template <class Type>
status_t
RemoveCommand<Type>::Undo()
{
	status_t ret = B_OK;

	// add shapes to container at remembered indices
	for (int32 i = 0; i < fCount; i++) {
		if (fItems[i] && !fContainer->AddItem(fItems[i], fIndices[i])) {
			ret = B_ERROR;
			break;
		}
	}
	fItemsRemoved = false;

	return ret;
}


template <class Type>
void
RemoveCommand<Type>::GetName(BString& name)
{
//	if (fCount > 1)
//		name << _GetString(MOVE_MODIFIERS, "Move Items");
//	else
//		name << _GetString(MOVE_MODIFIER, "Move Item");
	if (fCount > 1)
		name << B_TRANSLATE("Remove Items");
	else
		name << B_TRANSLATE("Remove Item");
}
#endif // REMOVE_COMMAND_H
