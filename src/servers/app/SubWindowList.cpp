/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrian Oanca <adioanca@cotty.iren.ro>
 */

/** List class for tracking floating and modal windows */

#include <stdio.h>
#include <List.h>

#include "SubWindowList.h"
#include "WinBorder.h"
#include "ServerWindow.h"


SubWindowList::SubWindowList(void)
{
}


SubWindowList::~SubWindowList(void)
{
}


void
SubWindowList::AddWinBorder(WinBorder *border)
{
	if (HasItem(border))
		return;

	int32 borderFeel = border->Feel();;
	int32 location = 0;

	for (; location < CountItems(); location++) {
		int32 feelTemp = ((WinBorder*)ItemAt(location))->Feel();

		// in short: if 'border' is a floating window and 'temp' a modal one
		if ((borderFeel == B_FLOATING_SUBSET_WINDOW_FEEL
				|| borderFeel == B_FLOATING_APP_WINDOW_FEEL
				|| borderFeel == B_FLOATING_ALL_WINDOW_FEEL)
			&& (feelTemp == B_MODAL_SUBSET_WINDOW_FEEL
				|| feelTemp == B_MODAL_APP_WINDOW_FEEL
				|| feelTemp == B_MODAL_ALL_WINDOW_FEEL)) {
			// means we found the place for our window('wb')
			break;
		}
	}
	AddItem(border, location);
}


void
SubWindowList::AddSubWindowList(SubWindowList *list)
{
	int32 i = 0;
	for (; i < CountItems(); i++) {
		int32 feel = ((WinBorder*)ItemAt(i))->Feel();
		if (feel == B_MODAL_SUBSET_WINDOW_FEEL
			|| feel == B_MODAL_APP_WINDOW_FEEL
			|| feel == B_MODAL_ALL_WINDOW_FEEL)
			break;
	}

	for (int32 j = 0; j < list->CountItems(); j++) {
		void *item = list->ItemAt(j);
		int32 feel = ((WinBorder*)item)->Feel();
		if (feel == B_MODAL_SUBSET_WINDOW_FEEL
			|| feel == B_MODAL_APP_WINDOW_FEEL
			|| feel == B_MODAL_ALL_WINDOW_FEEL) {
			AddItem(item, CountItems());
		} else {
			AddItem(item, i);
			i++;
		}
	}
}


void
SubWindowList::PrintToStream() const
{
	printf("Floating and modal windows list:\n");
	WinBorder* wb = NULL;

	for (int32 i=0; i<CountItems(); i++) {
		wb = (WinBorder*)ItemAt(i);

		printf("\t%s", wb->Name());

		if (wb->Feel() == B_FLOATING_SUBSET_WINDOW_FEEL)
			printf("\t%s\n", "B_FLOATING_SUBSET_WINDOW_FEEL");

		if (wb->Feel() == B_FLOATING_APP_WINDOW_FEEL)
			printf("\t%s\n", "B_FLOATING_APP_WINDOW_FEEL");

		if (wb->Feel() == B_FLOATING_ALL_WINDOW_FEEL)
			printf("\t%s\n", "B_FLOATING_ALL_WINDOW_FEEL");

		if (wb->Feel() == B_MODAL_SUBSET_WINDOW_FEEL)
			printf("\t%s\n", "B_MODAL_SUBSET_WINDOW_FEEL");

		if (wb->Feel() == B_MODAL_APP_WINDOW_FEEL)
			printf("\t%s\n", "B_MODAL_APP_WINDOW_FEEL");

		if (wb->Feel() == B_MODAL_ALL_WINDOW_FEEL)
			printf("\t%s\n", "B_MODAL_ALL_WINDOW_FEEL");

		// this should NOT happen
		if (wb->Feel() == B_NORMAL_WINDOW_FEEL)
			printf("\t%s\n", "B_NORMAL_WINDOW_FEEL");
	}
}
