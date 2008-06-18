/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#include "UIUtils.h"

void MakeEmpty(BListView* list)
{
	if (list != NULL) {
		BListItem* item;
		while ((item = list->RemoveItem((int32)0)) != NULL) {
			delete item;
		}
	}
}

void RemoveChildren(BView* view)
{
	if (view != NULL) {
		BView* child;
		while ((child = view->ChildAt(0)) != NULL) {
			child->RemoveSelf();
			delete child;
		}
	}
}

