/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "GUISettingsUtils.h"

#include <Message.h>
#include <SplitView.h>

#include "table/AbstractTable.h"


/*static*/ status_t
GUISettingsUtils::ArchiveSplitView(BMessage& settings, BSplitView* view)
{
	settings.MakeEmpty();
	for (int32 i = 0; i < view->CountItems(); i++) {
		if (settings.AddFloat("weight", view->ItemWeight(i)) != B_OK)
			return B_NO_MEMORY;

		if (settings.AddFloat("collapsed", view->IsItemCollapsed(i)) != B_OK)
			return B_NO_MEMORY;
	}

	return B_OK;
}


/*static*/ void
GUISettingsUtils::UnarchiveSplitView(const BMessage& settings,
	BSplitView* view)
{
	for (int32 i = 0; i < view->CountItems(); i++) {
		float weight;
		if (settings.FindFloat("weight", i, &weight) == B_OK)
			view->SetItemWeight(i, weight, i == view->CountItems() - 1);

		bool collapsed;
		if (settings.FindBool("collapsed", i, &collapsed) == B_OK)
			view->SetItemCollapsed(i, collapsed);
	}
}


/*static*/ status_t
GUISettingsUtils::ArchiveTableSettings(BMessage& settings,
	AbstractTable* table)
{
	settings.MakeEmpty();
	table->SaveState(&settings);

	return B_OK;
}


/*static*/ void
GUISettingsUtils::UnarchiveTableSettings(const BMessage& settings,
	AbstractTable* table)
{
	BMessage settingsCopy(settings);
	table->LoadState(&settingsCopy);
}

