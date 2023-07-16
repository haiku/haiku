/*
 * Copyright 2006-2007, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef ADD_COMMAND_H
#define ADD_COMMAND_H

#include <new>
#include <stdio.h>
#include <string.h>

#include <Catalog.h>
#include <Locale.h>
#include <Referenceable.h>
#include <StringFormat.h>

#include "Command.h"
#include "Container.h"
#include "IconBuild.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-AddItemsCmd"


using std::nothrow;


_BEGIN_ICON_NAMESPACE
	class BReferenceable;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


/*! Adds items to a \c Container.

	\note This class should be subclassed and the \c GetName member overridden.
*/
template<class Type>
class AddCommand : public Command {
 public:
								AddCommand(
									Container<Type>* container,
									const Type* const* items,
									int32 count,
									bool ownsItems,
									int32 index);
	virtual						~AddCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 protected:
			Container<Type>*	fContainer;
			Type**				fItems;
			int32				fCount;
			bool				fOwnsItems;
			int32				fIndex;
			bool				fItemsAdded;
};


template<class Type>
AddCommand<Type>::AddCommand(Container<Type>* container,
		const Type* const* items, int32 count, bool ownsItems, int32 index)
	: Command(),
	  fContainer(container),
	  fItems(items && count > 0 ? new (nothrow) Type*[count] : NULL),
	  fCount(count),
	  fOwnsItems(ownsItems),
	  fIndex(index),
	  fItemsAdded(false)
{
	if (!fContainer || !fItems)
		return;

	memcpy(fItems, items, sizeof(Type*) * fCount);

	if (!fOwnsItems) {
		// Add references to items
		for (int32 i = 0; i < fCount; i++) {
			if (fItems[i] != NULL)
				fItems[i]->AcquireReference();
		}
	}
}


template<class Type>
AddCommand<Type>::~AddCommand()
{
	if (!fItemsAdded && fItems) {
		for (int32 i = 0; i < fCount; i++) {
			if (fItems[i] != NULL)
				fItems[i]->ReleaseReference();
		}
	}
	delete[] fItems;
}


template<class Type>
status_t
AddCommand<Type>::InitCheck()
{
	return fContainer && fItems ? B_OK : B_NO_INIT;
}


template<class Type>
status_t
AddCommand<Type>::Perform()
{
	// add items to container
	for (int32 i = 0; i < fCount; i++) {
		if (fItems[i] && !fContainer->AddItem(fItems[i], fIndex + i)) {
			// roll back
			for (int32 j = i - 1; j >= 0; j--)
				fContainer->RemoveItem(fItems[j]);
			return B_ERROR;
		}
	}
	fItemsAdded = true;

	return B_OK;
}


template<class Type>
status_t
AddCommand<Type>::Undo()
{
	// remove items from container
	for (int32 i = 0; i < fCount; i++) {
		fContainer->RemoveItem(fItems[i]);
	}
	fItemsAdded = false;

	return B_OK;
}


template<class Type>
void
AddCommand<Type>::GetName(BString& name)
{
	static BStringFormat format(B_TRANSLATE("Add {0, plural, "
		"one{Item} other{Items}}"));
	format.Format(name, fCount);
}
#endif // ADD_COMMAND_H
